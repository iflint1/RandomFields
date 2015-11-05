/*
 Authors
 Martin Schlather, schlather@math.uni-mannheim.de

 Simulation of a random field by Cholesky or SVD decomposition

 Copyright (C) 2001 -- 2015 Martin Schlather, 

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include <math.h>
#include <stdio.h>  
#include <stdlib.h>
#include <R_ext/Lapack.h>

#include "RF.h"
#include "shape_processes.h"
#include "Coordinate_systems.h"
#include "variogramAndCo.h"
//#include <R_ext/Linpack.h>

bool debug=false;


int check_directGauss(cov_model *cov) {
#define nsel 4
  cov_model *next=cov->sub[0];
  location_type *loc = Loc(cov);
  int j, err ; // taken[MAX DIM],
  direct_param *gp  = &(GLOBAL.direct); //
  
  ROLE_ASSERT(ROLE_GAUSS); 

  kdefault(cov, DIRECT_METHOD, (int) gp->inversionmethod);
  kdefault(cov, DIRECT_SVDTOL, gp->svdtolerance);
  kdefault(cov, DIRECT_MAXVAR, gp->maxvariables);
  if ((err = checkkappas(cov, false)) != NOERROR) return err;
  if ((cov->tsdim != cov->xdimprev || cov->tsdim != cov->xdimown) &&
      (!loc->distances || cov->xdimprev!=1)) {
    return ERRORDIM;
  } 

  int jj, isotropy[2],  
    endjj = 0;
  if (!isCartesian(cov->isoown)) isotropy[endjj++] = cov->isoown;
  else isotropy[endjj++] = SymmetricOf(cov->isoown);
  Types type[2] = {PosDefType, VariogramType};
  for (jj=0; jj<endjj; jj++) {
     for (j=0; j<=1; j++) {    
       //printf("direct:: %s %s\n", ISONAMES[isotropy[jj]], TYPENAMES[type[j]]);

       //assert(cov->isoown == EARTH_COORD);
     
      if ((err = CHECK(next, cov->tsdim,  cov->xdimprev, 
		       type[j], KERNEL, isotropy[jj],
		       SUBMODEL_DEP, ROLE_COV)) == NOERROR) break;
     }
     if (err == NOERROR) break;
  }

  if (err != NOERROR) return err;  
  if (next->pref[Direct] == PREF_NONE) return ERRORPREFNONE;


  setbackward(cov, next);
  KAPPA_BOXCOX;
  if ((err = checkkappas(cov)) != NOERROR) return err;

  return NOERROR;
}



void range_direct(cov_model VARIABLE_IS_NOT_USED *cov, range_type *range) {
  GAUSS_COMMON_RANGE;

  range->min[DIRECT_METHOD] = Cholesky;
  range->max[DIRECT_METHOD] = NoFurtherInversionMethod;
  range->pmin[DIRECT_METHOD] = Cholesky;
  range->pmax[DIRECT_METHOD] = NoFurtherInversionMethod;
  range->openmin[DIRECT_METHOD] = false;
  range->openmax[DIRECT_METHOD] = true; 

  range->min[DIRECT_SVDTOL] = 0;
  range->max[DIRECT_SVDTOL] = 1;
  range->pmin[DIRECT_SVDTOL] = 1e-17;
  range->pmax[DIRECT_SVDTOL] = 1e-8;
  range->openmin[DIRECT_SVDTOL] = false;
  range->openmax[DIRECT_SVDTOL] = true; 

  range->min[DIRECT_MAXVAR] = 0;
  range->max[DIRECT_MAXVAR] = 10000;
  range->pmin[DIRECT_MAXVAR] = 500;
  range->pmax[DIRECT_MAXVAR] = 5000;
  range->openmin[DIRECT_MAXVAR] = false;
  range->openmax[DIRECT_MAXVAR] = false; 
}


int init_directGauss(cov_model *cov, gen_storage VARIABLE_IS_NOT_USED *S) {
  cov_model *next = cov->sub[0];
  double //*xx,
    svdtol = P0(DIRECT_SVDTOL), 
    *G=NULL, 
    *Cov=NULL, 
    *U=NULL, 
    *VT=NULL, 
    *work=NULL,
    *D=NULL, 
    *SICH=NULL;
  int 
    err = NOERROR,
    maxvariab = P0INT(DIRECT_MAXVAR),
    *iwork=NULL;
  int dim=cov->tsdim;
  direct_storage* s=NULL;
  InversionMethod
    method = (InversionMethod) P0INT(DIRECT_METHOD);
  location_type *loc = Loc(cov);
  bool storing = GLOBAL.internal.stored_init; //
  // nonstat_covfct cf;
  long 
    vdim = cov->vdim[0],
    locpts = loc->totalpoints,
//    loctot = locpts *dim,
    vdimtot = vdim * locpts,
    //     vdimSqtot = vdim * vdimtot,
    // vdimtotSq = vdimtot * locpts,
    vdimSqtotSq = vdimtot * vdimtot;

  ROLE_ASSERT_GAUSS;
  assert(cov->vdim[0] == cov->vdim[1]);

  cov->method = Direct;

  if ((err = alloc_cov(cov, dim, vdim, vdim)) != NOERROR) return err;

   if (vdimtot > maxvariab) {
     GERR3(" '%s' valid only for less than or equal to %d data. Got %ld data.",
	  NICK(cov), maxvariab, vdimtot);
   }
   
  //printf("vdim = %d %d %d %d\n", vdim, locpts, vdimtot, vdimSqtotSq); 
  //    PMI(cov);

   if ((Cov =(double *) MALLOC(sizeof(double) * vdimSqtotSq))==NULL ||
      (U =(double *) MALLOC(sizeof(double) * vdimSqtotSq))==NULL ||
   //for SVD/Chol intermediate r esults AND  memory space for do_directGauss:
       (G = (double *)  CALLOC(vdimtot + 1, sizeof(double)))==NULL) {
    err=ERRORMEMORYALLOCATION;  
    goto ErrorHandling;
  }  
  NEW_STORAGE(direct);
  s = cov->Sdirect;

  /* ********************* */
  /* matrix creation part  */
  /* ********************* */
  
  CovarianceMatrix(next, Cov); 
  assert(R_FINITE(Cov[0]));
 
  //PMI(cov->calling->calling->calling->calling);   
  if (false) {
    long i,j;
    PRINTF("\n");
    for (i=0; i<locpts * vdim; i++) {
       for (j=0; j<locpts * vdim; j++) {
	 PRINTF("%+2.3e ", Cov[i  + locpts * vdim * j]);
	 assert(R_FINITE( Cov[i  + locpts * vdim * j]));
       }
       PRINTF("\n");
    }
    assert(false); //
  }
  
  if (!isPosDef(next)) {
    if (isVariogram(next)) {
      long i, j, v;
      double min,
	*C = Cov;
      min = RF_INF;
      for (i=0; i< vdimSqtotSq; i++) if (Cov[i] < min) min=Cov[i];
      //       print("Cov %f\n", min);
      // Die Werte der Diagonalbloecke werden erh\"oht:
      for (C=Cov, v=0; v<vdim; v++, C += locpts) { 
	for (i=0; i<locpts; i++, C+=vdimtot) {
	  for (j=0; j<locpts; C[j++] -= min);
	}
      }
    } else {
      //   APMI(next);
      err = ERRORNOVARIOGRAM; 
      goto ErrorHandling;
    }
  }

  if (false) {
    long i,j,
      endfor = locpts * vdim
      // endfor = 40
      ;
    PRINTF("\n");
    for (i=0; i<endfor; i++) {
       for (j=0; j<endfor; j++) {
	 if (ISNAN(Cov[i  + locpts * vdim * j])) BUG;
	 PRINTF("%+2.2f ", Cov[i  + locpts * vdim * j]);
       }
       PRINTF("\n");
    }
    //   assert(false); 
  }
   
  //APMI(cov);
   
  /* ********************** */
  /*  square root of matrix */
  /* ********************** */
  int row, Err,k;

  //printf(" vdimSqtotSq = %d\n",  vdimSqtotSq);assert(false);
  
  switch (method) {
  case Cholesky : 
    // only works for strictly positive def. matrices
    if (PL>=PL_STRUCTURE) { LPRINT("method for the root=Cholesky\n"); }
    row=vdimtot;
    // dchdc destroys the input matrix; upper half of U contains result!
    MEMCOPY(U, Cov, sizeof(double) * vdimSqtotSq);
    if (debug) {
      err = ERRORDECOMPOSITION; goto ErrorHandling;
    }   

    //  printf("aaaa\n");
    //  printf("%d\n", row);
    // for (int y=0; y< row * row; y++) printf("%d %f ", y, U[y]); printf("\n");
    //APMI(cov);
    //BUG;

    F77_CALL(dpotrf)("Upper", &row, U, &row, &Err);  

    if (false)  {
      double *sq  = (double *) MALLOC(sizeof(double) * vdimtot * vdimtot);
      AtA(U, vdimtot, vdimtot, sq);
      
      long i,j;
      PRINTF("AtA \n");
      for (i=0; i<locpts * vdim; i++) {
	for (j=0; j<locpts * vdim; j++) {
	  PRINTF("%+2.2f ", Cov[i  + locpts * vdim * j]);
	}
	PRINTF("\n");
      }
      //  assert(false);
    }
 
    
    // F77_NAME(dchdc)(U, &row, &row, G, NULL, &choljob, &err);
    if (Err!=NOERROR) {
      if (PL>=PL_SUBIMPORTANT) { 
	INDENT; PRINTF("Error code Cholesky (dpotrf) = %d\n", Err); 
      }
      err=ERRORDECOMPOSITION;
    } else break;
    // try next method : 
    // most common error: singular matrix 
          
    if (svdtol <= 0.0) break;
    
   case SVD :  // works for any positive semi-definite matrix
     double sum;
     method = SVD; // necessary if the value of method has been Cholesky.
     //               originally

     if (vdimtot > maxvariab * 0.8)
       GERR3("'%s' valid for number of locations less than 0.8 * RFparameters()$direct.maxvariables (%d) for SVD. Got %ld.", NICK(cov), maxvariab, vdimtot);
     if (PL>=PL_STRUCTURE) { LPRINT("method to the root=SVD\n"); }
     if ((VT =(double *) MALLOC(sizeof(double) * vdimSqtotSq))==NULL ||
	 (D =(double *) MALLOC(sizeof(double) * vdimtot))==NULL ||
	 (iwork = (int *) MALLOC(sizeof(int) * 8 * vdimtot))==NULL ||
	 (SICH =(double *) MALLOC(sizeof(double) * vdimSqtotSq))==NULL) {
       err=ERRORMEMORYALLOCATION;
       goto ErrorHandling;
     }
 
     MEMCOPY(SICH, Cov, sizeof(double) * vdimSqtotSq);
     row=vdimtot;
     // dsvdc destroys the input matrix !!!!!!!!!!!!!!!!!!!!
     
     // DGESDD (or DGESVD)
     // dgesdd destroys the input matrix Cov;
     // F77_NAME(dsvdc)(Cov, &row, &row, &row, D, e, U, &row, V, &row, G,
     //		&jobint /* 11 */, &err);
     double optim_lwork;
     int lwork;
     lwork = -1;
     F77_CALL(dgesdd)("A", &row, &row, SICH, &row, D, U, &row, VT, &row, 
		      &optim_lwork, &lwork, iwork, &Err);
     if ((err=Err) != NOERROR) {
        err=ERRORDECOMPOSITION;
       goto ErrorHandling;
     }
     lwork = (int) optim_lwork;
     if ((work = (double *) MALLOC(sizeof(double) * lwork))==NULL)
       goto ErrorHandling;
 
     //   for (int y=0; y<row * row; y++) printf("%d:%f ", y, SICH[y]);

     F77_CALL(dgesdd)("A",  &row,  &row, SICH, &row, D, U, &row, VT, &row, 
		      work, &lwork, iwork, &Err);
     
     err = Err;
     if (err==NOERROR && ISNAN(D[0])) err=9999;
     if (err!=NOERROR) {
       if (PL>PL_ERRORS) { 
	 LPRINT("Error code SVD (dgesdd) = %d\n", err); 
	   }
       err=ERRORDECOMPOSITION;
       goto ErrorHandling;
     }

     int i,j;
	 /* calculate SQRT of covariance matrix */
     for (k=0,j=0;j<vdimtot;j++) {
       double dummy;
       //printf("diag=%d %f\n", j, D[j]);
       dummy = sqrt(D[j]);
       for (i=0;i<vdimtot;i++) {
	     U[k++] *= dummy;
       }
     }
 
     //for (k=0; k<16; k++) printf("%f\n", U[k]); printf("\n");

     /* check SVD */
     if (svdtol >=0) {
       for (i=0; i<vdimtot; i++) {
	 for (k=i; k<vdimtot; k++) {
	   sum = 0.0;
	   for (j=0; j<vdimSqtotSq; j+=vdimtot) sum += U[i+j] * U[k+j];
	   
	   if (fabs(Cov[i * vdimtot + k] - sum) > svdtol) {
	     if (PL > PL_ERRORS) {
	       LPRINT("difference %e at (%d,%d) between the value (%e) of the covariance matrix and the square of its root (%e).\n", 
		      Cov[i * vdimtot +k] - sum, i, k, Cov[i* vdimtot +k ], 
		      sum);
	     }
	     GERR3("required precision not attained  (%e !> %e): probably invalid model.\nSee also argument '%s'.", fabs(Cov[i * vdimtot + k] - sum), svdtol, direct[DIRECT_SVDTOL - COMMON_GAUSS- 1] );
	     goto ErrorHandling;
	   }
	 }
       }
     }

    if ((InversionMethod) P0INT(DIRECT_METHOD)==Cholesky && 
	PL>=PL_SUBIMPORTANT) {
       INDENT; PRINTF("SVD ok\n"); 
     }
     break;
  default : BUG;
   } // switch

 
  err = FieldReturn(cov);

  ErrorHandling: // and NOERROR...

   if (s != NULL) s->method = method;
   if (!storing && err!=NOERROR) {
     FREE(U);
     FREE(G); 
   } else {
    if (s != NULL) {
      s->U=U;
      s->G=G;
    }
  }
   FREE(SICH);
   FREE(Cov);
   FREE(D);
   FREE(work);
   FREE(iwork);
   FREE(VT);

  return err;
}

