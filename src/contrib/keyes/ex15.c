#ifdef PETSC_RCS_HEADER
static char vcid[] = "$Id: ex15.c,v 1.3 1999/10/06 19:10:35 bsmith Exp bsmith $";
#endif

static char help[] =
"This program demonstrates use of the SNES package to solve systems of\n\
nonlinear equations in parallel, using 2-dimensional distributed arrays.\n\
A 2-dim simplified RT test problem is used, with analytic Jacobian. \n\
\n\
  Solves the linear systems via 2 level additive Schwarz \n\
\n\
The command line\n\
options are:\n\
  -tleft <tl>, where <tl> indicates the left Diriclet BC \n\
  -tright <tr>, where <tr> indicates the right Diriclet BC \n\
  -Mx <xv>, where <xv> = number of coarse control volumes in the x-direction\n\
  -My <yv>, where <yv> = number of coarse control volumes in the y-direction\n\
  -ratio <r>, where <r> = ratio of fine volumes in each coarse in both x,y\n\
  -Nx <npx>, where <npx> = number of processors in the x-direction\n\
  -Ny <npy>, where <npy> = number of processors in the y-direction\n\n";

/*  
  
    This example models the partial differential equation 
   
         - Div(alpha* T^beta (GRAD T)) = 0.
       
    where beta = 2.5 and alpha = 1.0
 
    BC: T_left = 1.0 , T_right = 0.1, dT/dn_top = dTdn_bottom = 0.
    
    in the unit square, which is uniformly discretized in each of x and 
    y in this simple encoding.  The degrees of freedom are cell centered.
 
    A finite volume approximation with the usual 5-point stencil 
    is used to discretize the boundary value problem to obtain a 
    nonlinear system of equations. 
 
*/

#include "snes.h"
#include "da.h"
#include "mg.h"

/* User-defined application contexts */

typedef struct {
   int        mx,my;            /* number grid points in x and y direction */
   Vec        localX,localF;    /* local vectors with ghost region */
   DA         da;
   Vec        x,b,r;            /* global vectors */
   Mat        J;                /* Jacobian on grid */
   SLES       sles;
   Mat        R;
   Vec        Rscale;
} GridCtx;

#define MAX_LEVELS 10

typedef struct {
   GridCtx     grid[MAX_LEVELS];
   int         ratio;
   double      tleft, tright;  /* Dirichlet boundary conditions */
   double      beta, bm1, coef;/* nonlinear diffusivity parameterizations */
} AppCtx;

#define COARSE_LEVEL 0
#define FINE_LEVEL   1

extern int FormFunction(SNES,Vec,Vec,void*), FormInitialGuess1(AppCtx*,Vec);
extern int FormJacobian(SNES,Vec,Mat*,Mat*,MatStructure*,void*);
extern int FormInterpolation(AppCtx *);

