/* 
   This is where the abstract vector operations are defined
 */
#include "vecimpl.h"

/*@
     VecValidVector - returns 1 if this is a valid vector else 0

  Input Parameter:
.  v - the object to check
@*/
int VecValidVector(v)
Vec v;
{
  if (!v) return 0;
  if (v->cookie != VEC_COOKIE) return 0;
  return 1;
}
/*@
     VecDot  - Computes vector dot product.

  Input Parameters:
.  x,y - the vectors

  Output Parameter:
.  val - the dot product

@*/
int VecDot(x,y,val)
Vec       x,y;
VecScalar val;
{
  VALIDHEADER(x,VEC_COOKIE); VALIDHEADER(y,VEC_COOKIE);
  CHKSAME(x,y);
  return (*x->ops->dot)(x->data,y->data,val);
}

/*@
     VecNorm  - Computes vector two norm.

  Input Parameters:
.  x - the vector

  Output Parameter:
.  val - the norm 

@*/
int VecNorm(x,val)  
Vec       x;
VecScalar val;
{
  VALIDHEADER(x,VEC_COOKIE);
  return (*x->ops->norm)(x->data,val);
}
/*@
     VecASum  - Computes vector one norm.

  Input Parameters:
.  x - the vector

  Output Parameter:
.  val - the sum 

@*/
int VecASum(x,val)
Vec       x;
VecScalar val;
{
  VALIDHEADER(x,VEC_COOKIE);
  return (*x->ops->asum)(x->data,val);
}

/*@
     VecMax  - Computes maximum of vector and its location.

  Input Parameters:
.  x - the vector

  Output Parameter:
.  val - the max 
.  p - the location
@*/
int VecMax(x,p,val)
Vec       x;
VecScalar val;
int      *p;
{
  VALIDHEADER(x,VEC_COOKIE);
  return (*x->ops->max)(x->data,p,val);
}

/*@
     VecTDot  - non-Hermitian vector dot product. That is, it does NOT
              use the complex conjugate.

  Input Parameters:
.  x, y - the vectors

  Output Parameter:
.  val - the dot product
@*/
int VecTDot(x,y,val) 
Vec       x,y;
VecScalar val;
{
  VALIDHEADER(x,VEC_COOKIE); VALIDHEADER(y,VEC_COOKIE);
  CHKSAME(x,y);
  return (*x->ops->tdot)(x->data,y->data,val);
}
/*@
     VecScale  - Scales a vector. 

  Input Parameters:
.  x - the vector
.  alpha - the scalar
@*/
int VecScale(alpha,x)
VecScalar alpha;
Vec       x;
{
  VALIDHEADER(x,VEC_COOKIE);
  return (*x->ops->scal)(alpha,x->data);
}

/*@
     VecCopy  - Copys a vector. 

  Input Parameters:
.  x  - the vector

  Output Parameters:
.  y  - the copy
@*/
int VecCopy(x,y)
Vec x,y;
{
  VALIDHEADER(x,VEC_COOKIE); VALIDHEADER(y,VEC_COOKIE);
  CHKSAME(x,y);
  return (*x->ops->copy)(x->data,y->data);
}
 
/*@
     VecSet  - Sets all components of a vector to a scalar. 

  Input Parameters:
.  alpha - the scalar

  Output Parameters:
.  x  - the vector
@*/
int VecSet(alpha,x) 
Vec       x;
VecScalar alpha;
{
  VALIDHEADER(x,VEC_COOKIE);
  return (*x->ops->set)(alpha,x->data);
} 