void do_directGauss(cov_model *cov, gen_storage VARIABLE_IS_NOT_USED *S) {  
  location_type *loc = Loc(cov);
  direct_storage *s = cov->Sdirect;
  long i, j, k,
    locpts = loc->totalpoints,
    vdim = cov->vdim[0],
    vdimtot = locpts * vdim;
  double dummy,
    *G = NULL,
    *U = NULL,
    *res = cov->rf;  


  SAVE_GAUSS_TRAFO;
  //  int m, n;
   // bool  vdim_close_together = GLOBAL.general.vdim_close_together;

  //   APMI(cov);


  U = s->U;// S^{1/2}
  G = s->G;// only the memory space is of interest (stored to avoid 
  //          allocation errors here)
  for (i=0; i<vdimtot; i++) {
    G[i] = GAUSS_RANDOM(1.0);
    //printf("%d %f\n", i, G[i]);
  }
  
  switch (s->method) {
  case Cholesky :
    //   if (vdim_close_together) {
      for (k=0, i=0; i<vdimtot; i++, k+=vdimtot) {
	double *Uk = U + k; 
	dummy =0.0;
	for (j=0; j<=i; j++){
	  dummy += G[j] * Uk[j];
	}
	res[i] = (double) dummy; 
	//	printf("i=%d %f %lu\n", i, res[i], res);
      }
      //   } else {
      //    //printf("vdim %d vdimtot %d\n", vdim, vdimtot);
      //  for (k=i=m=0; m<vdim; m++) {
      //	for (n=m; n<vdimtot; n+=vdim, i++, k+=vdimtot) {
      //	  double *Uk = U + k;
      //	  dummy = 0.0;
      //	  for (j=0; j<=i; j++){
      //	    dummy += G[j] * Uk[j];
      //	  }
      //	  res[n] = (double) dummy; 
      //	  //	printf("%d %f\n", n, res[n]);
      //	}
      //      }
      //  }
    //{int i; for(i=0;i<18;i++) printf("%d:%f \n", i,res[i]); printf("\n");APMI(cov)}
    break;
  case SVD :
    // if (vdim_close_together) {
      for (i=0; i<vdimtot; i++){
	dummy = 0.0;
	for (j=0, k=i; j<vdimtot; j++, k+=vdimtot){
	dummy += U[k] * G[j];
      }
      res[i] = (double) dummy; 
      }
      // } else {
      ///      for (i=m=0; m<vdim; m++) {
      //	for (n=m; n<vdimtot; n+=vdim, i++) {
      //	  dummy = 0.0;
      //	  for (j=0, k=i; j<vdimtot; j++, k+=vdimtot){
      //	    dummy += U[k] * G[j];
      //	  }
      //	  res[n] = (double) dummy; 
      //	}
      //      }
      //    }
    break;
  default : BUG;
  }

  BOXCOX_INVERSE;

}