/*
      Mm_ratio - ration of grid lines between fine and coarse grids.
*/
#undef __FUNC__
#define __FUNC__ "main"
int main( int argc, char **argv )
{
  SNES          snes;                      
  AppCtx        user;                      
  int           ierr, its, lits, N, n, Nx = PETSC_DECIDE, Ny = PETSC_DECIDE;
  int           size, flg,nlocal,Nlocal;
  double	atol, rtol, stol, litspit;
  int	        maxit, maxf;
  SLES          sles;
  PC            pc;
  PLogDouble    v1, v2, elapsed;
/*
    Initialize PETSc, note that default options in ex15options can be 
    overridden at the command line
*/
  PetscInitialize( &argc, &argv,"ex15options",help );

  user.ratio = 2;
  user.grid[0].mx = 5; user.grid[0].my = 5; 
  user.tleft = 1.0; user.tright = 0.1;
  user.beta = 2.5; user.bm1 = 1.5; user.coef = 1.25;
  ierr = OptionsGetInt(PETSC_NULL,"-Mx",&user.grid[0].mx,&flg); CHKERRA(ierr);
  ierr = OptionsGetInt(PETSC_NULL,"-My",&user.grid[0].my,&flg); CHKERRA(ierr);
  ierr = OptionsGetInt(PETSC_NULL,"-ratio",&user.ratio,&flg); CHKERRA(ierr);
  ierr = OptionsGetDouble(PETSC_NULL,"-tleft",&user.tleft,&flg); CHKERRA(ierr);
  ierr = OptionsGetDouble(PETSC_NULL,"-tright",&user.tright,&flg);CHKERRA(ierr);
  ierr = OptionsGetDouble(PETSC_NULL,"-beta",&user.beta,&flg); CHKERRA(ierr);
  ierr = OptionsGetDouble(PETSC_NULL,"-bm1",&user.bm1,&flg); CHKERRA(ierr);
  ierr = OptionsGetDouble(PETSC_NULL,"-coef",&user.coef,&flg); CHKERRA(ierr);
  user.grid[1].mx = user.ratio*(user.grid[0].mx-1)+1; user.grid[1].my = user.ratio*(user.grid[0].my-1)+1;
  ierr = PetscPrintf(PETSC_COMM_WORLD,"Coarse grid size %d by %d\n",user.grid[0].mx,user.grid[0].my);CHKERRA(ierr);
  ierr = PetscPrintf(PETSC_COMM_WORLD,"Fine grid size %d by %d\n",user.grid[1].mx,user.grid[1].my);CHKERRA(ierr);
  n = user.grid[1].mx*user.grid[1].my; N = user.grid[0].mx*user.grid[0].my;

  ierr = MPI_Comm_size(PETSC_COMM_WORLD,&size);CHKERRA(ierr);
  ierr = OptionsGetInt(PETSC_NULL,"-Nx",&Nx,&flg); CHKERRA(ierr);
  ierr = OptionsGetInt(PETSC_NULL,"-Ny",&Ny,&flg); CHKERRA(ierr);

  /* Set up distributed array for fine grid */
  ierr = DACreate2d(PETSC_COMM_WORLD,DA_NONPERIODIC,DA_STENCIL_STAR,user.grid[1].mx,
                    user.grid[1].my,Nx,Ny,1,1,PETSC_NULL,PETSC_NULL,&user.grid[1].da); CHKERRA(ierr);
  ierr = DACreateGlobalVector(user.grid[1].da,&user.grid[1].x); CHKERRA(ierr);
  ierr = VecDuplicate(user.grid[1].x,&user.grid[1].r); CHKERRA(ierr);
  ierr = VecDuplicate(user.grid[1].x,&user.grid[1].b); CHKERRA(ierr);
  ierr = VecGetLocalSize(user.grid[1].x,&nlocal);CHKERRA(ierr);
  ierr = DACreateLocalVector(user.grid[1].da,&user.grid[1].localX); CHKERRA(ierr);
  ierr = VecDuplicate(user.grid[1].localX,&user.grid[1].localF); CHKERRA(ierr);
  ierr = MatCreateMPIAIJ(PETSC_COMM_WORLD,nlocal,nlocal,n,n,5,PETSC_NULL,3,PETSC_NULL,&user.grid[1].J); CHKERRA(ierr);

  /* Set up distributed array for coarse grid */
  ierr = DACreate2d(PETSC_COMM_WORLD,DA_NONPERIODIC,DA_STENCIL_STAR,user.grid[0].mx,
                    user.grid[0].my,Nx,Ny,1,1,PETSC_NULL,PETSC_NULL,&user.grid[0].da); CHKERRA(ierr);
  ierr = DACreateGlobalVector(user.grid[0].da,&user.grid[0].x); CHKERRA(ierr);
  ierr = VecDuplicate(user.grid[0].x,&user.grid[0].b); CHKERRA(ierr);
  ierr = VecGetLocalSize(user.grid[0].x,&Nlocal);CHKERRA(ierr);
  ierr = DACreateLocalVector(user.grid[0].da,&user.grid[0].localX); CHKERRA(ierr);
  ierr = VecDuplicate(user.grid[0].localX,&user.grid[0].localF); CHKERRA(ierr);
  ierr = MatCreateMPIAIJ(PETSC_COMM_WORLD,Nlocal,Nlocal,N,N,5,PETSC_NULL,3,PETSC_NULL,&user.grid[0].J); CHKERRA(ierr);

  /* Create nonlinear solver */
  ierr = SNESCreate(PETSC_COMM_WORLD,SNES_NONLINEAR_EQUATIONS,&snes);CHKERRA(ierr);

  /* provide user function and Jacobian */
  ierr = SNESSetFunction(snes,user.grid[1].b,FormFunction,&user); CHKERRA(ierr);
  ierr = SNESSetJacobian(snes,user.grid[1].J,user.grid[1].J,FormJacobian,&user);CHKERRA(ierr);

  /* set two level additive Schwarz preconditioner */
  ierr = SNESGetSLES(snes,&sles);CHKERRA(ierr);
  ierr = SLESGetPC(sles,&pc); CHKERRA(ierr);
  ierr = PCSetType(pc,PCMG); CHKERRA(ierr);
  ierr = MGSetLevels(pc,2); CHKERRA(ierr);
  ierr = MGSetType(pc,MGADDITIVE); CHKERRA(ierr);

  /* Create coarse level */
  ierr = MGGetCoarseSolve(pc,&user.grid[0].sles); CHKERRA(ierr);
  ierr = SLESSetOptionsPrefix(user.grid[0].sles,"coarse_"); CHKERRA(ierr);
  ierr = SLESSetFromOptions(user.grid[0].sles); CHKERRA(ierr);
  ierr = SLESSetOperators(user.grid[0].sles,user.grid[0].J,user.grid[0].J,DIFFERENT_NONZERO_PATTERN);CHKERRA(ierr);
  ierr = MGSetX(pc,COARSE_LEVEL,user.grid[0].x);CHKERRA(ierr); 
  ierr = MGSetRhs(pc,COARSE_LEVEL,user.grid[0].b);CHKERRA(ierr); 

  /* Create fine level */
  ierr = MGGetSmoother(pc,FINE_LEVEL,&user.grid[1].sles); CHKERRA(ierr);
  ierr = SLESSetOptionsPrefix(user.grid[1].sles,"fine_"); CHKERRA(ierr);
  ierr = SLESSetFromOptions(user.grid[1].sles); CHKERRA(ierr);
  ierr = SLESSetOperators(user.grid[1].sles,user.grid[1].J,user.grid[1].J,DIFFERENT_NONZERO_PATTERN);CHKERRA(ierr);
  ierr = MGSetR(pc,FINE_LEVEL,user.grid[1].r);CHKERRA(ierr); 
  ierr = MGSetResidual(pc,FINE_LEVEL,MGDefaultResidual,user.grid[1].J); CHKERRA(ierr);

  /* Create interpolation between the levels */
  ierr = FormInterpolation(&user);CHKERRA(ierr);
  ierr = MGSetInterpolate(pc,FINE_LEVEL,user.grid[0].R);CHKERRA(ierr);
  ierr = MGSetRestriction(pc,FINE_LEVEL,user.grid[0].R);CHKERRA(ierr);

  /* Solve 1 Newton iteration of nonlinear system (to load all arrays) */
  ierr = SNESSetFromOptions(snes); CHKERRA(ierr);
  ierr = SNESGetTolerances(snes,&atol,&rtol,&stol,&maxit,&maxf); CHKERRA(ierr);
  ierr = SNESSetTolerances(snes,atol,rtol,stol,1,maxf); CHKERRA(ierr);
  ierr = FormInitialGuess1(&user,user.grid[1].x); CHKERRA(ierr);
  ierr = SNESSolve(snes,user.grid[1].x,&its); CHKERRA(ierr);
  ierr = PetscPrintf(PETSC_COMM_WORLD,"Pre-load Newton iterations = %d\n", its );CHKERRA(ierr);

  /* Reset options, start timer, then solve nonlinear system */
  ierr = SNESSetTolerances(snes,atol,rtol,stol,maxit,maxf); CHKERRA(ierr);
  ierr = FormInitialGuess1(&user,user.grid[1].x); CHKERRA(ierr);
  ierr = PLogStagePush(1);CHKERRA(ierr);
  ierr = PetscGetTime(&v1); CHKERRA(ierr);
  ierr = SNESSolve(snes,user.grid[1].x,&its); CHKERRA(ierr);
  ierr = PetscGetTime(&v2); CHKERRA(ierr);
  ierr = PLogStagePop();CHKERRA(ierr);
  elapsed = v2 - v1;
  ierr = SNESGetNumberLinearIterations(snes,&lits); CHKERRA(ierr);
  litspit = ((double)lits)/((double)its);
  ierr = PetscPrintf(PETSC_COMM_WORLD,"Elapsed Time = %e\n", elapsed );CHKERRA(ierr);
  ierr = PetscPrintf(PETSC_COMM_WORLD,"Number of Newton iterations = %d\n", its );CHKERRA(ierr);
  ierr = PetscPrintf(PETSC_COMM_WORLD,"Number of Linear iterations = %d\n", lits );CHKERRA(ierr);
  ierr = PetscPrintf(PETSC_COMM_WORLD,"Average Linear its / Newton = %e\n", litspit );CHKERRA(ierr);

  /* Free data structures */
  ierr = MatDestroy(user.grid[1].J); CHKERRA(ierr);
  ierr = VecDestroy(user.grid[1].x); CHKERRA(ierr);
  ierr = VecDestroy(user.grid[1].r); CHKERRA(ierr);
  ierr = VecDestroy(user.grid[1].b); CHKERRA(ierr);
  ierr = DADestroy(user.grid[1].da); CHKERRA(ierr);
  ierr = VecDestroy(user.grid[1].localX); CHKERRA(ierr);
  ierr = VecDestroy(user.grid[1].localF); CHKERRA(ierr);

  ierr = MatDestroy(user.grid[0].J); CHKERRA(ierr);
  ierr = VecDestroy(user.grid[0].x); CHKERRA(ierr);
  ierr = VecDestroy(user.grid[0].b); CHKERRA(ierr);
  ierr = DADestroy(user.grid[0].da); CHKERRA(ierr);
  ierr = VecDestroy(user.grid[0].localX); CHKERRA(ierr);
  ierr = VecDestroy(user.grid[0].localF); CHKERRA(ierr);

  ierr = SNESDestroy(snes); CHKERRA(ierr);
  ierr = MatDestroy(user.grid[0].R);CHKERRA(ierr); 
  ierr = VecDestroy(user.grid[0].Rscale);CHKERRA(ierr); 
  PetscFinalize();

  return 0;
}/* --------------------  Form initial approximation ----------------- */
#undef __FUNC__
#define __FUNC__ "FormInitialGuess1"
int FormInitialGuess1(AppCtx *user,Vec X)
{
  int     i, j, row, mx, my, ierr, xs, ys, xm, ym, Xm, Ym, Xs, Ys;
  double  one = 1.0, hx, hy, hxdhy, hydhx;
  double  tleft, tright;
  Scalar  *x;
  Vec     localX = user->grid[1].localX;

  mx = user->grid[1].mx;       my = user->grid[1].my;            
  hx = one/(double)(mx-1);  hy = one/(double)(my-1);
  hxdhy = hx/hy;            hydhx = hy/hx;
  tleft = user->tleft;      tright = user->tright;

  /* Get ghost points */
  ierr = DAGetCorners(user->grid[1].da,&xs,&ys,0,&xm,&ym,0); CHKERRQ(ierr);
  ierr = DAGetGhostCorners(user->grid[1].da,&Xs,&Ys,0,&Xm,&Ym,0); CHKERRQ(ierr);
  ierr = VecGetArray(localX,&x); CHKERRQ(ierr);

  /* Compute initial guess */
  for (j=ys; j<ys+ym; j++) {
    for (i=xs; i<xs+xm; i++) {
      row = i - Xs + (j - Ys)*Xm; 
      x[row] = tleft;
    }
  }
  ierr = VecRestoreArray(localX,&x); CHKERRQ(ierr);

  /* Insert values into global vector */
  ierr = DALocalToGlobal(user->grid[1].da,localX,INSERT_VALUES,X); CHKERRQ(ierr);
  return 0;
} /* --------------------  Evaluate Function F(x) --------------------- */
#undef __FUNC__
#define __FUNC__ "FormFunction"
int FormFunction(SNES snes,Vec X,Vec F,void *ptr)
{
  AppCtx  *user = (AppCtx *) ptr;
  int     ierr, i, j, row, mx, my, xs, ys, xm, ym, Xs, Ys, Xm, Ym;
  double  zero = 0.0, half = 0.5, one = 1.0;
  double  hx, hy, hxdhy, hydhx;
  double  t0, tn, ts, te, tw, an, as, ae, aw, dn, ds, de, dw, fn, fs, fe, fw;
  double  tleft, tright, beta;
  Scalar  *x,*f;
  Vec     localX = user->grid[1].localX, localF = user->grid[1].localF; 

  mx = user->grid[1].mx;       my = user->grid[1].my;       
  hx = one/(double)(mx-1);  hy = one/(double)(my-1);
  hxdhy = hx/hy;            hydhx = hy/hx;
  tleft = user->tleft;      tright = user->tright;
  beta = user->beta;
 
  /* Get ghost points */
  ierr = DAGlobalToLocalBegin(user->grid[1].da,X,INSERT_VALUES,localX); CHKERRQ(ierr);
  ierr = DAGlobalToLocalEnd(user->grid[1].da,X,INSERT_VALUES,localX); CHKERRQ(ierr);
  ierr = DAGetCorners(user->grid[1].da,&xs,&ys,0,&xm,&ym,0); CHKERRQ(ierr);
  ierr = DAGetGhostCorners(user->grid[1].da,&Xs,&Ys,0,&Xm,&Ym,0); CHKERRQ(ierr);
  ierr = VecGetArray(localX,&x); CHKERRQ(ierr);
  ierr = VecGetArray(localF,&f); CHKERRQ(ierr);

  /* Evaluate function */
  for (j=ys; j<ys+ym; j++) {
    row = (j - Ys)*Xm + xs - Xs - 1; 
    for (i=xs; i<xs+xm; i++) {
      row++;
      t0 = x[row];

      if (i > 0 && i < mx-1 && j > 0 && j < my-1) {

	/* general interior volume */

        tw = x[row - 1];   
        aw = 0.5*(t0 + tw);                 
        dw = pow(aw, beta);
        fw = dw*(t0 - tw);

	te = x[row + 1];
        ae = 0.5*(t0 + te);
        de = pow(ae, beta);
        fe = de*(te - t0);

	ts = x[row - Xm];
        as = 0.5*(t0 + ts);
        ds = pow(as, beta);
        fs = ds*(t0 - ts);
  
        tn = x[row + Xm];  
        an = 0.5*(t0 + tn);
        dn = pow(an, beta);
        fn = dn*(tn - t0);

      } else if (i == 0) {

	/* left-hand boundary */
        tw = tleft;   
        aw = 0.5*(t0 + tw);                 
        dw = pow(aw, beta);
        fw = dw*(t0 - tw);

	te = x[row + 1];
        ae = 0.5*(t0 + te);
        de = pow(ae, beta);
        fe = de*(te - t0);

	if (j > 0) {
	  ts = x[row - Xm];
          as = 0.5*(t0 + ts);
          ds = pow(as, beta);
          fs = ds*(t0 - ts);
	} else {
 	  fs = zero;
	}

	if (j < my-1) { 
          tn = x[row + Xm];  
          an = 0.5*(t0 + tn);
          dn = pow(an, beta);
	  fn = dn*(tn - t0);
	} else {
	  fn = zero; 
	}

      } else if (i == mx-1) {

        /* right-hand boundary */ 
        tw = x[row - 1];   
        aw = 0.5*(t0 + tw);
        dw = pow(aw, beta);
        fw = dw*(t0 - tw);
 
        te = tright;
        ae = 0.5*(t0 + te);
        de = pow(ae, beta);
        fe = de*(te - t0);
 
        if (j > 0) { 
          ts = x[row - Xm];
          as = 0.5*(t0 + ts);
          ds = pow(as, beta);
          fs = ds*(t0 - ts);
        } else {
          fs = zero;
        }
 
        if (j < my-1) {
          tn = x[row + Xm];
          an = 0.5*(t0 + tn);
          dn = pow(an, beta);
          fn = dn*(tn - t0); 
        } else {   
          fn = zero; 
        }

      } else if (j == 0) {

	/* bottom boundary, and i <> 0 or mx-1 */
        tw = x[row - 1];
        aw = 0.5*(t0 + tw);
        dw = pow(aw, beta);
        fw = dw*(t0 - tw);

        te = x[row + 1];
        ae = 0.5*(t0 + te);
        de = pow(ae, beta);
        fe = de*(te - t0);

        fs = zero;

        tn = x[row + Xm];
        an = 0.5*(t0 + tn);
        dn = pow(an, beta);
        fn = dn*(tn - t0);

      } else if (j == my-1) {

	/* top boundary, and i <> 0 or mx-1 */ 
        tw = x[row - 1];
        aw = 0.5*(t0 + tw);
        dw = pow(aw, beta);
        fw = dw*(t0 - tw);

        te = x[row + 1];
        ae = 0.5*(t0 + te);
        de = pow(ae, beta);
        fe = de*(te - t0);

        ts = x[row - Xm];
        as = 0.5*(t0 + ts);
        ds = pow(as, beta);
        fs = ds*(t0 - ts);

        fn = zero;

      }

      f[row] = - hydhx*(fe-fw) - hxdhy*(fn-fs); 

    }
  }
  ierr = VecRestoreArray(localX,&x); CHKERRQ(ierr);
  ierr = VecRestoreArray(localF,&f); CHKERRQ(ierr);

  /* Insert values into global vector */
  ierr = DALocalToGlobal(user->grid[1].da,localF,INSERT_VALUES,F); CHKERRQ(ierr);
  PLogFlops(11*ym*xm);
  return 0; 
} 