/*@
     VecAXPY  -  Computes y <- alpha x + y. 

  Input Parameters:
.  alpha - the scalar
.  x,y  - the vectors
@*/
int VecAXPY(alpha,x,y)
VecScalar alpha;
Vec       x,y;
{
  VALIDHEADER(x,VEC_COOKIE); VALIDHEADER(y,VEC_COOKIE);
  CHKSAME(x,y);
  return (*x->ops->axpy)(alpha,x->data,y->data);
} 
/*@
     VecAYPX  -  Computes y <- x + alpha y.

  Input Parameters:
.  alpha - the scalar
.  x,y  - the vectors

@*/
int VecAYPX(alpha,x,y)
Vec       x,y;
VecScalar alpha;
{
  VALIDHEADER(x,VEC_COOKIE); VALIDHEADER(y,VEC_COOKIE);
  CHKSAME(x,y);
  return (*x->ops->aypx)(alpha,x->data,y->data);
} 
/*@
     VecSwap  -  Swaps x and y.

  Input Parameters:
.  x,y  - the vectors
@*/
int VecSwap(x,y)
Vec x,y;
{
  VALIDHEADER(x,VEC_COOKIE);  VALIDHEADER(y,VEC_COOKIE);
  CHKSAME(x,y);
  return (*x->ops->swap)(x->data,y->data);
}
/*@
     VecWAXPY  -  Computes w <- alpha x + y.

  Input Parameters:
.  alpha - the scalar
.  x,y  - the vectors

  Output Parameter:
.  w - the result
@*/
int VecWAXPY(alpha,x,y,w)
Vec       x,y,w;
VecScalar alpha;
{
  VALIDHEADER(x,VEC_COOKIE); VALIDHEADER(y,VEC_COOKIE);
  VALIDHEADER(w,VEC_COOKIE);
  CHKSAME(x,y); CHKSAME(y,w);
  return (*x->ops->waxpy)(alpha,x->data,y->data,w->data); 
}
/*@
     VPMult  -  Computes the componentwise multiplication w = x*y.

  Input Parameters:
.  x,y  - the vectors

  Output Parameter:
.  w - the result

@*/
int VecPMult(x,y,w)
Vec x,y,w;
{
  VALIDHEADER(x,VEC_COOKIE); VALIDHEADER(y,VEC_COOKIE);
  VALIDHEADER(w,VEC_COOKIE);
  CHKSAME(x,y); CHKSAME(y,w);
  return (*x->ops->pmult)(x->data,y->data,w->data);
} 
/*@
     VecPDiv  -  Computes the componentwise division w = x/y.

  Input Parameters:
.  x,y  - the vectors

  Output Parameter:
.  w - the result
@*/
int VecPDiv(x,y,w)
Vec x,y,w;
{
  VALIDHEADER(x,VEC_COOKIE); VALIDHEADER(y,VEC_COOKIE);
  VALIDHEADER(w,VEC_COOKIE);
  CHKSAME(x,y); CHKSAME(y,w);
  return (*x->ops->pdiv)(x->data,y->data,w->data);
}
/*@
     VecCreate  -  Creates a vector from another vector. Use VecDestroy()
                 to free the space. Use VecGetVecs() to get several 
                 vectors.

  Input Parameters:
.  v - a vector to mimic

  Output Parameter:
.  returns a pointer to a vector 
@*/
int VecCreate(v,newv) 
Vec v,*newv;
{
  VALIDHEADER(v,VEC_COOKIE);
  return   (*v->ops->create_vector)(v->data,newv);
}
/*@
     VecDestroy  -  Destroys  a vector created with VecCreate().

  Input Parameters:
.  v  - the vector
@*/
int VecDestroy(v)
Vec v;
{
  VALIDHEADER(v,VEC_COOKIE);
  return (*v->destroy)(v);
}

/*@
     VecGetVecs  -  Obtains several vectors. Use VecFreeVecs() to free the 
                  space. Use VecCreate() to get a single vector.

  Input Parameters:
.  m - the number of vectors to obtain
.  v - a vector

  Output Parameters:
.  returns a pointer to an array of pointers.
@*/
int VecGetVecs(v,m,V)  
int m;
Vec v,**V;
{
  VALIDHEADER(v,VEC_COOKIE);
  return (*v->ops->obtain_vectors)( v, m,V );
}

/*@
     VecFreeVecs  -  Frees a block of vectors obtained with VecGetVecs().

  Input Parameters:
.  vv - pointer to array of vector pointers
.  m - the number of vectors to obtain
@*/
int VecFreeVecs(vv,m)
Vec *vv;
int m;
{
  VALIDHEADER(*vv,VEC_COOKIE);
  return (*(*vv)->ops->release_vectors)( vv, m );
}

