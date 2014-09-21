/*
 Authors
 Martin Schlather, schlather@math.uni-mannheim.de

 Simulation of a random field by turning bands;
 see RFspectral.cc for spectral turning bands

 Copyright (C) 2001 -- 2014 Martin Schlather, 

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
//#include <sys/timeb.h>
#include <unistd.h>
 
#include <R_ext/Applic.h>
#include "RF.h"
#include "Covariance.h"
//#include "Covariance.h"
#include "win_linux_aux.h"
#include <R_ext/Utils.h>     

#define MAXNN 100000000.0 /* max number of points simulated on a line */

#define TBM_LINES (COMMON_GAUSS + 4)
#define TBM_LINESIMUFACTOR (COMMON_GAUSS + 5)
#define TBM_LINESIMUSTEP (COMMON_GAUSS + 6)
//#define TBM_GRID (COMMON_GAUSS + 7)

#define TBM_CENTER (COMMON_GAUSS + 7)
#define TBM_POINTS (COMMON_GAUSS + 8)

#define TBM_COV 0

#define BUFFER 4.0


void unitvector3D(int projectiondim, double *deltax, double *deltay, 
		double *deltaz) {
  switch (projectiondim) { // create random unit vector
  case 1 : 
    *deltax= UNIFORM_RANDOM;
    *deltaz = *deltay = 0.0;
    break;
  case 2 :
    *deltaz = 0.0;
    *deltax= UNIFORM_RANDOM;// see Martin's tech rep for details
    *deltay= sqrt(1.0 - *deltax * *deltax) * sin(UNIFORM_RANDOM*TWOPI);
    break;
  case 3 : 
    double dummy;
    *deltaz = 2.0 * UNIFORM_RANDOM - 1.0;
    dummy = sqrt(1.0 - *deltaz * *deltaz);
    *deltay = UNIFORM_RANDOM * TWOPI;
    *deltax = cos(*deltay) * dummy;
    *deltay = sin(*deltay) * dummy;
    break;
  default : BUG;
  }
}


cov_model* get_user_input(cov_model *cov) {
  // loc_user sind nicht die prevloc data, sondern, die die der Nutzer
  // eingegeben hatte !! Zumindest in erster Naehrung  
  // Problem hier falls intern aufgerufen wurde -- Abhilfe: layer muss
  // zwingend gesetzt sein
  cov_model *user=cov;
  while (user->calling != NULL) user = user->calling;
  return user;
}


int get_subdim(cov_model *cov, bool Time, bool *ce_dim2, int *ce_dim,
	       int *effectivedim) {
  int fulldim = P0INT(TBM_FULLDIM);
  double
    layers = P0(TBM_LAYERS);
  *effectivedim = cov->tsdim;
  if (Time) {
    *ce_dim2 = (!ISNA(layers) && layers) ||
      cov->isoown==SPACEISOTROPIC ||
      *effectivedim == 1 + fulldim;
    *effectivedim -= *ce_dim2;  
    if (*ce_dim2 && !ISNA(layers) && !layers) {
      SERR1("value of '%s' does not match the situation", KNAME(TBM_LAYERS))
    }
  } else {
    *ce_dim2 = false;
  }
  if (*effectivedim > fulldim) return ERRORWRONGDIM;
  *ce_dim = 1 + (int) *ce_dim2;
  return NOERROR;
}


void tbm_kappasproc(int i, cov_model *cov, int *nr, int *nc){
  int dim = cov->tsdim;
  //  printf("tbm kappas %d %d %d\n", i, TBM_CENTER, dim);

  *nr = (i==TBM_CENTER) ? dim : 1;
  *nc = i < CovList[cov->nr].kappas ? 1 : -1;
}


void rangetbmproc(cov_model *cov, range_type *range){ 
  rangetbm_common(cov, range, false);

  range->min[TBM_LINES] = 1.0;
  range->max[TBM_LINES] = RF_INF;
  range->pmin[TBM_LINES] = 1.0;
  range->pmax[TBM_LINES] = 10000;
  range->openmin[TBM_LINES] = false;
  range->openmax[TBM_LINES] = true;

  range->min[TBM_LINESIMUFACTOR] = 0.0;
  range->max[TBM_LINESIMUFACTOR] = RF_INF;
  range->pmin[TBM_LINESIMUFACTOR] = 0.0;
  range->pmax[TBM_LINESIMUFACTOR] = 10.0;
  range->openmin[TBM_LINESIMUFACTOR] = false;
  range->openmax[TBM_LINESIMUFACTOR] = true;

  range->min[TBM_LINESIMUSTEP] = 0.0;
  range->max[TBM_LINESIMUSTEP] = RF_INF;
  range->pmin[TBM_LINESIMUSTEP] = 0.0;
  range->pmax[TBM_LINESIMUSTEP] = 10.0;
  range->openmin[TBM_LINESIMUSTEP] = false;
  range->openmax[TBM_LINESIMUSTEP] = true;

  // range->min[TBM_GRID] = 0;
  //range->max[TBM_GRID] = 1;
  // range->pmin[TBM_GRID] = 0;
  // range->pmax[TBM_GRID] = 1;
  // range->openmin[TBM_GRID] = false;
  //  range->openmax[TBM_GRID] = false;

  range->min[TBM_CENTER] = RF_NEGINF;
  range->max[TBM_CENTER] = RF_INF;
  range->pmin[TBM_CENTER] = -1000000;
  range->pmax[TBM_CENTER] = 1000000;
  range->openmin[TBM_CENTER] = false;
  range->openmax[TBM_CENTER] = true;

  range->min[TBM_POINTS] = 0;
  range->max[TBM_POINTS] = RF_INF;
  range->pmin[TBM_POINTS] = 0;
  range->pmax[TBM_POINTS] = 1000;
  range->openmin[TBM_POINTS] = false;
  range->openmax[TBM_POINTS] = true;

}