#undef __FUNC__
#define __FUNC__ "FormJacobian_Grid"
int FormJacobian_Grid(AppCtx *user,GridCtx *grid,Vec X, Mat *J,Mat *B)
{
  Mat     jac = *J;
  int     ierr, i, j, row, mx, my, xs, ys, xm, ym, Xs, Ys, Xm, Ym, col[5];
  int     nloc, *ltog, grow;
  double  zero = 0.0, half = 0.5, one = 1.0;
  double  hx, hy, hxdhy, hydhx, value;
  double  t0, tn, ts, te, tw; 
  double  dn, ds, de, dw, an, as, ae, aw, bn, bs, be, bw, gn, gs, ge, gw;
  double  tleft, tright, beta, bm1, coef;
  Scalar  v[5], *x;
  Vec     localX = grid->localX;

  mx = grid->mx;            my = grid->my; 
  hx = one/(double)(mx-1);  hy = one/(double)(my-1);
  hxdhy = hx/hy;            hydhx = hy/hx;
  tleft = user->tleft;      tright = user->tright;
  beta = user->beta;	    bm1 = user->bm1;		coef = user->coef;

  /* Get ghost points */
  ierr = DAGlobalToLocalBegin(grid->da,X,INSERT_VALUES,localX); CHKERRQ(ierr);
  ierr = DAGlobalToLocalEnd(grid->da,X,INSERT_VALUES,localX); CHKERRQ(ierr);
  ierr = DAGetCorners(grid->da,&xs,&ys,0,&xm,&ym,0); CHKERRQ(ierr);
  ierr = DAGetGhostCorners(grid->da,&Xs,&Ys,0,&Xm,&Ym,0); CHKERRQ(ierr);
  ierr = DAGetGlobalIndices(grid->da,&nloc,&ltog); CHKERRQ(ierr);
  ierr = VecGetArray(localX,&x); CHKERRQ(ierr);

  /* Evaluate Jacobian of function */
  for (j=ys; j<ys+ym; j++) {
    row = (j - Ys)*Xm + xs - Xs - 1; 
    for (i=xs; i<xs+xm; i++) {
      row++;
      grow = ltog[row];
      t0 = x[row];

      if (i > 0 && i < mx-1 && j > 0 && j < my-1) {

        /* general interior volume */

        tw = x[row - 1];   
        aw = 0.5*(t0 + tw);                 
        bw = pow(aw, bm1);
	/* dw = bw * aw */
	dw = pow(aw, beta); 
	gw = coef*bw*(t0 - tw);

        te = x[row + 1];
        ae = 0.5*(t0 + te);
        be = pow(ae, bm1);
	/* de = be * ae; */
	de = pow(ae, beta);
        ge = coef*be*(te - t0);

        ts = x[row - Xm];
        as = 0.5*(t0 + ts);
        bs = pow(as, bm1);
	/* ds = bs * as; */
	ds = pow(as, beta);
        gs = coef*bs*(t0 - ts);
  
        tn = x[row + Xm];  
        an = 0.5*(t0 + tn);
        bn = pow(an, bm1);
	/* dn = bn * an; */
	dn = pow(an, beta);
        gn = coef*bn*(tn - t0);

	col[0] = ltog[row - Xm];
        v[0] = - hxdhy*(ds - gs); 
	col[1] = ltog[row - 1];
        v[1] = - hydhx*(dw - gw); 
        col[2] = grow;
        v[2] = hxdhy*(ds + dn + gs - gn) + hydhx*(dw + de + gw - ge); 
	col[3] = ltog[row + 1];
        v[3] = - hydhx*(de + ge); 
	col[4] = ltog[row + Xm];
        v[4] = - hxdhy*(dn + gn); 
        ierr = MatSetValues(jac,1,&grow,5,col,v,INSERT_VALUES); CHKERRQ(ierr);

      } else if (i == 0) {

        /* left-hand boundary */
        tw = tleft;
        aw = 0.5*(t0 + tw);                  
        bw = pow(aw, bm1); 
	/* dw = bw * aw */
	dw = pow(aw, beta); 
        gw = coef*bw*(t0 - tw); 
 
        te = x[row + 1]; 
        ae = 0.5*(t0 + te); 
        be = pow(ae, bm1); 
	/* de = be * ae; */
	de = pow(ae, beta);
        ge = coef*be*(te - t0);
 
	/* left-hand bottom boundary */
	if (j == 0) {

          tn = x[row + Xm];   
          an = 0.5*(t0 + tn); 
          bn = pow(an, bm1); 
	  /* dn = bn * an; */
	  dn = pow(an, beta);
          gn = coef*bn*(tn - t0); 
          
          col[0] = grow;
          v[0] = hxdhy*(dn - gn) + hydhx*(dw + de + gw - ge); 
          col[1] = ltog[row + 1];
          v[1] = - hydhx*(de + ge); 
          col[2] = ltog[row + Xm];
          v[2] = - hxdhy*(dn + gn); 
          ierr = MatSetValues(jac,1,&grow,3,col,v,INSERT_VALUES); CHKERRQ(ierr);
 
	/* left-hand interior boundary */
	} else if (j < my-1) {

          ts = x[row - Xm];    
          as = 0.5*(t0 + ts); 
          bs = pow(as, bm1);  
	  /* ds = bs * as; */
	  ds = pow(as, beta);
          gs = coef*bs*(t0 - ts);  
          
          tn = x[row + Xm];    
          an = 0.5*(t0 + tn); 
          bn = pow(an, bm1);  
	  /* dn = bn * an; */
	  dn = pow(an, beta);
          gn = coef*bn*(tn - t0);  
          
          col[0] = ltog[row - Xm];
          v[0] = - hxdhy*(ds - gs); 
          col[1] = grow; 
          v[1] = hxdhy*(ds + dn + gs - gn) + hydhx*(dw + de + gw - ge);  
          col[2] = ltog[row + 1]; 
          v[2] = - hydhx*(de + ge);  
          col[3] = ltog[row + Xm]; 
          v[3] = - hxdhy*(dn + gn);  
          ierr = MatSetValues(jac,1,&grow,4,col,v,INSERT_VALUES); CHKERRQ(ierr);  
	/* left-hand top boundary */
	} else {

          ts = x[row - Xm];
          as = 0.5*(t0 + ts);
          bs = pow(as, bm1);
	  /* ds = bs * as; */
	  ds = pow(as, beta);
          gs = coef*bs*(t0 - ts);
          
          col[0] = ltog[row - Xm]; 
          v[0] = - hxdhy*(ds - gs);  
          col[1] = grow; 
          v[1] = hxdhy*(ds + gs) + hydhx*(dw + de + gw - ge);  
          col[2] = ltog[row + 1];  
          v[2] = - hydhx*(de + ge); 
          ierr = MatSetValues(jac,1,&grow,3,col,v,INSERT_VALUES); CHKERRQ(ierr); 
	}

      } else if (i == mx-1) {
 
        /* right-hand boundary */
        tw = x[row - 1];
        aw = 0.5*(t0 + tw);                  
        bw = pow(aw, bm1); 
	/* dw = bw * aw */
	dw = pow(aw, beta); 
        gw = coef*bw*(t0 - tw); 
 
        te = tright; 
        ae = 0.5*(t0 + te); 
        be = pow(ae, bm1); 
	/* de = be * ae; */
	de = pow(ae, beta);
        ge = coef*be*(te - t0);
 
	/* right-hand bottom boundary */
	if (j == 0) {

          tn = x[row + Xm];   
          an = 0.5*(t0 + tn); 
          bn = pow(an, bm1); 
	  /* dn = bn * an; */
	  dn = pow(an, beta);
          gn = coef*bn*(tn - t0); 
          
          col[0] = ltog[row - 1];
          v[0] = - hydhx*(dw - gw); 
          col[1] = grow;
          v[1] = hxdhy*(dn - gn) + hydhx*(dw + de + gw - ge); 
          col[2] = ltog[row + Xm];
          v[2] = - hxdhy*(dn + gn); 
          ierr = MatSetValues(jac,1,&grow,3,col,v,INSERT_VALUES); CHKERRQ(ierr);
 
	/* right-hand interior boundary */
	} else if (j < my-1) {

          ts = x[row - Xm];    
          as = 0.5*(t0 + ts); 
          bs = pow(as, bm1);  
	  /* ds = bs * as; */
	  ds = pow(as, beta);
          gs = coef*bs*(t0 - ts);  
          
          tn = x[row + Xm];    
          an = 0.5*(t0 + tn); 
          bn = pow(an, bm1);  
	  /* dn = bn * an; */
	  dn = pow(an, beta);
          gn = coef*bn*(tn - t0);  
          
          col[0] = ltog[row - Xm];
          v[0] = - hxdhy*(ds - gs); 
          col[1] = ltog[row - 1]; 
          v[1] = - hydhx*(dw - gw);  
          col[2] = grow; 
          v[2] = hxdhy*(ds + dn + gs - gn) + hydhx*(dw + de + gw - ge);  
          col[3] = ltog[row + Xm]; 
          v[3] = - hxdhy*(dn + gn);  
          ierr = MatSetValues(jac,1,&grow,4,col,v,INSERT_VALUES); CHKERRQ(ierr);  
	/* right-hand top boundary */
	} else {

          ts = x[row - Xm];
          as = 0.5*(t0 + ts);
          bs = pow(as, bm1);
	  /* ds = bs * as; */
	  ds = pow(as, beta);
          gs = coef*bs*(t0 - ts);
          
          col[0] = ltog[row - Xm]; 
          v[0] = - hxdhy*(ds - gs);  
          col[1] = ltog[row - 1];  
          v[1] = - hydhx*(dw - gw); 
          col[2] = grow; 
          v[2] = hxdhy*(ds + gs) + hydhx*(dw + de + gw - ge);  
          ierr = MatSetValues(jac,1,&grow,3,col,v,INSERT_VALUES); CHKERRQ(ierr); 
	}

      /* bottom boundary, and i <> 0 or mx-1 */
      } else if (j == 0) {

        tw = x[row - 1];
        aw = 0.5*(t0 + tw);
        bw = pow(aw, bm1);
	/* dw = bw * aw */
	dw = pow(aw, beta); 
        gw = coef*bw*(t0 - tw);

        te = x[row + 1];
        ae = 0.5*(t0 + te);
        be = pow(ae, bm1);
	/* de = be * ae; */
	de = pow(ae, beta);
        ge = coef*be*(te - t0);

        tn = x[row + Xm];
        an = 0.5*(t0 + tn);
        bn = pow(an, bm1);
	/* dn = bn * an; */
	dn = pow(an, beta);
        gn = coef*bn*(tn - t0);
 
        col[0] = ltog[row - 1];
        v[0] = - hydhx*(dw - gw);
        col[1] = grow;
        v[1] = hxdhy*(dn - gn) + hydhx*(dw + de + gw - ge);
        col[2] = ltog[row + 1];
        v[2] = - hydhx*(de + ge);
        col[3] = ltog[row + Xm];
        v[3] = - hxdhy*(dn + gn);
        ierr = MatSetValues(jac,1,&grow,4,col,v,INSERT_VALUES); CHKERRQ(ierr);
 
      /* top boundary, and i <> 0 or mx-1 */
      } else if (j == my-1) {
 
        tw = x[row - 1];
        aw = 0.5*(t0 + tw);
        bw = pow(aw, bm1);
	/* dw = bw * aw */
	dw = pow(aw, beta); 
        gw = coef*bw*(t0 - tw);

        te = x[row + 1];
        ae = 0.5*(t0 + te);
        be = pow(ae, bm1);
	/* de = be * ae; */
	de = pow(ae, beta);
        ge = coef*be*(te - t0);

        ts = x[row - Xm];
        as = 0.5*(t0 + ts);
        bs = pow(as, bm1);
 	/* ds = bs * as; */
	ds = pow(as, beta);
        gs = coef*bs*(t0 - ts);

        col[0] = ltog[row - Xm];
        v[0] = - hxdhy*(ds - gs);
        col[1] = ltog[row - 1];
        v[1] = - hydhx*(dw - gw);
        col[2] = grow;
        v[2] = hxdhy*(ds + gs) + hydhx*(dw + de + gw - ge);
        col[3] = ltog[row + 1];
        v[3] = - hydhx*(de + ge);
        ierr = MatSetValues(jac,1,&grow,4,col,v,INSERT_VALUES); CHKERRQ(ierr);
 
      }
    }
  }
  ierr = MatAssemblyBegin(jac,MAT_FINAL_ASSEMBLY); CHKERRQ(ierr);
  ierr = VecRestoreArray(localX,&x); CHKERRQ(ierr);
  ierr = MatAssemblyEnd(jac,MAT_FINAL_ASSEMBLY); CHKERRQ(ierr);

  return 0;
}