/*@
     VecScatterBegin  -  Scatters from one vector into another.

  Input Parameters:
.  x - vector to scatter from
.  ix - indices of elements in x to take
.  iy - indices of locations in y to insert 

  Output Parameters:
.  y - vector to scatter to 

  Notes:
.   y[iy[i]] = x[ix[i]], for i=0,...,ni-1
@*/
int VecScatterBegin(x,ix,y,iy,ctx)
Vec y,x;
IS  ix,iy;
VecScatterCtx *ctx;
{
  VALIDHEADER(y,VEC_COOKIE);
  return (*y->ops->scatterbegin)( x, ix, y, iy,ctx);
}
/*@
     VecScatterEnd  -  End scatter from one vector into another.
            Call after call to VecScatterBegin().

  Input Parameters:
.  x - vector to scatter from
.  ix - indices of elements in x to take
.  iy - indices of locations in y to insert 

  Output Parameters:
.  y - vector to scatter to 

  Notes:
.   y[iy[i]] = x[ix[i]], for i=0,...,ni-1
@*/
int VecScatterEnd(x,ix,y,iy,ctx)
Vec y,x;
IS  ix,iy;
VecScatterCtx *ctx;
{
  VALIDHEADER(y,VEC_COOKIE);
  return (*y->ops->scatterend)( x, ix, y, iy,ctx);
}

/*@
     VecScatterAddBegin  -  Scatters from one vector into another.

  Input Parameters:
.  x - vector to scatter from
.  ix - indices of elements in x to take
.  iy - indices of locations in y to insert 

  Output Parameters:
.  y - vector to scatter to 

  Notes:
.   y[iy[i]] += x[ix[i]], for i=0,...,ni-1
@*/
int VecScatterAddBegin(x,ix,y,iy,ctx)
Vec y,x;
IS  ix,iy;
VecScatterCtx *ctx;
{
  VALIDHEADER(y,VEC_COOKIE);
  return (*y->ops->scatteraddbegin)( x, ix, y, iy,ctx);
}

/*@
     VecScatterAddEnd  -  End scatter from one vector into another.
            Call after call to VecScatterAddBegin().

  Input Parameters:
.  x - vector to scatter from
.  ix - indices of elements in x to take
.  iy - indices of locations in y to insert 

  Output Parameters:
.  y - vector to scatter to 

  Notes:
.   y[iy[i]] += x[ix[i]], for i=0,...,ni-1
@*/
int VecScatterAddEnd(x,ix,y,iy,ctx)
Vec y,x;
IS  ix,iy;
VecScatterCtx *ctx;
{
  VALIDHEADER(y,VEC_COOKIE);
  return (*y->ops->scatteraddend)( x, ix, y, iy,ctx);
}

/*@
     VecAddValues - add values into certain location in vector. These
         values may be cached, you must call VecBeginAssembly() and 
         VecEndAssembly() after you have completed all calls to 
         VecAddValues() and VecInsertValues().

  Input Parameters:
.  x - vector to add to 
.  ni - number of elements to add
.  ix - indices where to add
.  y - array of values

  Notes:
.  x[ix[i]] += y[i], for i=0,...,ni-1.

@*/
int VecAddValues(x,ni,ix,y) 
Vec       x;
VecScalar y;
int       *ix, ni;
{
  VALIDHEADER(x,VEC_COOKIE);
  return (*x->ops->addvalues)( x->data, ni,ix, y );
}
/*@
     VecInsertValues - insert values into certain locations in vector. These
         values may be cached, you must call VecBeginAssembly() and 
         VecEndAssembly() after you have completed all calls to 
         VecAddValues() and VecInsertValues().

  Input Parameters:
.  x - vector to insert in
.  ni - number of elements to add
.  ix - indices where to add
.  y - array of values

  Notes:
.  x[ix[i]] = y[i], for i=0,...,ni-1.

@*/
int VecInsertValues(x,ni,ix,y) 
Vec       x;
VecScalar y;
int       *ix, ni;
{
  VALIDHEADER(x,VEC_COOKIE);
  return (*x->ops->insertvalues)( x->data, ni,ix, y );
}

/*@
    VecBeginAssembly - Begins assembling global vector. Call after
      all calls to VecAddValues() and VecInsertValues().

  Input Parameter:
.   vec - the vector to assemble
@*/
int VecBeginAssembly(vec)
Vec vec;
{
  VALIDHEADER(vec,VEC_COOKIE);
  if (vec->ops->beginassm) return (*vec->ops->beginassm)(vec->data);
  else return 0;
}