int checktbmproc(cov_model *cov) {
#define NSEL 2
  cov_model 
    *next=cov->sub[0],
    *key =cov->key,
    *sub = key==NULL ? next : key;
  int i, err = NOERROR,
    isoselect[NSEL+1]={ISOTROPIC, SPACEISOTROPIC, SYMMETRIC},
    nsel = NSEL,
    dim = cov->tsdim; // taken[MAX DIM],
  tbm_param *gp  = &(GLOBAL.tbm);

  if (cov->tsdim != cov->xdimprev || cov->tsdim != cov->xdimown) 
    return ERRORDIM;
  
  if (GLOBAL.general.vdim_close_together && cov->vdim2[0] > 1)
    SERR1("TBM only allowed if '%s=FALSE'", general[GENERAL_CLOSE]);

  ROLE_ASSERT(ROLE_GAUSS);
  
   
  // printf("%s fulldim %d %d\n", NICK(cov), fulldim, 0);
  //   printf("%s fulldim %d %d\n", NICK(cov),
  // 	 fulldim, gp->lines[fulldim-1]);assert(false);

  //PMI(cov);

  kdefault(cov, TBM_FULLDIM, gp->fulldim);
  kdefault(cov, TBM_FULLDIM, PisNULL(TBM_TBMDIM) || gp->tbmdim >= 0
	   ? gp->fulldim : P0INT(TBM_TBMDIM) - gp->tbmdim);
  kdefault(cov, TBM_TBMDIM, gp->tbmdim > 0 
	   ? gp->tbmdim : P0INT(TBM_FULLDIM) + gp->tbmdim);
  kdefault(cov, TBM_LAYERS, gp->layers);
  int 
    tbmdim = P0INT(TBM_TBMDIM),
    fulldim = P0INT(TBM_FULLDIM);
  if (tbmdim >= fulldim) {
     SERR4("'%s' (=%d) must be less than '%s' (=%d)", 
	   KNAME(TBMOP_TBMDIM), tbmdim, KNAME(TBM_FULLDIM), fulldim);
  }
  kdefault(cov, TBM_LINES, gp->lines[fulldim-1]);
  kdefault(cov, TBM_LINESIMUFACTOR, gp->linesimufactor); 
  kdefault(cov, TBM_LINESIMUSTEP, gp->linesimustep);
  //  kdefault(cov, TBM_GRID, gp->grid);
  // if ( P0INT(TBM_GRID))
  //  warning("grid parameter for tbm not programmed yet");
  if (PisNULL(TBM_CENTER)) {    
    PALLOC(TBM_CENTER, dim, 1);
    for (i=0; i<dim; i++) {
      P(TBM_CENTER)[i] = gp->center[i];
    }
  } else {
    if (cov->nrow[TBM_CENTER] < dim) 
      SERR1("vector for '%s' too short", KNAME(TBM_CENTER));
  }

  // PMI(cov);

  kdefault(cov, TBM_POINTS, gp->points);

  //  printf("%d\n", gp->points);
  // PMI(cov);

  if ((err = checkkappas(cov)) != NOERROR)  return err;
 

  //   PMI(cov);
  //printf("key/next/sub %ld %ld %ld\n", key, next, sub);//  assert(false);

  if (key == NULL && isNegDef(sub)) {
    // Abfolge Tbm $(Aniso) iso-model braucht Moeglichkeit des 
    // anisotropen Modells
    if (cov->role == ROLE_BASE) nsel++;
 
    for (i=0; i<nsel; i++) {	
      //printf("dim =%d %d\n", dim, cov->role == ROLE_BASE ? KERNEL : XONLY);

      //    PMI(sub);

      if ((err = CHECK(sub, dim,  dim, NegDefType, 
		       cov->role == ROLE_BASE ? KERNEL : XONLY, // wegen
		       // nutzer eingabe aniso und dem allerersten check
		       isoselect[i],
		       SUBMODEL_DEP, ROLE_COV)) == NOERROR) break;
    }

    //  assert(false);

    if (i==nsel) {
      //
      //       printf("%d %d %d\n", cov->role,  ROLE_BASE, nsel);
      //APMI(cov); 
      //
       sprintf(ERROR_LOC, "%s: ", NICK(cov));
       XERR(err);
       SERR("Its submodel coudl not be identified as isotropic or space-isotropic and positive definite function.");
    }
  } else { // sub->nr > FIRSTGAUSSPROC || key != NULL

    bool dummy;
    int dummydim, newdim;

    if (key != NULL) {
      // falls hier gelandet, so ruft TBMINTERN TBM auf!!
      if (cov->nr == TBM_PROC_USER) { 
	cov_model *intern = sub;
	while (intern != NULL && //intern->nr != TBM_PROC_INTERN &&
	       (isAnyDollar(intern) || intern->nr == TBM_PROC_USER  // 
		))
	  intern = intern->key != NULL ? intern->key : intern->sub[0];
	if (intern == NULL) {
	  //	APMI(key);
	  BUG;
	} else if (intern != cov) 
	  paramcpy(intern, cov, true, true, false, false, false);
      }
    }

    if (cov->nr == TBM_PROC_USER) newdim = dim; 
    else if ((err = get_subdim(cov, get_user_input(cov)->prevloc->Time, &dummy,
			       &newdim, &dummydim)) != NOERROR) return err;

    //PMI(sub); printf("nd %d\n", newdim); assert(newdim == 1);
    //PMI(sub);

    if ((err = CHECK(sub, newdim, newdim, ProcessType, XONLY, CARTESIAN_COORD,
		     SUBMODEL_DEP, 
		     cov->role == ROLE_BASE ? cov->role : ROLE_GAUSS))
	!= NOERROR) {
      return err;
    }
  }
  setbackward(cov, sub);
  cov->vdim2[0] = next->vdim2[0];
  cov->vdim2[1] = next->vdim2[1];

  // PMI(cov);
  // 
  //printf("OK\n"); 
   
   
  return NOERROR;
}


