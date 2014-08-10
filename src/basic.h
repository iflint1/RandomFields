#ifndef GSL_VS_R_H
#define GSL_VS_R_H 1

#include <R.h>
#include <Rinternals.h>
#include <Rmath.h>
#include <errno.h>
#include <R_ext/Complex.h>


// Formerly in <R_ext/Applic.h>
void fft_factor_(int n, int *pmaxf, int *pmaxp);
Rboolean fft_work_(double *a, double *b, int nseg, int n, int nspn,
		  int isn, double *work, int *iwork);/* TRUE: success */

#define LENGTH length // safety, in order not to use LENGTH defined by R
#define complex Rcomplex
#define DOT "."
#define print Rprintf
#define PRINTF Rprintf
#define DOPRINTF if (DOPRINT) Rprintf
#define KPRINT leer(PrInL);Rprintf
#define LPRINT {cov_model *lprint_z=cov; int lprint_i=0; while (lprint_z->calling != NULL && lprint_i<10) {lprint_z=lprint_z->calling; if (DOPRINT) {Rprintf(DOT); Rprintf(" ");} lprint_i++;} if (lprint_i==100) {Rprintf("LPRINT i=%d\n", lprint_i);PMI(cov); assert(false);}} if (DOPRINT) Rprintf
#define RF_NA NA_REAL 
#define RF_NAN R_NaN
#define RF_NEGINF R_NegInf
#define RF_INF R_PosInf
#define GAUSS_RANDOM(SIGMA) rnorm(0.0, SIGMA)
#define UNIFORM_RANDOM unif_rand()
#define POISSON_RANDOM(x) rpois(x)
#define SQRT2 M_SQRT2
#define SQRTPI M_SQRT_PI
#define INVPI M_1_PI
#define RF_M_SQRT_3 M_SQRT_3
//#define TWOPI M_2PI
#define PIHALF M_PI_2 
#define T_PI M_2_PI
#define ONETHIRD 0.333333333333333333
#define TWOTHIRD 0.66666666666666666667
#define TWOPI 6.283185307179586476925286766559
#define INVLOG2 1.442695040888963
#define INVSQRTTWO 0.70710678118654752440084436210
#define INVSQRTTWOPI 0.39894228040143270286
#define SQRTTWOPI 2.5066282746310002416
#define SQRTINVLOG005 0.5777613700268771079749
//#define LOG05 -0.69314718055994528623
#define LOG3 1.0986122886681096913952452369225257046474905578227
#define LOG2 M_LN2

#define EPSILON     0.00000000001
#define EPSILON1000 0.000000001
#define MAXINT 2147483647
#define INFDIM MAXINT
#define INFTY INFDIM

extern double EIGENVALUE_EPS; // used in GetTrueDim


//
// 1

extern char BUG_MSG[250];
#ifdef ENABLEASSERT
// __extension__ unterdrueckt Fehlermeldung wegen geklammerter Argumente
#define assert(X) if (!__extension__ (X )) {				\
    sprintf(BUG_MSG,							\
	    "'assert(%s) failed in function '%s' (file '%s', line %d).", \
	    #X,__FUNCTION__, __FILE__, __LINE__);			\
    error(BUG_MSG);							\
  }
#define VARIABLE_IS_NOT_USED __attribute__ ((unused))
#else 
#define assert(X) {}
#define VARIABLE_IS_NOT_USED
#endif

#endif /* GSL_VS_R_H */