/*@
    VecEndAssembly - End assembling global vector. Call after
      all call VecBeginAssembly().

  Input Parameter:
.   vec - the vector to assemble
@*/
int VecEndAssembly(vec)
Vec vec;
{
  VALIDHEADER(vec,VEC_COOKIE);
  if (vec->ops->endassm) return (*vec->ops->endassm)(vec->data);
  else return 0;
}

/*@
     VecMTDot  - non-Hermitian vector multiple dot product. 
         That is, it does NOT use the complex conjugate.

  Input Parameters:
.  nv - number of vectors
.  x - one vector
.  y - array of vectors.  Note that vectors are pointers

  Output Parameter:
.  val - array of the dot products
@*/
int VecMTDot(nv,x,y,val)
Vec       x,*y;
VecScalar val;
int       nv;
{
  VALIDHEADER(x,VEC_COOKIE); VALIDHEADER(*y,VEC_COOKIE);
  CHKSAME(x,*y);
  return (*x->ops->mtdot)(nv,x->data,y,val);
}
/*@
     VecMDot  - non-Hermitian vector multiple dot product. That is, it does NOT
               use the complex conjugate.

  Input Parameters:
.  nv - number of vectors
.  x - one vector
.  y - array of vectors. 

  Output Parameter:
.  val - array of the dot products
@*/
int VecMDot(nv,x,y,val)
Vec       x,*y;
VecScalar val;
int       nv;
{
  VALIDHEADER(x,VEC_COOKIE); VALIDHEADER(*y,VEC_COOKIE);
  CHKSAME(x,*y);
  return (*x->ops->mdot)(nv,x->data,y,val);
}

/*@
     VecMAXPY  -  Computes y <- alpha[j] x[j] + y. 

  Input Parameters:
.  nv - number of scalars and x-vectors
.  alpha - array of scalars
.  x  - one vectors
.  y  - array of vectors
@*/
int  VecMAXPY(nv,alpha,x,y)
Vec       x,*y;
VecScalar alpha;
int       nv;
{
  VALIDHEADER(x,VEC_COOKIE); VALIDHEADER(*y,VEC_COOKIE);
  CHKSAME(x,*y);
  return (*x->ops->maxpy)(nv,alpha,x->data,y);
} 

/*@
   VecGetArray - return pointer to vector data. For default seqential 
        vectors returns pointer to array containing data. Otherwise 
        implementation dependent.

  Input Parameters:
.   x - the vector

  Returns pointer to the array
@*/
int VecGetArray(x,a)
Vec x;
VecScalar **a;
{
  VALIDHEADER(x,VEC_COOKIE);
  return (*x->ops->getarray)(x->data,a);
}

/*@
    VecView  - allows user to view a vector. This routines is intended to 
              be replacable with fancy graphical based viewing.

  Input Parameters:
.  v - the vector
.  ptr - a pointer to a viewer ctx
@*/
int VecView(v,ptr)
Vec  v;
void *ptr;
{
  VALIDHEADER(v,VEC_COOKIE);
  return (*v->ops->view)(v->data,ptr);
}

/*@
    VecGetSize - returns number of elements in vector

  Input Parameter:
.   x - the vector

  Output Parameters:
.  returns the vector length
@*/
int VecGetSize(x,size)
Vec x;
int *size;
{
  VALIDHEADER(x,VEC_COOKIE);
  return (*x->ops->getsize)(x->data,size);
}

/*@
    VecGetLocalSize - returns number of elements in vector stored 
               in local memory. This may mean different things 
               for different implementations, use with care.

  Input Parameter:
.   x - the vector

  Output Parameters:
.  returns the vector length stored locally
@*/
int VecGetLocalSize(x,size)
Vec x;
int *size;
{
  VALIDHEADER(x,VEC_COOKIE);
  return (*x->ops->localsize)(x->data,size);
}

/* Default routines for obtaining and releasing; */
/* may be used by any implementation */
int Veiobtain_vectors( w, m,V )
int m;
Vec w,**V;
{
  Vec *v;
  int  i;
  *V = v = (Vec *) MALLOC( m * sizeof(Vec *) );
  for (i=0; i<m; i++) VecCreate(w,v+i);
  return 0;
}

int Veirelease_vectors( v, m )
int m;
Vec *v;
{
  int i;
  for (i=0; i<m; i++) VecDestroy(v[i]);
  FREE( v );
  return 0;
}

int VeiDestroyVector( v )
Vec v;
{
  FREE(v->data); FREE(v);
  return 0;
}
 
