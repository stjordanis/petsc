#define PETSCMAT_DLL

/*
    Provides an implementation of the Unevenly Sampled FFT algorithm as a Mat.
    Testing examples can be found in ~/src/mat/examples/tests
*/

#include "private/matimpl.h"          /*I "petscmat.h" I*/
#include "petscda.h"                  /*I "petscda.h"  I*/ /* Unlike equispaced FFT, USFFT requires geometric information encoded by a DA */
#include "fftw3.h"

typedef struct {
  DA             inda;
  DA             outda;
  PetscInt       ndim;
  PetscInt       *indim;
  PetscInt       *outdim;
  PetscInt       m,n,M,N;
  PetscInt       dof;
  fftw_plan      p_forward,p_backward;
  unsigned       p_flag; /* planner flags, FFTW_ESTIMATE,FFTW_MEASURE, FFTW_PATIENT, FFTW_EXHAUSTIVE */
} Mat_USFFT;


#undef __FUNCT__  
#define __FUNCT__ "MatApply_USFFT_Private"
PetscErrorCode MatApply_USFFT_Private(Mat_USFFT *usfft, fftw_plan *plan, int direction, Vec x,Vec y)
{
  PetscErrorCode ierr;
  PetscScalar    *x_array, *y_array;

  PetscFunctionBegin;
  /* NB: for now we use outdim for both x and y; this will change once a full USFFT is implemented */
  ierr = VecGetArray(x,&x_array);CHKERRQ(ierr);
  ierr = VecGetArray(y,&y_array);CHKERRQ(ierr);
  if (!*plan){ /* create a plan then execute it*/
    if(usfft->dof == 1) {
#ifdef PETSC_DEBUG_USFFT
      ierr = PetscPrintf(PETSC_COMM_WORLD, "direction = %d, usfft->ndim = %d\n", direction, usfft->ndim); CHKERRQ(ierr);
      for(int ii = 0; ii < usfft->ndim; ++ii) {
        ierr = PetscPrintf(PETSC_COMM_WORLD, "usfft->outdim[%d] = %d\n", ii, usfft->outdim[ii]); CHKERRQ(ierr);
      }
#endif 
#if 0
      *plan = fftw_plan_dft(usfft->ndim,usfft->outdim,(fftw_complex*)x_array,(fftw_complex*)y_array,direction,usfft->p_flag);
#endif
      switch (usfft->ndim){
      case 1:
        *plan = fftw_plan_dft_1d(usfft->outdim[0],(fftw_complex*)x_array,(fftw_complex*)y_array,direction,usfft->p_flag);   
        break;
      case 2:
        *plan = fftw_plan_dft_2d(usfft->outdim[0],usfft->outdim[1],(fftw_complex*)x_array,(fftw_complex*)y_array,direction,usfft->p_flag);
        break;
      case 3:
        *plan = fftw_plan_dft_3d(usfft->outdim[0],usfft->outdim[1],usfft->outdim[2],(fftw_complex*)x_array,(fftw_complex*)y_array,direction,usfft->p_flag);
        break;
      default:
        *plan = fftw_plan_dft(usfft->ndim,usfft->outdim,(fftw_complex*)x_array,(fftw_complex*)y_array,direction,usfft->p_flag);
        break;
      }
      fftw_execute(*plan);
    }/* if(dof == 1) */
    else { /* if(dof > 1) */
      *plan = fftw_plan_many_dft(/*rank*/usfft->ndim, /*n*/usfft->outdim, /*howmany*/usfft->dof,
                                 (fftw_complex*)x_array, /*nembed*/usfft->outdim, /*stride*/usfft->dof, /*dist*/1,
                                 (fftw_complex*)y_array, /*nembed*/usfft->outdim, /*stride*/usfft->dof, /*dist*/1,
                                 /*sign*/direction, /*flags*/usfft->p_flag); 
      fftw_execute(*plan);
    }/* if(dof > 1) */
  }/* if(!*plan) */ 
  else { /* if(*plan) */
    /* use existing plan */
    fftw_execute_dft(*plan,(fftw_complex*)x_array,(fftw_complex*)y_array);
  }
  ierr = VecRestoreArray(y,&y_array);CHKERRQ(ierr);
  ierr = VecRestoreArray(x,&x_array);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}/* MatApply_FFTW_Private() */


