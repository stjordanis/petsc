#ifndef lint
static char vcid[] = "$Id: lsqr.c,v 1.2 1994/08/21 23:56:49 bsmith Exp $";
#endif

#define SWAP(a,b,c) { c = a; a = b; b = c; }

/*                       
       This implements LSQR (Paige and Saunders, ACM Transactions on
       Mathematical Software, Vol 8, pp 43-71, 1982).

       and add the extern declarations of the create, solve, and setup 
       routines to 

*/
#include <stdio.h>
#include <math.h>
#include "petsc.h"
#include "kspimpl.h"

static int KSPiLSQRSetUp();
static int  KSPiLSQRSolve();

int KSPiLSQRCreate(itP)
KSP itP;
{
itP->MethodPrivate        = (void *) 0;
itP->method               = KSPLSQR;
itP->right_pre            = 0;
itP->calc_res             = 1;
itP->setup                = KSPiLSQRSetUp;
itP->solver               = KSPiLSQRSolve;
itP->adjustwork           = KSPiDefaultAdjustWork;
itP->destroy              = KSPiDefaultDestroy;
return 0;
}

static int KSPiLSQRSetUp(itP)
KSP itP;
{
  int ierr;
if (ierr = KSPCheckDef( itP )) return ierr;
if (!itP->tamult) {
  SETERR(1,"LSQR requires matrix-transpose * vector");
}
ierr = KSPiDefaultGetWork( itP,  6 );
return ierr;
}


static int KSPiLSQRSolve(itP,its)
KSP itP;
int    *its;
{
int       i = 0, maxit, res, pres, hist_len, cerr;
double    rho, rhobar, phi, phibar, theta, c, s, beta, alpha, rnorm, *history;
double    tmp, zero = 0.0;
Vec       X,B,V,V1,U,U1,TMP,W,BINVF;

res     = itP->calc_res;
pres    = itP->use_pres;
maxit   = itP->max_it;
history = itP->residual_history;
hist_len= itP->res_hist_size;
X       = itP->vec_sol;
B       = itP->vec_rhs;
U       = itP->work[0];
U1      = itP->work[1];
V       = itP->work[2];
V1      = itP->work[3];
W       = itP->work[4];
BINVF   = itP->work[5];

/* Compute initial preconditioned residual */
KSPResidual(itP,X,V,U, W, BINVF, B );

/* Test for nothing to do */
VecNorm(W,&rnorm);
if (CONVERGED(itP,rnorm,0)) { *its = 0; return 0;}
MONITOR(itP,rnorm,0);
if (history) history[0] = rnorm;

VecCopy(B,U);
VecNorm(U,&beta);
tmp = 1.0/beta; VecScale( &tmp, U );
TMM(itP,  U, V );
VecNorm(V,&alpha);
tmp = 1.0/alpha; VecScale(&tmp, V );

VecCopy(V,W);
VecSet(&zero,X);

phibar = beta;
rhobar = alpha;
for (i=0; i<maxit; i++) {
    MM(itP,V,U1);
    tmp = -alpha; VecAXPY(&tmp,U,U1);
    VecNorm(U1,&beta);
    tmp = 1.0/beta; VecScale(&tmp, U1 );

    TMM(itP,U1,V1);
    tmp = -beta; VecAXPY(&tmp,V,V1);
    VecNorm(V1,&alpha);
    tmp = 1.0 / alpha; VecScale(&tmp , V1 );

    rho   = sqrt(rhobar*rhobar + beta*beta);
    c     = rhobar / rho;
    s     = beta / rho;
    theta = s * alpha;
    rhobar= - c * alpha;
    phi   = c * phibar;
    phibar= s * phibar;

    tmp = phi/rho; VecAXPY(&tmp,W,X);      /*    x <- x + (phi/rho) w   */
    tmp = -theta/rho; VecAYPX(&tmp,V1,W);  /*    w <- v - (theta/rho) w */

    rnorm = phibar;

    if (history && hist_len > i + 1) history[i+1] = rnorm;
    MONITOR(itP,rnorm,i+1);
    if (CONVERGED(itP,rnorm,i+1)) break;
    SWAP( U1, U, TMP );
    SWAP( V1, V, TMP );
    }
if (i == maxit) i--;
if (history) itP->res_act_size = (hist_len < i + 1) ? hist_len : i + 1;

KSPUnwindPre(  itP, X, W );
*its = RCONV(itP,i+1); return 0;
}
