
/*
       Tests the ESI_Vector interface
*/
#include "ESI.h"

extern int ESI_Map_test(ESI_Map *);

int ESI_Vector_test(ESI_Vector *vector)
{
  int            ierr;
  ESI_Map        *map;
  ESI_Vector     *newvector;
  double         mdot[1],dot,norm1,norm2,norm2squared,norminfinity;

  ierr = vector->getMap((const ESI_Map *)map);
  if (ierr) {printf("error calling vector->getMap()\n");return ierr;}
  ierr = ESI_Map_test(map); 
  if (ierr) {printf("error calling ESI_Map_test\n");return ierr;}

  ierr = vector->clone(newvector);
  if (ierr) {printf("error calling vector->clone() \n");return ierr;}

  ierr = vector->copy(*newvector);
  if (ierr) {printf("error calling vector->copy() \n");return ierr;}

  ierr = vector->put(2.0);
  ierr = newvector->put(1.0);
  if (ierr) {printf("error calling vector->put() \n");return ierr;}

  ierr = vector->scale(2.0);
  if (ierr) {printf("error calling vector->scale() \n");return ierr;}

  ierr = vector->scaleDiagonal(*newvector);
  if (ierr) {printf("error calling vector->scalediagonal() \n");return ierr;}

  ierr = vector->axpy(*newvector,3.0);
  if (ierr) {printf("error calling vector->axpy() \n");return ierr;}

  ierr = vector->aypx(5.0,*newvector);
  if (ierr) {printf("error calling vector->aypx() \n");return ierr;}

  ierr = vector->dot(*newvector,dot);
  if (ierr) {printf("error calling vector->dot() \n");return ierr;}
  printf("ESI_Vector_test: dot %g\n",dot);

  ierr = vector->norm1(norm1);
  if (ierr) {printf("error calling vector->norm1() \n");return ierr;}
  printf("ESI_Vector_test: norm1 %g\n",norm1);

  ierr = vector->norm2(norm2);
  if (ierr) {printf("error calling vector->norm2() \n");return ierr;}
  printf("ESI_Vector_test: norm2 %g\n",norm2);

  ierr = vector->norm2squared(norm2squared);
  if (ierr) {printf("error calling vector->norm2squared() \n");return ierr;}
  printf("ESI_Vector_test: norm2squared %g\n",norm2squared);

  ierr = vector->normInfinity(norminfinity);
  if (ierr) {printf("error calling vector->normInfinity() \n");return ierr;}
  printf("ESI_Vector_test: normInfinity %g\n",norminfinity);

  ierr = vector->mdot(1,(const ESI_Vector **) &newvector,mdot);
  if (ierr) {printf("error calling vector->mdot() \n");return ierr;}
  printf("ESI_Vector_test: normmdot %g\n",mdot[0]);

  delete newvector;

  return 0;
}