/* --------------------  Evaluate Jacobian F'(x) --------------------- */
#undef __FUNC__
#define __FUNC__ "FormJacobian"
int FormJacobian(SNES snes,Vec X,Mat *J,Mat *B,MatStructure *flag,void *ptr)
{
  AppCtx  *user = (AppCtx *) ptr;
  int     ierr;
  SLES    sles;
  PC      pc;

  *flag = SAME_NONZERO_PATTERN;
  ierr = FormJacobian_Grid(user,&user->grid[1],X,J,B); CHKERRQ(ierr);

  /* create coarse grid jacobian for preconditioner */
  ierr = SNESGetSLES(snes,&sles);CHKERRQ(ierr);
  ierr = SLESGetPC(sles,&pc);CHKERRQ(ierr);
  if (PetscTypeCompare(pc,PCMG)) {

    ierr = SLESSetOperators(user->grid[1].sles,user->grid[1].J,user->grid[1].J,SAME_NONZERO_PATTERN);CHKERRA(ierr);

    /* restrict X to coarse grid */
    ierr = MGRestrict(user->grid[0].R,X,user->grid[0].x);CHKERRQ(ierr);
    ierr = VecPointwiseMult(user->grid[0].Rscale,user->grid[0].x,user->grid[0].x);CHKERRQ(ierr);
    /* form Jacobian on coarse grid */
    ierr = FormJacobian_Grid(user,&user->grid[0],user->grid[0].x,&user->grid[0].J,&user->grid[0].J);CHKERRQ(ierr);
    
    ierr = SLESSetOperators(user->grid[0].sles,user->grid[0].J,user->grid[0].J,SAME_NONZERO_PATTERN);CHKERRA(ierr);

  }

  return 0;
}

