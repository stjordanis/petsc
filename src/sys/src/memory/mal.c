#ifndef lint
static char vcid[] = "$Id: mal.c,v 1.18 1996/12/16 21:40:18 balay Exp balay $";
#endif
/*
    Code that allows a user to dictate what malloc() PETSc uses.
*/
#include "petsc.h"             /*I   "petsc.h"   I*/
#if defined(HAVE_STDLIB_H)
#include <stdlib.h>
#endif
#if defined(HAVE_MALLOC_H) && !defined(__cplusplus)
#include <malloc.h>
#endif
#include "pinclude/petscfix.h"

/*
    Set the default malloc and free to be the usual system versions
*/
void *(*PetscTrMalloc)(unsigned int,int,char*) = (void*(*)(unsigned int,int,char*))malloc;
int  (*PetscTrFree)(void *,int,char*)          = (int (*)(void*,int,char*))free;

#undef __FUNCTION__  
#define __FUNCTION__ "PetscSetMalloc"
/*@
   PetscSetMalloc - Sets the routines used to do mallocs and frees.
   This routine MUST be called before PetscInitialize() and may be
   called only once.

   Input Parameters:
.  malloc - the malloc routine
.  free - the free routine

.keywords: Petsc, set, malloc, free, memory allocation
@*/
int PetscSetMalloc(void *(*imalloc)(unsigned int,int,char*),
                   int (*ifree)(void*,int,char*))
{
  static int visited = 0;
  if (visited) SETERRQ(1,"cannot call multiple times");
  PetscTrMalloc = imalloc;
  PetscTrFree   = ifree;
  visited       = 1;
  return 0;
}

