#ifndef lint
static char vcid[] = "$Id: gtype.c,v 1.4 1996/12/16 21:34:38 balay Exp balay $";
#endif
/*
     Provides utility routines for manulating any type of PETSc object.
*/
#include "petsc.h"  /*I   "petsc.h"    I*/

#undef __FUNCTION__  
#define __FUNCTION__ "PetscObjectGetType"
/*@C
   PetscObjectGetType - Gets the object type of any PetscObject.

   Input Parameter:
.  obj - any PETSc object, for example a Vec, Mat or KSP.

   Output Parameter:
.  type - the object type

.keywords: object, get, type
@*/
int PetscObjectGetType(PetscObject obj,int *type)
{
  if (!obj) SETERRQ(1,"Null object");
  *type = obj->type;
  return 0;
}



