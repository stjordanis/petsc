#ifdef PETSC_RCS_HEADER
static char vcid[] = "$Id: vinv.c,v 1.52 1999/05/12 03:28:02 bsmith Exp bsmith $";
#endif
/*
     Some useful vector utility functions.
*/
#include "vec.h"                 /*I "vec.h" I*/
#include "src/vec/vecimpl.h"

#undef __FUNC__  
#define __FUNC__ "VecStrideNorm"
/*@C
   VecStrideNorm - Computes the norm of subvector of a vector defined 
   by a starting point and a stride.

   Collective on Vec

   Input Parameter:
+  v - the vector 
.  start - starting point of the subvector (defined by a stride)
-  ntype - type of norm, one of NORM_1, NORM_2, NORM_INFINITY

   Output Parameter:
.  norm - the norm

   Notes:
   One must call VecSetBlockSize() before this routine to set the stride 
   information.

   If x is the array representing the vector x then this computes the norm 
   of the array (x[start],x[start+stride],x[start+2*stride], ....)

   This is useful for computing, say the norm of the pressure variable when
   the pressure is stored (interlaced) with other variables, say density etc.

   This will only work if the desire subvector is a stride subvector

   Level: advanced

.keywords: vector, subvector norm, norm

.seealso: VecNorm(), VecStrideGather(), VecStrideScatter(), VecStrideMin(), VecStrideMax()
@*/
int VecStrideNorm(Vec v,int start,NormType ntype,double *norm)
{
  int      i,n,ierr,bs;
  Scalar   *x;
  double   tnorm;
  MPI_Comm comm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(v,VEC_COOKIE);
  ierr = VecGetLocalSize(v,&n);CHKERRQ(ierr);
  ierr = VecGetArray(v,&x);CHKERRQ(ierr);
  ierr = PetscObjectGetComm((PetscObject)v,&comm);CHKERRQ(ierr);

  bs   = v->bs;
  if (start >= bs) {
    SETERRQ2(1,1,"Start of stride subvector (%d) is too large for stride\n\
            Have you set the vector blocksize (%d) correctly with VecSetBlockSize()?",start,bs);
  }
  x += start;

  if (ntype == NORM_2) {
    Scalar sum = 0.0;
    for ( i=0; i<n; i+=bs ) {
      sum += x[i]*(PetscConj(x[i]));
    }
    tnorm  = PetscReal(sum);
    ierr   = MPI_Allreduce(&tnorm,norm,1,MPI_DOUBLE,MPI_SUM,comm);CHKERRQ(ierr);
    *norm = sqrt(*norm);
  } else if (ntype == NORM_1) {
    tnorm = 0.0;
    for ( i=0; i<n; i+=bs ) {
      tnorm += PetscAbsScalar(x[i]);
    }
    ierr   = MPI_Allreduce(&tnorm,norm,1,MPI_DOUBLE,MPI_SUM,comm);CHKERRQ(ierr);
  } else if (ntype == NORM_INFINITY) {
    double tmp;
    tnorm = 0.0;

    for (i=0; i<n; i+=bs) {
      if ((tmp = PetscAbsScalar(x[i])) > tnorm) tnorm = tmp;
      /* check special case of tmp == NaN */
      if (tmp != tmp) {tnorm = tmp; break;}
    } 
    ierr   = MPI_Allreduce(&tnorm,norm,1,MPI_DOUBLE,MPI_MAX,comm);CHKERRQ(ierr);
  } else {
    SETERRQ(1,1,"Unknown norm type");
  }

  ierr = VecRestoreArray(v,&x);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNC__  
#define __FUNC__ "VecStrideMax"
/*@C
   VecStrideMax - Computes the maximum of subvector of a vector defined 
   by a starting point and a stride and optionally its location.

   Collective on Vec

   Input Parameter:
+  v - the vector 
-  start - starting point of the subvector (defined by a stride)

   Output Parameter:
+  index - the location where the maximum occurred (not supported, pass PETSC_NULL)
-  norm - the max

   Notes:
   One must call VecSetBlockSize() before this routine to set the stride 
   information; or use a vector associated with a DA.

   If xa is the array representing the vector x, then this computes the max
   of the array (xa[start],xa[start+stride],xa[start+2*stride], ....)

   This is useful for computing, say the maximum of the pressure variable when
   the pressure is stored (interlaced) with other variables, e.g., density, etc.
   This will only work if the desire subvector is a stride subvector.

   Level: advanced

.keywords: vector, subvector maximum, maximum

.seealso: VecMax(), VecStrideNorm(), VecStrideGather(), VecStrideScatter(), VecStrideMin()
@*/
int VecStrideMax(Vec v,int start,int *index,double *norm)
{
  int      i,n,ierr,bs;
  Scalar   *x;
  double   max,tmp;
  MPI_Comm comm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(v,VEC_COOKIE);
  if (index) {
    SETERRQ(1,1,"No support yet for returning index; send mail to petsc-maint@mcs.anl.gov asking for it");
  }
  ierr = VecGetLocalSize(v,&n);CHKERRQ(ierr);
  ierr = VecGetArray(v,&x);CHKERRQ(ierr);
  ierr = PetscObjectGetComm((PetscObject)v,&comm);CHKERRQ(ierr);

  bs   = v->bs;
  if (start >= bs) {
    SETERRQ2(1,1,"Start of stride subvector (%d) is too large for stride\n\
            Have you set the vector blocksize (%d) correctly with VecSetBlockSize()?",start,bs);
  }
  x += start;

  if (!n) {
    max = PETSC_MIN;
  } else {
#if defined(PETSC_USE_COMPLEX)
    max = PetscReal(x[0]);
#else
    max = x[0];
#endif
    for ( i=bs; i<n; i+=bs ) {
#if defined(PETSC_USE_COMPLEX)
      if ((tmp = PetscReal(x[i])) > max) { max = tmp;}
#else
      if ((tmp = x[i]) > max) { max = tmp; } 
#endif
    }
  }
  ierr   = MPI_Allreduce(&max,norm,1,MPI_DOUBLE,MPI_MAX,comm);CHKERRQ(ierr);

  ierr = VecRestoreArray(v,&x);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNC__  
#define __FUNC__ "VecStrideMin"
/*@C
   VecStrideMin - Computes the minimum of subvector of a vector defined 
   by a starting point and a stride and optionally its location.

   Collective on Vec

   Input Parameter:
+  v - the vector 
-  start - starting point of the subvector (defined by a stride)

   Output Parameter:
+  index - the location where the minimum occurred (not supported, pass PETSC_NULL)
-  norm - the min

   Level: intermediate

   Notes:
   One must call VecSetBlockSize() before this routine to set the stride 
   information; or use a vector associated with a DA.

   If xa is the array representing the vector x, then this computes the min
   of the array (xa[start],xa[start+stride],xa[start+2*stride], ....)

   This is useful for computing, say the minimum of the pressure variable when
   the pressure is stored (interlaced) with other variables, e.g., density, etc.
   This will only work if the desire subvector is a stride subvector.

.keywords: vector, subvector minimum, minimum

.seealso: VecMin(), VecStrideNorm(), VecStrideGather(), VecStrideScatter(), VecStrideMax()
@*/
int VecStrideMin(Vec v,int start,int *index,double *norm)
{
  int      i,n,ierr,bs;
  Scalar   *x;
  double   min,tmp;
  MPI_Comm comm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(v,VEC_COOKIE);
  if (index) {
    SETERRQ(1,1,"No support yet for returning index; send mail to petsc-maint@mcs.anl.gov asking for it");
  }
  ierr = VecGetLocalSize(v,&n);CHKERRQ(ierr);
  ierr = VecGetArray(v,&x);CHKERRQ(ierr);
  ierr = PetscObjectGetComm((PetscObject)v,&comm);CHKERRQ(ierr);

  bs   = v->bs;
  if (start >= bs) {
    SETERRQ2(1,1,"Start of stride subvector (%d) is too large for stride\n\
            Have you set the vector blocksize (%d) correctly with VecSetBlockSize()?",start,bs);
  }
  x += start;

  if (!n) {
    min = PETSC_MAX;
  } else {
#if defined(PETSC_USE_COMPLEX)
    min = PetscReal(x[0]);
#else
    min = x[0];
#endif
    for ( i=bs; i<n; i+=bs ) {
#if defined(PETSC_USE_COMPLEX)
      if ((tmp = PetscReal(x[i])) < min) { min = tmp;}
#else
      if ((tmp = x[i]) < min) { min = tmp; } 
#endif
    }
  }
  ierr   = MPI_Allreduce(&min,norm,1,MPI_DOUBLE,MPI_MIN,comm);CHKERRQ(ierr);

  ierr = VecRestoreArray(v,&x);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNC__  
#define __FUNC__ "VecStrideGather"
/*@
   VecStrideGather - Gathers a single component from a multi-component vector into
   another vector.

   Collective on Vec

   Input Parameter:
+  v - the vector 
.  start - starting point of the subvector (defined by a stride)
-  addv - one of ADD_VALUES,SET_VALUES,MAX_VALUES

   Output Parameter:
.  s - the location where the subvector is stored

   Notes:
   One must call VecSetBlockSize() before this routine to set the stride 
   information.

   If x is the array representing the vector x then this gathers
   the array (x[start],x[start+stride],x[start+2*stride], ....)

   The parallel layout of the vector and the subvector must be the same;
   i.e., nlocal of v = stride*(nlocal of s) 

   Level: advanced

.keywords: vector, subvector

.seealso: VecStrideNorm(), VecStrideScatter(), VecStrideMin(), VecStrideMax()
@*/
int VecStrideGather(Vec v,int start,Vec s,InsertMode addv)
{
  int      i,n,ierr,bs,ns;
  Scalar   *x,*y;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(v,VEC_COOKIE);
  PetscValidHeaderSpecific(s,VEC_COOKIE);
  ierr = VecGetLocalSize(v,&n);CHKERRQ(ierr);
  ierr = VecGetLocalSize(s,&ns);CHKERRQ(ierr);
  ierr = VecGetArray(v,&x);CHKERRQ(ierr);
  ierr = VecGetArray(s,&y);CHKERRQ(ierr);

  bs   = v->bs;
  if (start >= bs) {
    SETERRQ2(1,1,"Start of stride subvector (%d) is too large for stride\n\
            Have you set the vector blocksize (%d) correctly with VecSetBlockSize()?",start,bs);
  }
  if (n != ns*bs) {
    SETERRQ2(1,1,"Subvector length * blocksize %d not correct for gather from original vector %d",ns*bs,n);
  }
  x += start;
  n =  n/bs;

  if (addv == INSERT_VALUES) {
    for ( i=0; i<n; i++ ) {
      y[i] = x[bs*i];
    }
  } else if (addv == ADD_VALUES) {
    for ( i=0; i<n; i++ ) {
      y[i] += x[bs*i];
    }
#if !defined(PETSC_USE_COMPLEX)
  } else if (addv == MAX_VALUES) {
    for ( i=0; i<n; i++ ) {
      y[i] = PetscMax(y[i],x[bs*i]);
    }
#endif
  } else {
    SETERRQ(1,1,"Unknown insert type");
  }

  ierr = VecRestoreArray(v,&x);CHKERRQ(ierr);
  ierr = VecRestoreArray(s,&y);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNC__  
#define __FUNC__ "VecStrideScatter"
/*@
   VecStrideScatter - Scatters a single component from a vector into a multi-component vector.

   Collective on Vec

   Input Parameter:
+  s - the single-component vector 
.  start - starting point of the subvector (defined by a stride)
-  addv - one of ADD_VALUES,SET_VALUES,MAX_VALUES

   Output Parameter:
.  v - the location where the subvector is scattered (the multi-component vector)

   Notes:
   One must call VecSetBlockSize() on the multi-component vector before this
   routine to set the stride  information.

   The parallel layout of the vector and the subvector must be the same;
   i.e., nlocal of v = stride*(nlocal of s) 

   Level: advanced

.keywords: vector, subvector

.seealso: VecStrideNorm(), VecStrideGather(), VecStrideMin(), VecStrideMax()
@*/
int VecStrideScatter(Vec s,int start,Vec v,InsertMode addv)
{
  int      i,n,ierr,bs,ns;
  Scalar   *x,*y;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(v,VEC_COOKIE);
  PetscValidHeaderSpecific(s,VEC_COOKIE);
  ierr = VecGetLocalSize(v,&n);CHKERRQ(ierr);
  ierr = VecGetLocalSize(s,&ns);CHKERRQ(ierr);
  ierr = VecGetArray(v,&x);CHKERRQ(ierr);
  ierr = VecGetArray(s,&y);CHKERRQ(ierr);

  bs   = v->bs;
  if (start >= bs) {
    SETERRQ2(1,1,"Start of stride subvector (%d) is too large for stride\n\
            Have you set the vector blocksize (%d) correctly with VecSetBlockSize()?",start,bs);
  }
  if (n != ns*bs) {
    SETERRQ2(1,1,"Subvector length * blocksize %d not correct for scatter to multicomponent vector %d",ns*bs,n);
  }
  x += start;
  n =  n/bs;


  if (addv == INSERT_VALUES) {
    for ( i=0; i<n; i++ ) {
      x[bs*i] = y[i];
    }
  } else if (addv == ADD_VALUES) {
    for ( i=0; i<n; i++ ) {
      x[bs*i] += y[i];
    }
#if !defined(PETSC_USE_COMPLEX)
  } else if (addv == MAX_VALUES) {
    for ( i=0; i<n; i++ ) {
      x[bs*i] = PetscMax(y[i],x[bs*i]);
    }
#endif
  } else {
    SETERRQ(1,1,"Unknown insert type");
  }


  ierr = VecRestoreArray(v,&x);CHKERRQ(ierr);
  ierr = VecRestoreArray(s,&y);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNC__  
#define __FUNC__ "VecReciprocal_Default"
int VecReciprocal_Default(Vec v)
{
  int    i,n,ierr;
  Scalar *x;

  PetscFunctionBegin;
  ierr = VecGetLocalSize(v,&n);CHKERRQ(ierr);
  ierr = VecGetArray(v,&x);CHKERRQ(ierr);
  for ( i=0; i<n; i++ ) {
    if (x[i] != 0.0) x[i] = 1.0/x[i];
  }
  ierr = VecRestoreArray(v,&x);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNC__  
#define __FUNC__ "VecSum"
/*@
   VecSum - Computes the sum of all the components of a vector.

   Collective on Vec

   Input Parameter:
.  v - the vector 

   Output Parameter:
.  sum - the result

   Level: beginner

.keywords: vector, sum

.seealso: VecNorm()
@*/
int VecSum(Vec v,Scalar *sum)
{
  int    i,n,ierr;
  Scalar *x,lsum = 0.0;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(v,VEC_COOKIE);
  ierr = VecGetLocalSize(v,&n);CHKERRQ(ierr);
  ierr = VecGetArray(v,&x);CHKERRQ(ierr);
  for ( i=0; i<n; i++ ) {
    lsum += x[i];
  }
#if defined(PETSC_USE_COMPLEX)
  ierr = MPI_Allreduce(&lsum,sum,2,MPI_DOUBLE,MPI_SUM,v->comm);CHKERRQ(ierr);
#else
  ierr = MPI_Allreduce(&lsum,sum,1,MPI_DOUBLE,MPI_SUM,v->comm);CHKERRQ(ierr);
#endif
  ierr = VecRestoreArray(v,&x);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNC__  
#define __FUNC__ "VecShift"
/*@
   VecShift - Shifts all of the components of a vector by computing
   x[i] = x[i] + shift.

   Collective on Vec

   Input Parameters:
+  v - the vector 
-  shift - the shift

   Output Parameter:
.  v - the shifted vector 

   Level: intermediate

.keywords: vector, shift
@*/
int VecShift(const Scalar *shift,Vec v)
{
  int    i,n,ierr;
  Scalar *x,lsum = *shift;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(v,VEC_COOKIE);
  ierr = VecGetLocalSize(v,&n);CHKERRQ(ierr); 
  ierr = VecGetArray(v,&x);CHKERRQ(ierr);
  for ( i=0; i<n; i++ ) {
    x[i] += lsum;
  }
  ierr = VecRestoreArray(v,&x);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNC__  
#define __FUNC__ "VecAbs"
/*@
   VecAbs - Replaces every element in a vector with its absolute value.

   Collective on Vec

   Input Parameters:
.  v - the vector 

   Level: intermediate

.keywords: vector,absolute value
@*/
int VecAbs(Vec v)
{
  int    i,n,ierr;
  Scalar *x;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(v,VEC_COOKIE);
  ierr = VecGetLocalSize(v,&n);CHKERRQ(ierr);
  ierr = VecGetArray(v,&x);CHKERRQ(ierr);
  for ( i=0; i<n; i++ ) {
    x[i] = PetscAbsScalar(x[i]);
  }
  ierr = VecRestoreArray(v,&x);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}


#undef __FUNC__  
#define __FUNC__ "VecEqual"
/*@
   VecEqual - Compares two vectors.

   Collective on Vec

   Input Parameters:
+  vec1 - the first matrix
-  vec2 - the second matrix

   Output Parameter:
.  flg - PETSC_TRUE if the vectors are equal; PETSC_FALSE otherwise.

   Level: intermediate

.keywords: vec, equal, equivalent
@*/
int VecEqual(Vec vec1,Vec vec2,PetscTruth *flg)
{
  Scalar *v1,*v2;
  int    n1,n2,ierr,flg1;

  PetscFunctionBegin;
  ierr = VecGetSize(vec1,&n1);CHKERRQ(ierr);
  ierr = VecGetSize(vec2,&n2);CHKERRQ(ierr);
  if (n1 != n2) {
    flg1 = PETSC_FALSE;
  } else {
    ierr = VecGetArray(vec1,&v1);CHKERRQ(ierr);
    ierr = VecGetArray(vec2,&v2);CHKERRQ(ierr);

    if (PetscMemcmp(v1,v2,n1*sizeof(Scalar))) flg1 = PETSC_FALSE;
    else  flg1 = PETSC_TRUE;
    ierr = VecRestoreArray(vec1,&v1);CHKERRQ(ierr);
    ierr = VecRestoreArray(vec2,&v2);CHKERRQ(ierr);
  }

  /* combine results from all processors */
  MPI_Allreduce(&flg1,flg,1,MPI_INT,MPI_MIN,vec1->comm);
  

  PetscFunctionReturn(0);
}