int struct_tbmproc(cov_model *cov, cov_model **newmodel) { 
  cov_model
    *next = cov->sub[TBM_COV];
  ASSERT_NEWMODEL_NULL;
  if (next->pref[TBM] == PREF_NONE) {
    return ERRORPREFNONE;
  }

  //   PMI(cov, "start struct tbmproc"); assert(false);

  assert(cov->nr == TBM_PROC_INTERN);

  cov_model *user = get_user_input(cov);
  location_type *loc_user = user->prevloc;

  double linesimufactor, diameter,
    min[MAXTBMSPDIM], max[MAXTBMSPDIM], Center[MAXTBMSPDIM],
    user_min[MAXTBMSPDIM], user_max[MAXTBMSPDIM], user_Center[MAXTBMSPDIM],
    tbm_linesimustep = P0(TBM_LINESIMUSTEP),
    tbm_linesimufactor = P0(TBM_LINESIMUFACTOR),
    *tbm_center = P(TBM_CENTER);
  int d, 
    err=NOERROR,
    //  tbm_lines = P0INT(TBM_LINES),
    fulldim = P0INT(TBM_FULLDIM), 
    tbmdim = P0INT(TBM_TBMDIM),
    //  endaniso = origdim * origdim - 1,
    *points = PINT(TBM_POINTS),
    user_dim = loc_user->timespacedim;

  long
    user_spatialpoints = loc_user->spatialtotalpoints,
    user_spatialdim = loc_user->spatialdim
    // the isotropic part (i.e. time might be not considered)
    ;
  
  bool ce_dim2=false; 
  //  char errorloc_save[nErrorLoc];


  if (cov->role != ROLE_GAUSS && cov->role != ROLE_BASE){
    return ERRORFAILED;
  }
 

  /****************************************************************/
  /*               Determination of the dimensions                */
  /****************************************************************/
  // einfache checks
  // extraktion von covarianzfunktion mit gleichartiger anisotropie
  //    time-komponente wird nicht ge-checked da letzter parameter true
  // bereitstellung von param, quotient, multiply
  
  /* in tbm, time component must be ALWAYS alone, i.e. matrix looks like
   * * 0 
   * * 0
   * * x
   -- this is checked late on
   
   here x may be any constant. In case x is zero 
   (for all matrices (0..actcov-1), the last dimension
   vanishes (performed in the next step)
   
   the asterixes build submatrices for (v=1..actcov-1) that are multiple of the 
   submatrix for v=0, already checked by Check
  */
 
  if (tbmdim != 1 || (fulldim != 2 && fulldim != 3))
    SERR5("'%s' only works for %s =1 and %s=2 or 3. Got %d %d",
	  NICK(cov), KNAME(TBM_TBMDIM), KNAME(TBM_FULLDIM), tbmdim, fulldim);
  if (cov->tsdim > MAXTBMSPDIM) return ERRORMAXDIMMETH;


  // in theory it works also for variograms!
 
  if (!isNegDef(next->typus) || next->domown != XONLY) 
    return ERRORNOVARIOGRAM; 

  if (user_dim == 1) 
    SERR("dimension must currently be at least 2 and at most 4 for TBM");

  NEW_STORAGE(Stbm, TBM, TBM_storage);
  TBM_storage *s = cov->Stbm;

  // nutzer can ueber tbm_center den centre vorgeben.
  // tbm_center muss auch wieder zurueckgegeben werden.
  // tbm_center/user_centre muss dann noch in caniso-Koordinaten
  // transformiert werden, siehe unten
  GetDiameter(loc_user, user_min, user_max, user_Center);

  if (false) { 
    int i,j;
    double xmin[2] = {RF_INF, RF_INF},
      xmax[2] = {RF_NEGINF, RF_NEGINF};
      
      //      PMI(cov);
      for (i=0; i<loc_user->totalpoints; i++) {
	for (j=0; j<2; j++) {
	  //printf("i=%d j=%d\n", i, j);
	  if (loc_user->x[2*i + j] < xmin[j]) xmin[j] = loc_user->x[2*i + j];
	  if (loc_user->x[2*i + j] > xmax[j]) xmax[j] = loc_user->x[2*i + j];
	}
      }
      
      //      for (j=0; j<2; j++) {
      //	printf("d=%d  xmin=%f min=%f;  xmax=%f max=%f\n",
      //	       j, xmin[j], user_min[j], xmax[j], user_max[j]);
      //      }      
  }

  if (!ISNAN(tbm_center[0])) {
    for (d=0; d < user_dim; d++) {
      if (!R_FINITE(tbm_center[d])) {
	SERR("center contains non finite values");
      }
    }
    for (d=0; d<user_dim; d++) { // not here no time component!
      user_Center[d] = tbm_center[d]; // in user_ coordinates !
    }
  } else {
    for (d=0; d<user_dim; d++) {
      tbm_center[d] = user_Center[d];   
    }
  }


  if ((err=get_subdim(cov, loc_user->Time, &ce_dim2, &(s->ce_dim),
		      &(s->simuspatialdim)))
      != NOERROR) return err;


   /****************************************************************/
  /*          determine grid distance for the line                */
  /****************************************************************/

 // sort out which finer grid should be used in the spatial domain for
  // low dimensional simulation
  if (PL>=PL_STRUCTURE) PRINTF("\ndetermining the length of line segment...");

  // PrintMethodInfo(meth);

  // minimum distance between the points   
  linesimufactor = -1.0;
  if (tbm_linesimustep  > 0.0) linesimufactor = 1.0 / tbm_linesimustep;
  else if (tbm_linesimufactor > 0) {
    double mindelta = RF_INF;
    int spDim = user_dim - (int) ce_dim2; // eigentlich muesste man
    // es das Urbild der Zeitachse abziehen... Egal. Fuer den Ausbau reicht es.
    if (loc_user->grid) {
      // if linesimufactor, the fine grid scale is proportional to smallest 
      // distance in the grid. 
      for (d=0; d<spDim; d++) {
        if ((loc_user->xgr[d][XSTEP]<mindelta) && (loc_user->xgr[d][XSTEP]>0)) {
	  mindelta=loc_user->xgr[d][XSTEP];
	}
      }
    } else {
      long j, i, k0, k1, k2;	
      if (user_spatialpoints > 10000) {
	SERR("too many points to determine smallest distance between the points in a reasonable time; try TBM*.linesimustep with a positive value");
	  /* algorithmus kann verbessert werden, so dass diese Fehlermeldung 
	     nicht mehr notwendig ist! */ 
      }
      double diff, dist;
      for (k0=i=0; i<user_spatialpoints; i++, k0+=user_spatialdim) {
	for (k2=j=0; j<i; j++) {
	  k1 = k0;
	  for (dist=0.0, d=0; d<user_spatialdim; d++) {
	    diff = loc_user->x[k1++] - loc_user->x[k2++]; 
	    dist += diff * diff;
	    }
	  if (dist>0 && dist<mindelta) mindelta=dist; 
	}
      } // if linesimustep==0.0
      if (user_dim == spDim && loc_user->Time) {
	mindelta += loc_user->T[XSTEP] * loc_user->T[XSTEP];
      }
      mindelta = sqrt(mindelta);
    }
    linesimufactor = tbm_linesimufactor / mindelta;
  } else {
    // both, tbm_linesimustep and tbm_linesimufactor are zeror or negative
    if (*points < BUFFER) { 
      SERR("if linesimufactor and linesimustep are naught then TBM.points must be at least 4 (better between 100 and 10000)");
    }
  }


  /****************************************************************/
  /*          switch to transformed data                          */
  /****************************************************************/


  //  printf("XXXuserCenter %f %f %d\n", user_Center[0], user_Center[1], loc_user->timespacedim);
  //  printf("XXxCenter %f %f\n", Center[0], Center[1]);
  //  printf("XXx Min %f %f\n", user_min[0], user_min[1]);
  //  printf("XXx Max %f %f\n", user_max[0], user_max[1]);
  
  // transform the center
  {    
    cov_model *sub = user;
    double dummy[MAXTBMSPDIM], *aniso;
    int  nrow, ncol;
    if (loc_user->timespacedim > MAXTBMSPDIM)
      SERR("cannot determine centre as dimensions too large");

    MEMCOPY(Center, user_Center, sizeof(double) * loc_user->timespacedim);
    while (true) {      
      if (isDollar(sub)) {
	aniso = getAnisoMatrix(sub, true, &nrow, &ncol);
	if (aniso != NULL) {
	  MEMCOPY(dummy, Center, sizeof(double) * ncol);	
	  xA(dummy, aniso, nrow, ncol, Center);
	  free(aniso);
	}
      }
      if (sub->sub[0] == NULL || sub == cov) break;
      sub = sub->sub[0];
    }
  }

  //  printf("userCenter %f %f %d\n", user_Center[0], user_Center[1],
  //	 loc_user->timespacedim);
  ///  printf("Center %f %f\n", Center[0], Center[1]);
  ///  printf("usermin %f %f\n", user_min[0], user_min[1]);
  //  printf("usermax %f %f\n", user_max[0], user_max[1]);

  //  A PMI(cov, "vorher");

  
  location_type *prevloc = Loc(cov);
  assert(prevloc == cov->prevloc);
  if ((prevloc->Time && !ce_dim2) ||
      (prevloc->grid && prevloc->caniso != NULL)) {
    Transform2NoGrid(cov, ce_dim2, false);

    // APMI(cov);
    //SetLoc2NewLoc(next, Loc(cov));   
  }

  location_type *loc = Loc(cov); 
  int     // totalpoints = loc->totalpoints,
    dim = loc->timespacedim
    //spatialpoints = loc->spatialtotalpoints,
    //    spatialdim = loc->spatialdim
    ;



  // multiplication der x-skalenparameter so dass letztendlich
  // auf der TBM-Geraden mit Abstand 1 der Punkte simuliert werden kann
  // mit dem Vorteil des einfachen Zugriffs auf die simulierten Werte 

  
  /****************************************************************/
  /*          determine length and resolution of the band         */
  /****************************************************************/
  double dummy_center[MAXTBMSPDIM], min_center, max_center;
  
  // print("%ld\n", loc); APMI(cov);
  

  GetDiameter(loc, min, max, dummy_center);
  min_center = max_center = 0.0;
  for (d=0; d<loc->timespacedim; d++) {
    double dummy;
    dummy = Center[d] - min[d];
    min_center += dummy * dummy;
    //printf("d=%d Center=%f min=%f ", d, Center[d], min[d]);
    dummy = Center[d] - max[d];
    max_center += dummy * dummy;    
    //printf("max=%f\n ",  max[d]);
  }
  diameter = 2.0 * sqrt(min_center > max_center ? min_center : max_center);

  //assert(false);

  for (d=0; d<dim; d++) {
    s->center[d] = user_Center[d];
    if (loc->grid || (loc->Time && d==loc->timespacedim-1)) 
      s->center[d] -= loc->xgr[d][XSTART];
  }

  //    print("diam %f pts=%d %f %f\n", diameter, *points, BUFFER, linesimufactor ); //assert(false);
  // 4.231878 0 3.000000 1054.407676


  if (linesimufactor < 0) { // i.e. points given directly by user
      linesimufactor = (*points - BUFFER) / diameter;
  }
  s->linesimufactor = linesimufactor;

  diameter = trunc(BUFFER + diameter * linesimufactor); // in pts

  // print("diam %f %f\n", diameter, linesimufactor);

  if ( *points > 0) { 
    if (diameter > *points) { // never happens if linesimufactor has been < 0
      PRINTF("tbm points minimum=%f, but tbm_points is %d.\n", 
	     diameter,  *points);
      SERR( "given number of points to simulate on the TBM line is too small");
    } else diameter = (double) *points;
  } 

// print("diam %f %f %d %d\n", diameter, linesimufactor, *points, GLOBAL.tbm.points);

  if (diameter > MAXNN) {

    //    printf("diamter = %f %d\n", diameter, MAXNN);
    //    PMI(cov);

    SERR("The number of points on the line is too large. Check your parameters and make sure that none of the locations are given twice");
  } else if (diameter < 10) {
    SERR2("too few points on the line -- modify '%s' or '%s'",
	  KNAME(TBM_LINESIMUFACTOR), KNAME(TBM_LINESIMUSTEP));
  }
  


  // only needed for nongrid !
  s->spatialtotalpts = loc->totalpoints;
  if (s->ce_dim == 2) {
    s->spatialtotalpts /= loc->length[dim - 1];
  }
 

  /****************************************************************/
  /*          set up the covariance function on the line          */
  /****************************************************************/

   if (PL>=PL_STRUCTURE) {
    PRINTF("\nrestructing tbm ");
    PRINTF(ce_dim2 ? "plane...\n" : "line...\n");
  }

  cov_model  *modelB1;

  if (cov->key != NULL) COV_DELETE(&(cov->key));
  if ((err = covcpy(&(cov->key), next)) != NOERROR) return err;
  modelB1 = cov->key;
  while (isGaussProcess(modelB1)) modelB1 = modelB1->sub[0];
  if (modelB1 == cov->key) {
    addModel(&(cov->key), GAUSSPROC);
    modelB1 = cov->key;
  } else {
    modelB1 = modelB1->calling;
  }
 
  addModel(modelB1, 0, DOLLAR);
  if (modelB1->nr==BROWNIAN){
    // &(cov->key))->calling = key;
    kdefault(cov, DVAR, 1.0 + PARAM0(next, BROWN_ALPHA) / tbmdim);
  } else {
    cov_model *dollar = modelB1->sub[0];
    dollar->tsdim = dollar->xdimprev = s->ce_dim;
    kdefault(dollar, DVAR, 1.0);
    PARAMALLOC(dollar, DANISO, s->ce_dim, s->ce_dim);
    PARAM(dollar, DANISO)[0] = 1.0 / linesimufactor;

    if (s->ce_dim == 2) {
      PARAM(dollar, DANISO)[1] = PARAM(dollar, DANISO)[2] = 0.0;
      PARAM(dollar, DANISO)[3] = loc->T[XSTEP];
    } 
    addModel(modelB1, 0, TBM_OP);
    if (!PisNULL(TBMOP_TBMDIM))
      kdefault(modelB1->sub[0], TBMOP_TBMDIM, P0INT(TBM_TBMDIM));
    if (!PisNULL(TBMOP_FULLDIM))
      kdefault(modelB1->sub[0], TBMOP_FULLDIM, P0INT(TBM_FULLDIM));
    if (!PisNULL(TBMOP_LAYERS))
      kdefault(modelB1->sub[0], TBMOP_LAYERS, P0(TBM_LAYERS));
  }
 

  /****************************************************************/
  /*                set up the process on the line                */
  /****************************************************************/

  if (PL>=PL_STRUCTURE) {
    PRINTF("\nsetting up the process on the ");
    PRINTF(ce_dim2 ? "plane..." : "line...");
  }
  
  // xline[XEND]: number of points on the TBM line
  double xline[3];
  xline[XSTART] = 1.0;
  xline[XSTEP] = 1.0;
  s->xline_length  = xline[XLENGTH] = (double) diameter;// diameter > MAXNN must be first since
  //                                risk of overflow !

  // printf("length %f %f\n", diameter, 1.0 / linesimufactor); assert(false);
  // 4465  0.000948


  //printf("xline %f %f %f\n", xline[XSTART], xline[XSTEP], xline[XLENGTH]);
  
  double T[3];
  if (ce_dim2) {
    T[XSTART] = loc->T[XSTART];
    T[XSTEP] = loc->T[XSTEP];
    T[XLENGTH] = (double) loc->length[dim-1];
  } else T[XSTART] = T[XSTEP] = T[XLENGTH] = RF_NA;

  loc_set(xline, T, 1, 1, 3, ce_dim2 /* time */, 
	  true /* grid */, false, &(cov->key->ownloc));
 
  //APMI(cov);

  int ens = GLOBAL.general.expected_number_simu;
  cov_model *key = cov->key;

   GLOBAL.general.expected_number_simu = 100;
  if ((err = CHECK(key, 1 + (int) ce_dim2, 1 + (int) ce_dim2, 
		     ProcessType, XONLY, CARTESIAN_COORD,
		     cov->vdim2, ROLE_GAUSS)) != NOERROR) {
    goto ErrorHandling;  
  }


  //printf("dims %d %d\n", cov->key->xdimown, s->ce_dim);
  assert(key->xdimown == s->ce_dim);

  if ((err = STRUCT(key, NULL)) != NOERROR) goto ErrorHandling;  
  if (!key->initialised) {
    if (key->key != NULL &&
	(err = CHECK(key, 1 + (int) ce_dim2, 1 + (int) ce_dim2, 
		       PosDefType, XONLY, SYMMETRIC,
		       SUBMODEL_DEP, ROLE_GAUSS)) != NOERROR) {
      // PMI(key);
      goto ErrorHandling;  
    }
    if (PL >= PL_DETAILS) {
      PRINTF("\n\nCheckModelInternal (%s, #=%d), after 2nd check:", 
	     NICK(key), key->gatternr); // ok
    }
  }
  
  NEW_STORAGE(stor, STORAGE, gen_storage);

  if ((err = INIT(key, 0, cov->stor)) != NOERROR) goto ErrorHandling;  

  s->err = err;

 ErrorHandling :
  //cov->initialised = err==NOERROR && key->initialised;
  GLOBAL.general.expected_number_simu = ens;
  return err;
}


