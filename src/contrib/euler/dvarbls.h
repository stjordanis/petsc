c
c  Parallel array sizes, including ghost points, for the
c  residual, corresponding to the F vector in PETSc code.
c  Space is allocated in UserCreateEuler().
c
c      double precision  dr(gxsf1:gxefp1,gysf1:gyefp1,gzsf1:gzefp1)
c      double precision dru(gxsf1:gxefp1,gysf1:gyefp1,gzsf1:gzefp1)
c      double precision drv(gxsf1:gxefp1,gysf1:gyefp1,gzsf1:gzefp1)
c      double precision drw(gxsf1:gxefp1,gysf1:gyefp1,gzsf1:gzefp1)
c      double precision  de(gxsf1:gxefp1,gysf1:gyefp1,gzsf1:gzefp1)
c
c  Uniprocessor array sizes:
c      COMMON /DVRBLS/ DR(NI1,NJ1,NK1),DRU(NI1,NJ1,NK1),DRV(NI1,NJ1,NK1)
c      COMMON /DVRBLS/ DRW(NI1,NJ1,NK1),DE(NI1,NJ1,NK1)

#define DR(i,j,k) dxx(1,i,j,k)
#define DRU(i,j,k) dxx(2,i,j,k)
#define DRV(i,j,k) dxx(3,i,j,k)
#define DRW(i,j,k) dxx(4,i,j,k)
#define DE(i,j,k) dxx(5,i,j,k)

#define dr(i,j,k) dxx(1,i,j,k)
#define dru(i,j,k) dxx(2,i,j,k)
#define drv(i,j,k) dxx(3,i,j,k)
#define drw(i,j,k) dxx(4,i,j,k)
#define de(i,j,k) dxx(5,i,j,k)

        double precision dxx(5,gxsf1:gxefp1,gysf1:gyefp1,gzsf1:gzefp1)