#undef __FUNC__
#define __FUNC__ "FormInterpolation"
/*
      Forms the interpolation (and restriction) operator from 
coarse grid to grid[1].
*/
int FormInterpolation(AppCtx *user)
{
  int      ierr,i,j,i_start,m_fine,j_start,m,n,M,Mx = user->grid[0].mx,My = user->grid[0].my,*idx;
  int      m_ghost,n_ghost,*idx_c,m_ghost_c,n_ghost_c,m_coarse;
  int      row,col,i_start_ghost,j_start_ghost,cols[4],mx = user->grid[1].mx, m_c,my = user->grid[1].my;
  int      c0,c1,c2,c3,nc,ratio = user->ratio,i_end,i_end_ghost,m_c_local,m_fine_local;
  int      i_c,j_c,i_start_c,j_start_c,n_c,i_start_ghost_c,j_start_ghost_c;
  Scalar   v[4],x,y, one = 1.0;
  Mat      mat;
  Vec	   Rscale; 

  ierr = DAGetCorners(user->grid[1].da,&i_start,&j_start,0,&m,&n,0);CHKERRQ(ierr);
  ierr = DAGetGhostCorners(user->grid[1].da,&i_start_ghost,&j_start_ghost,0,&m_ghost,&n_ghost,0);CHKERRQ(ierr);
  ierr = DAGetGlobalIndices(user->grid[1].da,PETSC_NULL,&idx); CHKERRQ(ierr);

  ierr = DAGetCorners(user->grid[0].da,&i_start_c,&j_start_c,0,&m_c,&n_c,0);CHKERRQ(ierr);
  ierr = DAGetGhostCorners(user->grid[0].da,&i_start_ghost_c,&j_start_ghost_c,0,&m_ghost_c,&n_ghost_c,0);CHKERRQ(ierr);
  ierr = DAGetGlobalIndices(user->grid[0].da,PETSC_NULL,&idx_c); CHKERRQ(ierr);

  /* create interpolation matrix */
  ierr = VecGetLocalSize(user->grid[1].x,&m_fine_local);CHKERRQ(ierr);
  ierr = VecGetLocalSize(user->grid[0].x,&m_c_local);CHKERRQ(ierr);
  ierr = VecGetSize(user->grid[1].x,&m_fine);CHKERRQ(ierr);
  ierr = VecGetSize(user->grid[0].x,&m_coarse);CHKERRQ(ierr);
  ierr = MatCreateMPIAIJ(PETSC_COMM_WORLD,m_fine_local,m_c_local,m_fine,m_coarse,
                         5,0,3,0,&mat);CHKERRQ(ierr);

  /* loop over local fine grid nodes setting interpolation for those*/
  for ( j=j_start; j<j_start+n; j++ ) {
    for ( i=i_start; i<i_start+m; i++ ) {
      /* convert to local "natural" numbering and 
         then to PETSc global numbering */
      row    = idx[m_ghost*(j-j_start_ghost) + (i-i_start_ghost)];

      i_c = (i/ratio);    /* coarse grid node to left of fine grid node */
      j_c = (j/ratio);    /* coarse grid node below fine grid node */

      /* 
         Only include those interpolation points that are truly 
         nonzero. Note this is very important for final grid lines
         in x and y directions; since they have no right/top neighbors
      */
      x  = ((double)(i - i_c*ratio))/((double)ratio);
      y  = ((double)(j - j_c*ratio))/((double)ratio);
      /* printf("i j %d %d %g %g\n",i,j,x,y); */
      nc = 0;
      /* one left and below; or we are right on it */
      if (j_c < j_start_ghost_c || j_c > j_start_ghost_c+n_ghost_c) {
        SETERRQ3(1,1,"Sorry j %d %d %d",j_c,j_start_ghost_c,j_start_ghost_c+n_ghost_c);
      }
      if (i_c < i_start_ghost_c || i_c > i_start_ghost_c+m_ghost_c) {
        SETERRQ3(1,1,"Sorry i %d %d %d",i_c,i_start_ghost_c,i_start_ghost_c+m_ghost_c);
      }
      col      = m_ghost_c*(j_c-j_start_ghost_c) + (i_c-i_start_ghost_c);
      cols[nc] = idx_c[col]; 
      v[nc++]  = x*y - x - y + 1.0;
      /* one right and below */
      if (i_c*ratio != i) { 
        cols[nc] = idx_c[col+1];
        v[nc++]  = -x*y + x;
      }
      /* one left and above */
      if (j_c*ratio != j) { 
        cols[nc] = idx_c[col+m_ghost_c];
        v[nc++]  = -x*y + y;
      }
      /* one right and above */
      if (j_c*ratio != j && i_c*ratio != i) { 
        cols[nc] = idx_c[col+m_ghost_c+1];
        v[nc++]  = x*y;
      }
      ierr = MatSetValues(mat,1,&row,nc,cols,v,INSERT_VALUES); CHKERRQ(ierr); 
    }
  }
  ierr = MatAssemblyBegin(mat,MAT_FINAL_ASSEMBLY); CHKERRQ(ierr);
  ierr = MatAssemblyEnd(mat,MAT_FINAL_ASSEMBLY); CHKERRQ(ierr);

  ierr = VecDuplicate(user->grid[0].x,&Rscale);CHKERRQ(ierr);
  ierr = VecSet(&one,user->grid[1].x);CHKERRQ(ierr);
  ierr = MGRestrict(mat,user->grid[1].x,Rscale);CHKERRQ(ierr);
  ierr = VecReciprocal(Rscale);CHKERRQ(ierr);
  user->grid[0].Rscale = Rscale;
  user->grid[0].R      = mat;
  return 0;
}