// aufpassen auf ( * * 0 )
//               ( * * 0 )
//               ( 0 0 1 )


int init_tbmproc(cov_model *cov, gen_storage *S) {
  location_type *loc = Loc(cov);
  int err=NOERROR;
  char errorloc_save[nErrorLoc];
  cov_model *key = cov->key; // == NULL ? cov->sub[0] : cov->key;
  assert(key != NULL);  
  TBM_storage *s = cov->Stbm;
   
  //  PMI(cov);
  //  crash();
  assert(s != NULL);

  strcpy(errorloc_save, ERROR_LOC);
  sprintf(ERROR_LOC, "%s TBM: ", errorloc_save);
  cov->method = TBM;

  ROLE_ASSERT_GAUSS;

  if (s->err==NOERROR) {
    err = INIT(key, 0, S); 
  } else {
    assert(s->err < NOERROR);
  }

  strcpy(ERROR_LOC, errorloc_save);
  if (err != NOERROR) return err; 

  if (loc->distances) return ERRORFAILED;
   
  err = FieldReturn(cov);
  //  APMI(cov);
  cov->simu.active = err == NOERROR;
 
  if (PL>= PL_STRUCTURE) PRINTF("\ntbm is now initialized.\n");

  return err;
  
}


void GetE(int fulldim, TBM_storage *s, int origdim, bool Time, 
	  double *phi, double deltaphi,	 double *aniso,
	  double *offset, double *ex, double *ey, double *ez, double *et) {
  double sube[MAXTBMSPDIM], e[MAXTBMSPDIM];
  int d, j, idx, k,
    spatialdim = s->simuspatialdim;

//      total = spatialdim * dim;
  // dim is the users's dim of the field
  // this might be reduced by zonal anisotropies leading to spatialdim

  //  printf("gete fulldim=%d %d %d\n", fulldim, origdim, spatialdim);

  // for debugging only
  for(d=0; d<MAXTBMSPDIM; d++) sube[d] = e[d] = RF_NEGINF;
  assert(fulldim >= spatialdim); 

  if (fulldim == 2) {
    // no choice between random and non-random
    (*phi) += deltaphi;
    sube[0] = sin(*phi); 
    sube[1] = cos(*phi);
  } 
  
  else if (fulldim == 3) {
    unitvector3D(spatialdim, sube, sube +1, sube + 2);
//    print("spatdim=%d %d  sube=%f  %f %f\n", 
//	   spatialdim, s->simuspatialdim, sube[0], sube[1], sube[2]);
  }

  else ERR("wrong full dimension");

  *offset = 0.5 * s->xline_length;

  //  print("offset %f %f\n", *offset,  s->xline_length);


  if (aniso == NULL) {
    for (d=0; d<spatialdim; d++) e[d] = sube[d];    
  } else {
    for (d=0; d<spatialdim; d++) e[d] = 0.0;
    for (k=j=0; j<spatialdim; j++) {
      for (d=0; d<origdim; d++) {
	e[d] += sube[j] * aniso[k++];
	//      print("e,s,a, (%d, %d) %f     %f     %f\n",
	//	     d, j, e[d], sube[j], aniso[j + d]);
      }
    }
  }
  for (d=0; d<spatialdim; d++) {
    e[d] *= s->linesimufactor;
    *offset -= s->center[d] * e[d];    
    //   printf("d=%d %f sube=%f %f %f\n", d, e[d], sube[d],s->linesimufactor, s->center[d]);
    //  
  }

  //   for (d=0; d<dim; d++) {
  //     print("e, lines %d offset=%f e=%f center=%f %f %d\n", 
  // 	     d, *offset, e[d], 
  // 	     s->center[d],  s->linesimufactor, s->ce_dim);
  //  }

  idx = spatialdim;
  if (Time && s->ce_dim==1) {
    *et = e[--idx];
  }
  switch (idx) {
  case 4 : BUG;
  case 3 : *ez = e[--idx];
  case 2 : *ey = e[--idx];
  case 1 : *ex = e[--idx];
    break;
  default: BUG;
  }
}