#undef __FUNCT__  
#define __FUNCT__ "MatMult_SeqUSFFT"
PetscErrorCode MatMult_SeqUSFFT(Mat A,Vec x,Vec y)
{
  PetscErrorCode ierr;
  Mat_USFFT       *usfft = (Mat_USFFT*)A->data;

  PetscFunctionBegin;
  /* NB: for now we use outdim for both x and y; this will change once a full USFFT is implemented */
  ierr = MatApply_USFFT_Private(usfft, &usfft->p_forward, FFTW_FORWARD, x,y); CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatMultTranspose_SeqUSFFT"
PetscErrorCode MatMultTranspose_SeqUSFFT(Mat A,Vec x,Vec y)
{
  PetscErrorCode ierr;
  Mat_USFFT       *usfft = (Mat_USFFT*)A->data;
  PetscFunctionBegin;
  /* NB: for now we use outdim for both x and y; this will change once a full USFFT is implemented */
  ierr = MatApply_USFFT_Private(usfft, &usfft->p_backward, FFTW_BACKWARD, x,y); CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "MatDestroy_SeqUSFFT"
PetscErrorCode MatDestroy_SeqUSFFT(Mat A)
{
  Mat_USFFT       *usfft = (Mat_USFFT*)A->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;  
  fftw_destroy_plan(usfft->p_forward);
  fftw_destroy_plan(usfft->p_backward);
  ierr = PetscFree(usfft->indim);CHKERRQ(ierr);
  ierr = PetscFree(usfft->outdim);CHKERRQ(ierr);
  ierr = PetscFree(usfft);CHKERRQ(ierr);
  ierr = PetscObjectChangeTypeName((PetscObject)A,0);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}/* MatDestroy_SeqUSFFT() */


#undef __FUNCT__  
#define __FUNCT__ "MatCreateSeqUSFFT"
/*@
      MatCreateSeqUSFFT - Creates a matrix object that provides sequential USFFT
  via the external package FFTW

   Collective on MPI_Comm

   Input Parameter:
+   da - geometry of the domain encoded by a DA

   Output Parameter:
.   A  - the matrix

  Options Database Keys:
+ -mat_usfft_plannerflags - set the FFTW planner flags

   Level: intermediate
   
@*/
PetscErrorCode PETSCMAT_DLLEXPORT MatCreateSeqUSFFT(DA inda, DA outda, Mat* A)
{
  PetscErrorCode ierr;
  Mat_USFFT      *usfft;
  PetscInt       m,n,M,N,i;
  const char     *p_flags[]={"FFTW_ESTIMATE","FFTW_MEASURE","FFTW_PATIENT","FFTW_EXHAUSTIVE"};
  PetscTruth     flg;
  PetscInt       p_flag;
  PetscInt       dof, ndim;
  PetscInt       dim[3];
  MPI_Comm       comm;
  PetscInt       size;

  PetscFunctionBegin;
  ierr = PetscObjectGetComm((PetscObject)inda, &comm); CHKERRQ(ierr);
  ierr = MPI_Comm_size(comm, &size); CHKERRQ(ierr);
  if (size > 1) SETERRQ(PETSC_ERR_USER, "Parallel DA (in) not yet supported by USFFT"); 
  ierr = PetscObjectGetComm((PetscObject)outda, &comm); CHKERRQ(ierr);
  ierr = MPI_Comm_size(comm, &size); CHKERRQ(ierr);
  if (size > 1) SETERRQ(PETSC_ERR_USER, "Parallel DA (out) not yet supported by USFFT"); 
  ierr = MatCreate(comm,A);CHKERRQ(ierr);
  ierr = PetscNewLog(*A,Mat_USFFT,&usfft);CHKERRQ(ierr);
  (*A)->data = (void*)usfft;
  usfft->inda = inda;
  usfft->outda = outda;
  /* inda */
  ierr = DAGetInfo(usfft->inda, &ndim, dim+0, dim+1, dim+2, PETSC_NULL, PETSC_NULL, PETSC_NULL, &dof, PETSC_NULL, PETSC_NULL, PETSC_NULL); CHKERRQ(ierr);
  if (ndim <= 0) SETERRQ1(PETSC_ERR_USER,"ndim %d must be > 0",ndim);
  if (dof <= 0) SETERRQ1(PETSC_ERR_USER,"dof %d must be > 0",dof);
  usfft->ndim = ndim;
  usfft->dof = dof;
  /* Store input dimensions */
  /* NB: we reverse the DA dimensions, since the DA ordering (natural on x-y-z, with x varying the fastest) 
     is the order opposite of that assumed by FFTW: z varying the fastest */
  ierr = PetscMalloc((usfft->ndim+1)*sizeof(PetscInt),&usfft->indim);CHKERRQ(ierr);
  for(i = usfft->ndim; i > 0; --i) {
    usfft->indim[usfft->ndim-i] = dim[i-1];
  }
  /* outda */
  ierr = DAGetInfo(usfft->outda, &ndim, dim+0, dim+1, dim+2, PETSC_NULL, PETSC_NULL, PETSC_NULL, &dof, PETSC_NULL, PETSC_NULL, PETSC_NULL); CHKERRQ(ierr);
  if (ndim != usfft->ndim) SETERRQ2(PETSC_ERR_USER,"in and out DA dimensions must match: %d != %d",usfft->ndim, ndim);
  if (dof != usfft->dof) SETERRQ2(PETSC_ERR_USER,"in and out DA dof must match: %d != %d",usfft->dof, dof);
  /* Store output dimensions */
  /* NB: we reverse the DA dimensions, since the DA ordering (natural on x-y-z, with x varying the fastest) 
     is the order opposite of that assumed by FFTW: z varying the fastest */
  ierr = PetscMalloc((usfft->ndim+1)*sizeof(PetscInt),&usfft->outdim);CHKERRQ(ierr);
  for(i = usfft->ndim; i > 0; --i) {
    usfft->outdim[usfft->ndim-i] = dim[i-1];
  }
  /* mat sizes */
  m = 1; n = 1;
  for (i=0; i<usfft->ndim; i++){
    if (usfft->indim[i] <= 0) SETERRQ2(PETSC_ERR_USER,"indim[%d]=%d must be > 0",i,usfft->indim[i]);
    if (usfft->outdim[i] <= 0) SETERRQ2(PETSC_ERR_USER,"outdim[%d]=%d must be > 0",i,usfft->outdim[i]);
    n *= usfft->indim[i];
    m *= usfft->outdim[i];
  }
  N = n*usfft->dof;
  M = m*usfft->dof;
  ierr = MatSetSizes(*A,M,N,M,N);CHKERRQ(ierr);  /* "in size" is the number of columns, "out size" is the number of rows" */
  ierr = PetscObjectChangeTypeName((PetscObject)*A,MATSEQUSFFT);CHKERRQ(ierr);
  usfft->m = m; usfft->n = n; usfft->M = M; usfft->N = N;
  /* FFTW */
  usfft->p_forward  = 0;
  usfft->p_backward = 0;
  usfft->p_flag     = FFTW_ESTIMATE;
  /* set Mat ops */
  (*A)->ops->mult          = MatMult_SeqUSFFT;
  (*A)->ops->multtranspose = MatMultTranspose_SeqUSFFT;
  (*A)->assembled          = PETSC_TRUE;
  (*A)->ops->destroy       = MatDestroy_SeqUSFFT;
  /* get runtime options */
  ierr = PetscOptionsBegin(((PetscObject)(*A))->comm,((PetscObject)(*A))->prefix,"USFFT Options","Mat");CHKERRQ(ierr);
  ierr = PetscOptionsEList("-mat_usfft_fftw_plannerflags","Planner Flags","None",p_flags,4,p_flags[0],&p_flag,&flg);CHKERRQ(ierr);
  if (flg) {usfft->p_flag = (unsigned)p_flag;}
  PetscOptionsEnd();
  PetscFunctionReturn(0);
}/* MatCreateSeqUSFFT() */