void do_tbmproc(cov_model *cov, gen_storage  VARIABLE_IS_NOT_USED *S) { 
  cov_model 
    *key = cov->key; 
  assert(key != NULL); // == NULL ? cov->sub[0] : cov->key;
  location_type 
    *loc = Loc(cov),
    *keyloc = Loc(key);
  TBM_storage *s =  cov->Stbm;

 int vdim = cov->vdim2[0];
 long
    nn = keyloc->length[0],
    ntot = keyloc->totalpoints * vdim,
    totvdim = loc->totalpoints * vdim;



  //printf("%d\n", ntot);// assert(false);
  //  assert(ntot > 4000);
  // APMI(cov->key);

  int
    origdim = loc->caniso == NULL ? cov->tsdim : loc->cani_nrow,
    tsdim = cov->tsdim,
    spatialdim = s->simuspatialdim, // for non-grids only
    every = GLOBAL.general.every,
    tbm_lines = P0INT(TBM_LINES);
   
  res_type
    *res = cov->rf,
    *simuline = key->rf;
  
  assert(res != NULL);
  assert(simuline != NULL);



  double phi, deltaphi, stepx, stepy, stepz, stept, 
    incx=0.0,
    incy=0.0, 
    incz=0.0,
    inct=0.0;
  long n, totpoints;
  int v, nt, idx, gridlent,
    fulldim = P0INT(TBM_FULLDIM);
  bool loggauss = GLOBAL.gauss.loggauss;

  assert(cov->stor != NULL);
     

  // printf("%d %d\n", P0INT(TBM_FULLDIM), s->ce_dim);

  //  APMI(cov);

  for (n=0; n<totvdim; n++) {
    //printf("n=%d %ld\n", (int) n, totvdim);
    res[n]=0.0; 
  }
  deltaphi = PI / (double) tbm_lines; // only used for tbm2
  phi = deltaphi * UNIFORM_RANDOM;     // only used for tbm2

  if (loc->grid) {
    int nx, ny, nz, gridlenx, gridleny, gridlenz;
    long zaehler;
    double xoffset, yoffset, zoffset,  toffset;

    stepx = stepy = stepz = stept =  0.0;
    gridlenx = gridleny = gridlenz = gridlent = 1;
  
    idx = origdim;
    if (s->ce_dim==2) {
      gridlent = loc->length[--idx]; // could be one !!
      stept = loc->xgr[idx][XSTEP];	    
      inct = (double) nn; // wegen assert unten
    } else if (loc->Time) {
      gridlent = loc->length[--idx]; // could be one !!
      stept = loc->T[XSTEP];	    
    }

    switch (idx) {
	case 4 : 
	  BUG;
	case 3 : 
	  gridlenz = loc->length[--idx];
	  stepz = loc->xgr[idx][XSTEP];	  
	  // no break;
	case 2 : 
	  gridleny = loc->length[--idx];
	  stepy = loc->xgr[idx][XSTEP];	 
	  // no break;
	case 1 : 
	  gridlenx = loc->length[--idx];
	  stepx = loc->xgr[idx][XSTEP];	  
	  break;
    default: BUG;
    }
         
    for (n=0; n<tbm_lines; n++) {
      double inittoffset;
      R_CheckUserInterrupt();
      if (every>0  && n > 0 && (n % every == 0)) PRINTF("%d \n",n);

      GetE(fulldim, s, origdim, loc->Time, &phi, deltaphi,
	   loc->caniso, &inittoffset, &incx, &incy, &incz, &inct);
      incx *= stepx;
      incy *= stepy;
      incz *= stepz;
      if (s->ce_dim == 1) inct *= stept;
      inittoffset += UNIFORM_RANDOM - 0.5;
      
      //  PMI(key);
      //  printf("%ld %ld\n", (long int) S, (long int) cov->stor);assert(false);

      DO(key, cov->stor);

//      if (n<5) {
//	  int i;
//	  for (i=0; i<8; i++) print("%2.3f ", simuline[i]);
//	  print("%d \n", ntot);
//      }


      zaehler = 0;
      for (v=0; v<vdim; v++) {
	toffset = inittoffset;
	for (nt=0; nt<gridlent; nt++) {
	  zoffset = toffset;
	  for (nz=0; nz<gridlenz; nz++) {	  
	    yoffset = zoffset;
	    for (ny=0; ny<gridleny; ny++) {	  
	      xoffset = yoffset;
	      for (nx=0; nx<gridlenx; nx++) {
		long longxoffset = (long) xoffset;
		if (!((longxoffset<ntot) && (longxoffset>=0))) {
		  PRINTF("xx ntot %ld %ld %ld -- offsets %f %f %f %f\n x:%d %d y:%d %d z:%d %d t:%d %d\n", 
			 ntot, longxoffset, zaehler,
			 xoffset, yoffset, zoffset, toffset,
			 nx, gridlenx, ny, gridleny, nz, gridlenz, nt, gridlent
			 );
		  //		continue;#
		  //APMI(key);
		  assert((longxoffset<ntot) && (longxoffset>=0) );
		}
				
		res[zaehler++] += simuline[longxoffset];
		xoffset += incx;
	      }
	      yoffset += incy;
	    }
	    zoffset += incz;
	  }
	  toffset += inct;
	}
	inittoffset += s->xline_length;
      } // vdim
    } // n

  } else { 
    //     assert(false);
    // not grid, could be time-model!
    // both old and new form included
    double offset, offsetinit;
    long xi;
    int i, end;

    // PrintMethodInfo(s->key.meth);

#define TBMST(INDEX) {							\
      for (n=0; n<tbm_lines; n++){/*printf("n=%ld tbm.lines%d\n",n,tbm_lines);//*/ \
	R_CheckUserInterrupt();						\
        if (every>0  && (n % every == 0)) PRINTF("%ld \n",n);		\
        GetE(fulldim, s, origdim, loc->Time, &phi, deltaphi,	\
	     loc->caniso, &offsetinit, &incx, &incy, &incz, &inct);		\
        offset += UNIFORM_RANDOM - 0.5; 				\
	DO(key, cov->stor);						\
	i = 0;								\
	end=totpoints;							\
    	for (v=0; v<vdim; v++) {					\
	  offset = offsetinit;						\
	  for (xi = nt=0; nt<gridlent; nt++, end+=totpoints){ \
	    for (; i < end; i++) {					\
	      long index;						\
	      index = (long) (offset + INDEX);				\
	      if (!((index<ntot) && (index>=0))) {			\
		PRINTF("\n%f %f %f (%f %f %f))\n",			\
		       loc->x[xi], loc->x[xi+1] , loc->x[xi+2], incx, incy, incz); \
		PRINTF("n=%ld index=%ld nn=%ld ntot=%ld xi=%ld nt=%d\n OFF=%f IDX=%f inct=%f e=%d\n", \
		       n, index, nn, ntot, xi, (int) nt, offset, INDEX, inct, (int) end); \
	      }								\
	      assert((index<ntot) && (index>=0));			\
 	      res[i] += simuline[index];				\
	      xi += tsdim;						\
	    }								\
	    offset += inct;						\
	  }								\
	  offsetinit += s->xline_length;				\
        }								\
     }									\
   }
    
   if (s->ce_dim == 1) {
      gridlent = 1;
      totpoints = loc->totalpoints;
      inct = RF_NA; // should be set by GetE
    } else { // ce_dim==2 nur erlaubt fuer echte Zeitkomponente
      assert(s->ce_dim==2); 
      gridlent = keyloc->length[1];
      totpoints = s->spatialtotalpts;
      inct = (double) nn;
    }

    //   printf("%d %ld %ld\n", gridlent, nn, ntot);
    
    assert(gridlent * nn * vdim == ntot);
    
    switch (spatialdim) {
    case 1 : //see Martin's techrep f. details
      TBMST(loc->x[xi] * incx);
      break;
    case 2: 
      TBMST(loc->x[xi] * incx + loc->x[xi+1] * incy); //
      break;
    case 3:
      TBMST(loc->x[xi] * incx + loc->x[xi+1] * incy + loc->x[xi+2] * incz);
      break;
    default : BUG;
    }
  } // end not grid
  
//  {double z; int i; for(i=0, z=0.0; i<ntot; i++) z+=simuline[i]*simuline[i];
//      print("%f %f %f\n", 
//	     simuline[0], z / ntot, res[10] / sqrt((double) tbm_lines));
//  }

  long i;
  double InvSqrtNlines;   
  InvSqrtNlines = 1.0 / sqrt((double) tbm_lines);

 
  if (loggauss) {
    for(i=0; i<totvdim; i++) res[i] = (res_type) exp(res[i] * InvSqrtNlines);
  } else {
    for(i=0; i<totvdim; res[i++] *= (res_type) InvSqrtNlines);
  }

//  {double z; int i; for(i=0, z=0.0; i<loc->totalpoints; i++) z+=res[i]*res[i];
//     print("%f %f %f\n", 
//	     simuline[0], z /loc->totalpoints , res[10] );
//  }
  
}

