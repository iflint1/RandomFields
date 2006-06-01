

/* 
 Authors
 Martin Schlather, schlath@hsu-hh.de

 library for simulation of random fields -- get's, set's, and print's

 Copyright (C) 2001 -- 2006 Martin Schlather, 

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
RO
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

extern "C" {
#include <R.h>
#include <Rdefines.h>
}
#include <R_ext/Linpack.h>
#include <math.h>  
#include <stdio.h>  
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "RFsimu.h"
#include "RFCovFcts.h"
#include "MPPFcts.h"


void GetrfParameters(int *covmaxchar, int *methodmaxchar, 
		     int *distrmaxchar,
		     int *covnr, int *methodnr, int *distrnr,
		     int *maxdim, int *maxmodels) {
  if (currentNrCov==-1) InitModelList();
  *covmaxchar=COVMAXCHAR;
  *methodmaxchar=METHODMAXCHAR;
  *distrmaxchar=DISTRMAXCHAR;
  *covnr=currentNrCov;
  *methodnr=(int) Nothing;
  *distrnr=DISTRNR;
  *maxdim=MAXDIM;
  *maxmodels=MAXKEYS;
}


void GetDistrName(int *nr,char **name){
  if ((*nr<0) ||(*nr>=DISTRNR)) {strncpy(*name,"",DISTRMAXCHAR); return;}
  strncpy(*name, DISTRNAMES[*nr], DISTRMAXCHAR);
}

void GetDistrNr(char **name, int *n, int *nr) 
{
  unsigned int ln, v, nn;
  nn = (unsigned int) *n;
  // == -1 if no matching function is found
  // == -2 if multiple matching fnts are found, without one matching exactly
  // if more than one match exactly, the last one is taken (enables overwriting 
  // standard functions)

  for (v=0; v<nn; v++) {
    nr[v]=0;
    ln=strlen(name[v]);
    while ( (nr[v] < DISTRNR) && strncmp(name[v], DISTRNAMES[nr[v]], ln)) {
      (nr[v])++;
    }
    if (nr[v]<DISTRNR) { 
      // a matching function is found. Are there other functions that match?
      int j; 
      bool exactmatching,multiplematching;
      exactmatching=(ln==strlen(DISTRNAMES[nr[v]]));
      multiplematching=false;
      j=nr[v]+1; // if two or more distributions have the same name 
      //            the last one is taken 
      while (j<DISTRNR) {
	while ( (j<DISTRNR) && strncmp(name[v],DISTRNAMES[j],ln)) {j++;}
	if (j<DISTRNR) {
	  if (ln==strlen(DISTRNAMES[j])) {nr[v]=j; exactmatching=true;} 
	  else {multiplematching=true;}
	}
	j++;
      } 
      if (!exactmatching && multiplematching) {nr[v]=-2;}
    } else nr[v]=-1;
  }
}

void SetParamDecision(int *action, int *stationary_only, int *exactness)
{
  int NA=-1;
  if (*action) {
    DECISION_PARAM.stationary_only = (*stationary_only==NA) ?
       DECISION_CASESPEC : *stationary_only ? DECISION_TRUE : DECISION_FALSE;
    DECISION_PARAM.exactness = (*exactness==NA) ?
      DECISION_CASESPEC : *exactness ? DECISION_TRUE : DECISION_FALSE;
  } else {
    *stationary_only = DECISION_PARAM.stationary_only==DECISION_TRUE ? true :
      DECISION_PARAM.stationary_only==DECISION_FALSE ? false : NA;
    *exactness = DECISION_PARAM.exactness==DECISION_TRUE ? true :
      DECISION_PARAM.exactness==DECISION_FALSE ? false : NA;
  }
}

void SetParamCE(int *action, int *force, double *tolRe, double *tolIm,
		int *trials, int *severalrealisations,
		double *mmin, // mmin must be vector of length MAXDIM!!
		int *useprimes, int *strategy, double *maxmem,
		int *dependent, ce_param *CE, char *name)
{
  int d;
  if (*action) {
    CE->force=(bool)*force;

    CE->tol_re=*tolRe;
    if (CE->tol_re>0) {
      CE->tol_re=0;
      if (GENERAL_PRINTLEVEL>0)
	PRINTF("\nWARNING! %s.tol_re which has been positive is set to 0.\n",name);
    }

    CE->tol_im=*tolIm; 
    if (CE->tol_im<0) {
      CE->tol_im=0; 
      if (GENERAL_PRINTLEVEL>0) 
	PRINTF("\nWARNING! %s.tol_im which has been negative is set 0.\n",name);
    }

    CE->severalrealisations=(bool) *severalrealisations; 

    CE->trials=*trials; 
    if (CE->trials<1) {
      CE->trials=1;
      if (GENERAL_PRINTLEVEL>0) 
	PRINTF("\nWARNING! %s.trials had been less than 1\n",name);
    }

    for (d=0; d<MAXDIM; d++) {
      CE->mmin[d] = mmin[d];
      if (CE->mmin[d]<0.0) {
	if ((CE->mmin[d]>-1.0) && (GENERAL_PRINTLEVEL>0)) {
	    CE->mmin[d] = -1.0;
	    PRINTF("\nWARNING! %s.mmin[%d] set to -1.0.\n");
	}
      }
    }
 
    CE->useprimes=(bool)*useprimes;
    CE->dependent=(bool)*dependent;

    if (*strategy>LASTSTRATEGY) {  
      if (GENERAL_PRINTLEVEL>0) 
	PRINTF("\nWARNING! %s_STRATEGY not set\n",name);
    } else CE->strategy=(char) *strategy;     

    CE->maxmem = *maxmem;
  } else {
    *force=CE->force;
    *tolRe=CE->tol_re;
    *tolIm=CE->tol_im;
    *trials=CE->trials;
    *severalrealisations = CE->severalrealisations;
    for (d=0; d<MAXDIM; d++) {mmin[d]=CE->mmin[d];}   
    *useprimes= CE->useprimes; //
    *dependent= (int) CE->dependent;
    *strategy = (int) CE->strategy;
    *maxmem = CE->maxmem; //
  }
}
  

void GetModelName(int *nr,char **name){
  if (currentNrCov==-1) InitModelList();
  if ((*nr<0) ||(*nr>=currentNrCov)) {strncpy(*name,"",COVMAXCHAR); return;}
  strncpy(*name,CovList[*nr].name,COVMAXCHAR);
}

void GetNrParameters(int *covnr, int *n, int* dim, int* kappas) {
  int v;
  if (currentNrCov==-1) InitModelList();
  for (v=0; v<*n; v++) {
    if ((covnr[v]<0) ||(covnr[v]>=currentNrCov)) {kappas[v]=-1;}
    else {
//	printf("%d %s %d %d %d %d\n", 
//	       v, CovList[covnr[v]].name, *dim, (int) CovList[covnr[v]].kappas,
//	       (int) kappaOne, CovList[covnr[v]].kappas(*dim));
	kappas[v] = CovList[covnr[v]].kappas(*dim);
    }
  }
}

void GetModelNr(char **name, int *n, int *nr) 
{
  unsigned int ln, v, nn;
  nn = (unsigned int) *n;
  // == -1 if no matching function is found
  // == -2 if multiple matching fctns are found, without one matching exactly
  // if more than one match exactly, the last one is taken (enables overwriting 
  // standard functions)
  if (currentNrCov==-1) InitModelList();

  for (v=0; v<nn; v++) {
    nr[v]=0;
    ln=strlen(name[v]);
    while ( (nr[v]<currentNrCov) && strncmp(name[v],CovList[nr[v]].name,ln)) {
      (nr[v])++;
    }
    if (nr[v]<currentNrCov) { 
      // a matching function is found. Are there other functions that match?
      int j; 
      bool exactmatching,multiplematching;
      exactmatching=(ln==strlen(CovList[nr[v]].name));
      multiplematching=false;
      j=nr[v]+1; // if two or more covariance functions have the same name 
      //            the last one is taken 
      while (j<currentNrCov) {
	while ( (j<currentNrCov) && strncmp(name[v],CovList[j].name,ln)) {j++;}
	if (j<currentNrCov) {
	  if (ln==strlen(CovList[j].name)) {nr[v]=j; exactmatching=true;} 
	  else {multiplematching=true;}
	}
	j++;
      } 
      if (!exactmatching && multiplematching) {nr[v]=-2;}
    } else nr[v]=-1;
  }
}

void GetMethodNr(char **name, int *n, int *nr) 
{
  unsigned int ln, v, nn;
  nn = (unsigned int) *n;
  // == -1 if no matching method is found
  // == -2 if multiple matching methods are found, without one matching exactly
  for (v=0; v<nn; v++) {
    nr[v]=0;
    ln=strlen(name[v]);
    while ( (nr[v]<(int) Forbidden) && strncmp(name[v],METHODNAMES[nr[v]],ln)) {
      (nr[v])++;
    }
    if (nr[v]<(int) Forbidden) { 
      // a matching method is found. Are there other methods that match?
      int j; 
      bool exactmatching,multiplematching;
      exactmatching=(ln==strlen(METHODNAMES[nr[v]]));
      multiplematching=false;
      j=nr[v]+1; // if two or more methods have the same name the last one is
      //          taken; stupid here, but nice in GetCovNr
      while (j<(int) Forbidden) {
	while ( (j<(int) Forbidden) && strncmp(name[v],METHODNAMES[j],ln)) j++;
	if (j<(int) Forbidden) {
	  if (ln==strlen(METHODNAMES[j])) {nr[v]=j; exactmatching=true;} 
	  else {multiplematching=true;}
	}
	j++;
      } 
      if (!exactmatching && multiplematching) {nr[v]=-2;}
    } else nr[v]=-1;
  }
}

void GetMethodName(int *nr, char **name) 
{   
  if ((*nr<0) ||(*nr>=(int) Forbidden)) {
    strncpy(*name,"",METHODMAXCHAR); 
    return;
  }
  strncpy(*name,METHODNAMES[*nr],METHODMAXCHAR);
}

void PrintMethods()
{ 
  int i;
  PRINTF("\n\n  Methods for generating Gaussian random fields\n  =============================================\n\n");  
  for (i=0;i<(int) Nothing;i++) { PRINTF("  * %s\n",METHODNAMES[i]); }
  PRINTF("\n\n  Methods for non-Gaussian random fields\n  ======================================\n");
  for (i=1+(int) Nothing;i<(int) Forbidden; i++) { 
    PRINTF("  * %s\n",METHODNAMES[i]); }
  PRINTF("\n");
}

void PrintCovDetailed(int i)
{
  PRINTF("%5d: %s cov=%d, tbm=%d %d, spec=%d,\n abf=%d hyp=%d, oth=%d %d\n",
	 i,CovList[i].name,CovList[i].cov,
	 CovList[i].cov_tbm2,CovList[i].cov_tbm3,
	 CovList[i].spectral,CovList[i].add_mpp_scl,CovList[i].hyperplane,
	 CovList[i].initother,CovList[i].other);
}

void PrintModelListDetailed()
{
  int i;
  if (CovList==NULL) {PRINTF("There are no functions available!\n");} 
  else {
    PRINTF("\nList of covariance functions:\n=============================\n");
    for (i=0;i<currentNrCov;i++) PrintCovDetailed(i);
  }
}

void GetRange(int *nr, int *dim, int *index, double *range, int *lrange){
  // never change double without crosschecking with fcts in RFCovFcts.cc!
  // index is increased by one except index is the largest value possible
  int i;
  if (currentNrCov==-1) InitModelList();
  if ((*nr<0) || (*nr>=currentNrCov) || 
      (*lrange != CovList[*nr].kappas(*dim) * 4) ) {
    for (i=0; i<*lrange; i++) range[i]=RF_NAN;
    *index = -100;
    return;
  }
  assert(CovList[*nr].range != NULL);
  CovList[*nr].range(*dim, index, range);
  for (i=0; i<*lrange; i+=3) {
      if (range[i] == floor(range[i]) + OPEN) range[i] = floor(range[i]);
      i++;
      if (range[i] == ceil(range[i]) - OPEN) range[i] = ceil(range[i]);
  }
  // index>0 : further parts of the range are missing
  // index=-1: no further part of the range, see also RFsimu.h
  // index=-2: dimension not valid
  //assert(false);
}

int kappaZero(int dim) {return 0;}
int kappaOne(int dim) {return 1;}
int kappaTwo(int dim) {return 2;}
int kappaThree(int dim) {return 3;}
int kappaFour(int dim) {return 4;}
int kappaFive(int dim) {return 5;}
int kappaSix(int dim) {return 6;}
int kappaSeven(int dim) {return 7;}
int kappaFalse(int dim) { assert(false); return 0;}

int createmodel(char *name, int kappas, kappas_type kappas_fct, 
		int type, infofct info, bool variogram, 
		rangefct range) {
  int i;
  if (currentNrCov==-1) InitModelList(); assert(CovList!=NULL);
  assert(currentNrCov>=0);
  if (currentNrCov>=MAXNRCOVFCTS) 
    {PRINTF("Error. Model list full.\n");return -1;}

  strncpy(CovList[currentNrCov].name, name, COVMAXCHAR-1);
  CovList[currentNrCov].name[COVMAXCHAR-1]='\0';
  if (strlen(name)>=COVMAXCHAR) {
    PRINTF("Warning! Covariance name is truncated to `%s'",
	   CovList[currentNrCov].name);
  }

  CovList[currentNrCov].exception = 0;
  assert(kappas < 0 xor kappas_fct == NULL);
  CovList[currentNrCov].kappas = 
      kappas < 0  ? kappas_fct :
      kappas == 0 ? kappaZero :
      kappas == 1 ? kappaOne :
      kappas == 2 ? kappaTwo :
      kappas == 3 ? kappaThree :
      kappas == 4 ? kappaFour :
      kappas == 5 ? kappaFive :
      kappas == 6 ? kappaSix :
      kappas == 7 ? kappaSeven : kappaFalse;
  CovList[currentNrCov].type = type;
  assert((CovList[currentNrCov].info=info) != NULL); 
  CovList[currentNrCov].variogram = variogram;
  assert((CovList[currentNrCov].range= range) != NULL);
  CovList[currentNrCov].check = NULL;
  CovList[currentNrCov].checkNinit = NULL; // hyper and local ce
  for (i=0; i<SimulationTypeLength; i++)
    CovList[currentNrCov].implemented[i] = NOT_IMPLEMENTED;

  CovList[currentNrCov].naturalscale=NULL;
  CovList[currentNrCov].cov=NULL;
  CovList[currentNrCov].localtype = Nothing; // local ce
  CovList[currentNrCov].getparam = NULL;

  CovList[currentNrCov].derivative=NULL;
  CovList[currentNrCov].secondderivt=NULL;

  CovList[currentNrCov].cov_tbm2=NULL;
  CovList[currentNrCov].cov_tbm3=NULL;
  CovList[currentNrCov].spectral=NULL;

  CovList[currentNrCov].add_mpp_scl=NULL;  
  CovList[currentNrCov].add_mpp_rnd=NULL;
  CovList[currentNrCov].hyperplane=NULL;
  CovList[currentNrCov].initother=NULL;
  CovList[currentNrCov].other=NULL;
  currentNrCov++;
  return(currentNrCov-1);
}

int IncludeHyperModel(char *name, int kappas, checkhyper check,
			     int type, bool variogram, infofct info,
			     rangefct range)
{  
    int nr;
    nr = createmodel(name, kappas, NULL, type, info, variogram, range);
    assert((CovList[nr].checkNinit = check) != NULL);
    return nr;
}
int IncludeHyperModel(char *name, kappas_type kappas, checkhyper check,
		      int type, bool variogram, infofct info,
		      rangefct range)
{  
    int nr;
    assert(type!=ISOTROPIC && type!=ISOHYPERMODEL);
    nr = createmodel(name, -1, kappas, type, info, variogram, range);
    assert((CovList[nr].checkNinit = check) != NULL);
    return nr;
}


extern int IncludeModel(char *name, int kappas, checkfct check,
			int type, bool variogram, infofct info,
			rangefct range) 
{  
    int nr;
    nr = createmodel(name, kappas, NULL, type, info, variogram, range);
    assert((CovList[nr].check = check) != NULL);
    return nr;
}
extern int IncludeModel(char *name, kappas_type kappas, checkfct check,
			int type, bool variogram, infofct info,
			rangefct range) 
{  
    int nr;
    assert(type!=ISOTROPIC && type!=ISOHYPERMODEL);
    nr = createmodel(name, -1, kappas, type, info, variogram, range);
    assert((CovList[nr].check = check) != NULL);
    return nr;
}


void addCov(int nr, covfct cov, isofct derivative,
		   natscalefct naturalscale)
{
  assert((nr>=0) && (nr<currentNrCov) && cov!=NULL);
  CovList[nr].cov=cov;
  CovList[nr].derivative=derivative;
  CovList[nr].naturalscale=naturalscale;
  CovList[nr].implemented[CircEmbed] =
    CovList[nr].implemented[Direct] = !CovList[nr].variogram;
}

void modelexception(int nr, bool scale, bool aniso) {
    CovList[nr].exception = (int) scale + 2 * (int) aniso;
}

void GetModelException(int *nr, int *n, int *scale, int*aniso) {
    int i, Nr;
    for (i=0; i<*n; i++) {
	Nr = nr[i]; scale[i] = Nr % 2;
	Nr >>= 1;   aniso[i] = Nr % 2;  
    }
}

void addLocal(int nr, bool cutoff, isofct secondderiv, int*variable)
{
  assert((nr>=0) && (nr<currentNrCov) && CovList[nr].derivative!=NULL);
  CovList[nr].secondderivt=secondderiv;
  assert(variable != NULL);
  assert(CovList[nr].derivative != NULL);
  *variable = nr;
  CovList[nr].implemented[CircEmbedCutoff] = cutoff;
  CovList[nr].implemented[CircEmbedIntrinsic] = secondderiv != NULL;
  // printf("**** %d %s %d %d\n", 
//	 nr, CovList[nr].name, secondderiv, &DDexponential);
}

extern void addInitLocal(int nr, getparamfct getparam,
			 alternativeparamfct alternative, LocalType type)
{
  assert((nr>=0) && (nr<currentNrCov));
  CovList[nr].localtype = type;
  CovList[nr].getparam = getparam;
  CovList[nr].alternative = alternative;
  assert(nlocal < nLocalCovList);
  LocalCovList[nlocal++] = nr; 
  switch(type) {
    case CircEmbed : case CircEmbedCutoff : case CircEmbedIntrinsic : 
	CovList[nr].implemented[type] = HYPERIMPLEMENTED; 
	// printf("init %s %d %d %d\n",CovList[nr].name, CovList[nr].implemented[type], type, CircEmbedCutoff);
	break;
    default : assert(false);
  }
}

extern void addTBM(int nr, isofct cov_tbm2, isofct cov_tbm3,
		   randommeasure spectral) {
  // must be called always AFTER addCov !!!!
  int index=1;
  double range[4 * LASTKAPPA];
  assert((nr>=0) && (nr<currentNrCov));
  CovList[nr].cov_tbm2=cov_tbm2;
  CovList[nr].range(2, &index, range);
  assert(index != RANGE_INVALIDDIM);
  if (cov_tbm2 != NULL) {
    CovList[nr].implemented[TBM2] = IMPLEMENTED;
    assert(CovList[nr].derivative!=NULL); 
    // IMPLEMENTED must imply the NUM_APPROX to simply the choice
    // between COV_TBM2 and Cov_TBM2Num
  } else {
    CovList[nr].implemented[TBM2] = (CovList[nr].derivative!=NULL) 
	? NUM_APPROX : NOT_IMPLEMENTED;
  }
  CovList[nr].cov_tbm3=cov_tbm3;
  CovList[nr].range(3, &index, range);
  if (cov_tbm3!=NULL || cov_tbm2!=NULL) assert(CovList[nr].derivative!=NULL);
  CovList[nr].implemented[TBM3] = (index != RANGE_INVALIDDIM) && 
      (CovList[nr].cov_tbm3!=NULL || 
       (CovList[nr].derivative!=NULL && CovList[nr].cov!=NULL));
  CovList[nr].spectral=spectral;
  CovList[nr].implemented[SpectralTBM] = spectral!=NULL;
}
	
extern void addOther(int nr, initmppfct add_mpp_scl, MPPRandom add_mpp_rnd, 
		     hyper_pp_fct hyper_pp,
		     generalSimuInit initother, generalSimuMethod other){
  assert((nr>=0) && (nr<currentNrCov));
  CovList[nr].add_mpp_scl=add_mpp_scl;  
  CovList[nr].add_mpp_rnd=add_mpp_rnd;  
  if (add_mpp_scl!=NULL || add_mpp_rnd!=NULL)
    assert(add_mpp_scl!=NULL && add_mpp_rnd!=NULL);
  CovList[nr].implemented[AdditiveMpp] = add_mpp_scl!=NULL;
  CovList[nr].hyperplane=hyper_pp;
  CovList[nr].implemented[Hyperplane] = hyper_pp!=NULL;
  CovList[nr].initother=initother;
  CovList[nr].other=other;
  if ((other!=NULL) || (initother!=NULL)) 
    assert((other!=NULL) && (initother!=NULL));
}

void PrintModelList()
{
  int i, last_method, m;
  char percent[]="%";
  char header[]="Circ cut intr TBM2 TBM3 spec drct nugg add hyp oth\n";
  char coded[5][2]={"-", "X", "+", "o", "h"};
  char firstcolumn[20], empty[5][5]={"", " ", "  ", "   ", "    "};
  int empty_idx[(int) Nothing] = {1, 2, 2, 2, 2, 2, 2, 1, 1, 1, 0};

  last_method = (int) Nothing;    
  if (currentNrCov==-1) {
    InitModelList(); 
    if (GENERAL_PRINTLEVEL>5) 
      PRINTF("List of covariance functions initiated.\n"); 
  }
  if (CovList==NULL) {PRINTF("There are no functions available!\n");} 
  else {
    sprintf(firstcolumn,"%s%ds ",percent, COVMAXCHAR);
    PRINTF("\n\n");
    PRINTF(firstcolumn,empty[0]); PRINTF("       List of models\n");
    PRINTF(firstcolumn,empty[0]); PRINTF("       ==============\n");
    PRINTF(firstcolumn,empty[0]); PRINTF("[See also PrintMethodList()]\n\n");
    PRINTF(firstcolumn,empty[0]);PRINTF(header,empty[0]);  
    for (i=0;i<currentNrCov;i++) {
      PRINTF(firstcolumn, CovList[i].name);
      for (m=(int) CircEmbed; m<(int) Nothing; m++)
	PRINTF("%3s%s", coded[(int) CovList[i].implemented[m]], 
	       empty[empty_idx[m]]);
      PRINTF("\n");
    }
    PRINTF(firstcolumn,empty[0]);PRINTF(header,empty[0]);  
    PRINTF("Legend:");
    PRINTF("'-': method not available\n");
    PRINTF("'X': method available for at least some parameter values\n");
    PRINTF("'+': parts are evaluated only approximatively\n");
    PRINTF("'o': given method is ignored and an alternative one is used\n");
    PRINTF("'h': available only as internal model within the method \n");
  }
}

void GetModelList(int* idx) {
  int i, j, m;
  if (currentNrCov==-1) {
    InitModelList(); 
    if (GENERAL_PRINTLEVEL>5) 
      PRINTF("List of covariance functions initiated.\n"); 
  }
  if (CovList==NULL) return;
  for (j=i=0; i<currentNrCov; i++)
    for (m=(int) CircEmbed; m<(int) Nothing; m++)
      idx[j++] = CovList[i].implemented[m];
  return;
}

void GetNaturalScaling(int *covnr, double *q, int *naturalscaling,
		       double *natscale, int *error)
{ // called also by R 

  //  q: kappas only

  // values of naturalscaling:
  //#define NATSCALE_EXACT 1   
  //#define NATSCALE_APPROX 2
  //#define NATSCALE_MLE 3 /* check fitvario when changing !! */
  // +10 if numerical is allowed
  static int oldcovnr = -99;
  static param_type oldp, p;
  static double OldNatScale;
  int TEN, k;
  bool numeric;  
  *error = 0;
  TEN = 10;

  for (k=KAPPA; k<=LASTKAPPA; k++) p[k]=q[k-KAPPA];
  if (*naturalscaling) {	 
    if (CovList[*covnr].type!=ISOTROPIC) {
      *error=ERRORANISOTROPIC; return;
    }
    if (!(numeric=*naturalscaling>TEN)) {TEN=0;}
    *natscale=0.0;
    if (CovList[*covnr].naturalscale!=NULL) { 
      *natscale = CovList[*covnr].naturalscale(p,*naturalscaling-TEN);
      if (*natscale!=0.0) return;
    }
    if (numeric) {
      double x,newx,yold,y,newy;
      covfct cov;
      int parami, wave,i;
      if ((cov=CovList[*covnr].cov)==NULL) {*error=ERRORNOTDEFINED;return;}
      if ((cov=CovList[*covnr].cov)==nugget) {*error=ERRORRESCALING; return;}
      
      // already calculated ?
      parami=KAPPA; // do not compare mean,variance, etc.
      if (oldcovnr==*covnr) {
	for (; parami<=LASTKAPPA; parami++) {
	  if (oldp[parami]!=p[parami]) {
	    break;
	  }
	}
	if (parami > LASTKAPPA) {
	  *natscale=OldNatScale; 
	  return;
	}
      }   

      // the other parameters need not to be copied as checked that 
      // they are identical to previous ones
      for (; parami<=LASTKAPPA; parami++) {oldp[parami]=p[parami];}
      oldp[EFFECTIVEDIM] = 1.0;
      // oldp[VARIANCE] = oldp[ANISO] = 1.0; not necessary to set explicitly, 
      // but used in this sense in the following
      oldcovnr = -99; /* if error occurs, the next call will realise that 
			 the previous result cannot be used */

      /* **************************************
	 Now, find a quick and good solution for NewInvScale --
      */
      wave  = 0;
      x = 1.0; 
      if ( (yold=cov(&x, oldp)) > 0.05) {
	double leftx;
	x *= 2.0;
	while ( (y=cov(&x, oldp)) > 0.05) {  
	  if (yold<y){ wave++;  if (wave>10) {*error=ERRORWAVING; return;} }
	  yold = y;
	  x *= 2.0;
	  if (x>1E30) {*error=ERRORRESCALING; return;} // or use a separate ERROR
	} 
	leftx = x * 0.5;
	for (i=0; i<3 /* good choice?? */ ;i++) {          
	  if (y==yold) {*error=ERRORWAVING; return;} // should never appear
	  newx = x + (x-leftx)/(y-yold)*(0.05-y);
	  if ( (newy=cov(&newx, oldp)) >0.05) {
	    leftx = newx;
	    yold  = newy;
	  } else {
	    x = newx;
	    y = newy;
	  }
	}
	if (y==yold)  {*error=ERRORWAVING; return;} // should never appear
	*natscale = 1.0 / ( x + (x-leftx)/(y-yold)*(0.05-y) );
      } else {
	double rightx;
	x *= 0.5;
	while ( (y=cov(&x, oldp)) < 0.05) {  
	  if (yold>y){ wave++;  if (wave>10) {*error=ERRORWAVING; return;} }
	  yold = y;
	  x *= 0.5;
	  if (x<1E-30) {*error=ERRORRESCALING; return;} //or use a separate ERROR
	}    
	rightx = x * 2.0;
	for (i=0; i<3 /* good choice?? */ ;i++) {          
	  if (y==yold) {*error=ERRORWAVING; return;} // should never appear
	  newx = x + (x-rightx)/(y-yold)*(0.05-y);
	  if ( (newy=cov(&newx, oldp)) <0.05) {
	    rightx = newx;
	    yold   = newy;
	  } else {
	    x = newx;
	    y = newy;
	  }
	}
	if (y==yold)  {*error=ERRORWAVING; return;} // should never appear
	*natscale = 1.0 / ( x + (x-rightx)/(y-yold)*(0.05-y) );
      }
      oldcovnr=*covnr;  /* result is reliable -> stored for next call */
      OldNatScale = *natscale;
    } else {*error=ERRORRESCALING; }
  } else {
    *natscale = 1.0;
  }
}
void SetParam(int *action, int *storing,int *printlevel,int *naturalscaling,
	      char **pch, int *skipchecks) {
  if (*action) {
    GENERAL_STORING= (bool) *storing;
    GENERAL_PRINTLEVEL=*printlevel;
    GENERAL_NATURALSCALING=*naturalscaling;
    if ((strlen(*pch)>1) && (GENERAL_PRINTLEVEL>0)) 
	PRINTF("\n`pch' has more than one character -- first character taken only\n");
    strncpy(GENERAL_PCH, *pch, 1);
    GENERAL_SKIPCHECKS=*skipchecks;
  } else {
    *storing =  GENERAL_STORING;
    *printlevel = GENERAL_PRINTLEVEL;
    *naturalscaling = GENERAL_NATURALSCALING;
    strcpy(*pch, GENERAL_PCH);
    *skipchecks = GENERAL_SKIPCHECKS;
  }
}


void StoreTrend(int *keyNr, int *modus, char **trend, 
		int *lt, double *lambda, int *ll, int *error)
{
  unsigned long len;
  key_type *key;
  if (currentNrCov==-1) InitModelList(); assert(CovList!=NULL); 
  if ((*keyNr<0) || (*keyNr>=MAXKEYS)) {*error=ERRORREGISTER;goto ErrorHandling;}

  key = &(KEY[*keyNr]);
  DeleteKeyTrend(key);
  switch(*modus){
      case TREND_MEAN : //0
	key->mean = *lambda;
	assert(*lt==0 && *ll==1);
	break;
      case TREND_PARAM_FCT : case TREND_FCT: // 3, 1
	key->LinearTrend = (double *) malloc(len = sizeof(double) * *ll);
	memcpy(key->LinearTrend, lambda, len);
	if (*modus==1) break; 
	key->lLinTrend = *ll;
      case TREND_LINEAR : //2
	key->TrendFunction = (char *) malloc(len = sizeof(char) * *lt);
	memcpy(key->TrendFunction, *trend, len);
	key->lTrendFct = *lt;
	break; 
      default: key->TrendModus = -1; *error=ERRORUNSPECIFIED; goto ErrorHandling;
  }
  key->TrendModus = *modus;
  *error = 0;
  return;

 ErrorHandling:
  return;
}



void GetParamterPos(int *variance, int *kappa, int* lastkappa, 
		    int *hyperinternal, int *lasthyperinternal,
		    int *scale, int *aniso, int *hypernr, int *localdiameter,
		    int *local_r, int *cutoff_theo_r, int *hyperkappa,
		    int *total) {
    *variance = VARIANCE;
    *kappa = KAPPA;
    *lastkappa = LASTKAPPA;
    *hyperinternal = HYPERINTERNALI;
    * lasthyperinternal = LASTHYPERINTERNAL;
    *scale = SCALE;
    *aniso = ANISO;
    *hypernr = HYPERNR;
    *localdiameter = DIAMETER;
    *local_r = LOCAL_R;
    *cutoff_theo_r = CUTOFF_THEOR;
    *hyperkappa = HYPERKAPPAI;
    *total = TOTAL_PARAM;
}




void Getxsimugr(coord_type x, param_type param, int timespacedim, 
	   bool anisotropy, double *xsimugr) {
  // bisher nur fuer Diagonalmatrizen 
  int n, k, i, w;
  double factor;
  
  for(n=ANISO, k=w=0; w<timespacedim; w++, n+=timespacedim+1) {
//    printf("n=%d %f %f \n", n, param[n], param[SCALE]);
    factor = anisotropy ? param[n] : 1.0 / param[SCALE];
    for (i=0; i<3; i++) {
      xsimugr[k++] = factor * x[w][i];
    }
  }
}

void matrixrotat(double *paramaniso, int col, int row,
		 int *truedim, aniso_type aniso) {
  double G[MAXDIM+1], matrix[MAXDIMSQ], e[MAXDIM], D[MAXDIM], V[MAXDIMSQ];
  int i, j, TrueDim, rowsq, job=01, err, index[MAXDIM];
  for (i=0; i<row; i++) D[i] = V[i] = 0.0;
  rowsq = row * row;
  for (; i<rowsq; V[i++]=0.0);
  if (is_diag(paramaniso, row)) {
      int diag = row + 1, size = row * col;
      for (j=i=0; i<size; i+=diag, j++) {
	  D[j] = paramaniso[i]; /// WICHTIG! Dadurch wird Ordnung
	  //                           der Dimensionen eingehalten!!
	  // siehe auch CheckAndBuild
	  V[i] = 1.0;
      }
  } else {
      for (j=0; j<col; j++)
	  for (i=0; i<row; i++)
	      matrix[i * row + j] = paramaniso[i + j * row];
      for (; j<row; j++)
	  for (i=0; i<row; i++) 
	      matrix[i * row + j] = 0.0;
      
      // dsvdc destroys the input matrix !!!!!!!!!!!!!!!!!!!!
      F77_NAME(dsvdc)(matrix, &row, &row, &row, D, e, NULL /* U */,
		      &row, V, &row, G, &job, &err);
      if (err!=NOERROR) {
	  PRINTF("F77 error in GetTrueDim: %d\n", err);
	  assert(false);
      }
  }
  
  for (TrueDim=i=j=0; j<row; j++)
      if (fabs(D[j]) > EIGENVALUE_EPS) {
	  index[i++] = j;
	  TrueDim++;
      }
  
  for (j=0; j<TrueDim; j++) 
      for (i=0; i<row; i++) {
	  aniso[i + j * row] =//note V, not V^T is returned by dsvdc !
	      V[i + index[j] * row] * D[index[j]];
      }
  for ( ; j<row; j++) for (i=0; i<row; i++)  aniso[i + j * row] = 0.0;
  *truedim = TrueDim;
}

void GetTrueDim(bool anisotropy, int timespacedim, param_type param,
		char type, bool *genuine_last_dim, int *reduceddim, 
		aniso_type aniso) {
  // returns always the a (reduced) matrix; if not anisotropy then diag matrix
  // of full size with elements 1/SCALE. 
  int i,j;
  // int dummytruedim;
  // aniso_type dummyaniso;

  assert(timespacedim>0);
  if (anisotropy) {
    int col, row, endfor;
    // row: rows of aniso matrix and of param
    // col: number colums of param used for "matrix" (i.e. time is excluded for
    //                                      space isotropy), see switch below
    col = row = timespacedim;
    endfor = ANISO + row * col; 
    
    //  *genuine_last_dim = DIM_GENUINE;
    for (j=endfor - row; j<endfor; j++) {
      if (param[j] != 0.0) break;
    }
//    if (j != endfor) *genuine_last_dim = DIM_ZERO;
    *genuine_last_dim = j != endfor;

    switch (type) {
	case ANISOTROPIC: case ANISOHYPERMODEL :
	    for (j=0; j<col; j++)
		for (i=0; i<row; i++)
		    aniso[i + j * row] = param[ANISO + i + j * row];
	    *reduceddim = timespacedim;
	    break;
	case SPACEISOTROPIC :
	    matrixrotat(&(param[ANISO]), col - 1, row, reduceddim, aniso);
	    int param_segm, aniso_segm;
	    param_segm = ANISO + row * (col - 1);
	    aniso_segm = *reduceddim * row;
	    for (i=0; i<row; i++) {
		aniso[aniso_segm + i] = param[param_segm + i];
//		printf("> %d %f ", param_segm + i, aniso[aniso_segm + i]);
	    }
//	    for (i=0; i<TOTAL_PARAM; i++) printf("i=%d %f\n", i, param[i]);
	    (*reduceddim)++;
//	    printf("%d %d\n", param_segm, aniso_segm); 
	    break;
	case ISOTROPIC: case ISOHYPERMODEL:
	    matrixrotat(&(param[ANISO]), col, row, reduceddim, aniso);
	    break;
	default: assert(false);
    }
  } else { 
    *genuine_last_dim = true;
    *reduceddim = timespacedim; // == spatialdim
    double invscale;
    int endfor;
    invscale = 1.0 / param[SCALE];
    j = timespacedim + 1;
    endfor = timespacedim * timespacedim;
    for (i=0; i<endfor; aniso[i++]=0.0); 
    for (i=0; i<endfor; i+=j) aniso[i] = invscale; 
  }

//  printf("gettrue aniso %f\n", aniso[0]);
}


void GetTrendLengths(int *keyNr, int *modus, int *lt, int *ll, int *lx)
{
  // currently unused, since AddTrend in RFsimu.cc is being programmed
  assert(false);

  key_type *key;
  if (currentNrCov==-1) {*modus=-1;goto ErrorHandling;}
  if ((*keyNr<0) || (*keyNr>=MAXKEYS)) {*modus=-2; goto ErrorHandling;}
  key = &(KEY[*keyNr]);
  if (!key->active) {*modus=-3; goto ErrorHandling;}
  *modus = key->TrendModus;
  *lt = key->lTrendFct;
  *ll = key->lLinTrend;
  *lx = key->totalpoints;
  return;
 ErrorHandling:
  return;
}


void GetTrend(int *keyNr, char **trend, double *lambda, double *x, int *error)
{
  // currently unused, since AddTrend in RFsimu.cc is being programmed
  assert(false);
  
  key_type *key;
  if (currentNrCov==-1) {*error=-1;goto ErrorHandling;}
  if ((*keyNr<0) || (*keyNr>=MAXKEYS)) {*error=-2; goto ErrorHandling;}
  key = &(KEY[*keyNr]);
  if (!key->active) {*error=-3; goto ErrorHandling;}
  switch(key->TrendModus){
  case 0 : break;
  case 3 : case 1 : 
    memcpy(lambda, key->LinearTrend, key->lLinTrend);
    if (key->TrendModus==1) break; 
  case 2:
    strcpy(*trend, key->TrendFunction);
    break; 
  default: *error=-4; goto ErrorHandling;
  }
  *error = 0;
  return;

 ErrorHandling:
  return;
}


void GetCoordinates(int *keyNr, double *x, int *error)
{
  key_type *key;
  if (currentNrCov==-1) {*error=-1;goto ErrorHandling;}
  if ((*keyNr<0) || (*keyNr>=MAXKEYS)) {*error=-2; goto ErrorHandling;}
  key = &(KEY[*keyNr]);
  if (!key->active) {*error=-3; goto ErrorHandling;}
  if (key->TrendModus==0) {*error=-4; goto ErrorHandling;}
 
  *error = ERRORNOTPROGRAMMED;  // TODO !!

  return;

 ErrorHandling:
  return;
}


void GetKeyInfo(int *keyNr, int *total, int *lengths, int *spatialdim, 
		int *timespacedim, int *grid, int *distr, int *maxdim)
{
    // check with DoSimulateRF and subsequently with extremes.cc, l.170
    // if any changings !
  int d;
  key_type *key;
  if (*maxdim<MAXDIM) { *total=-3; *maxdim=MAXDIM; return;} 
  if ((*keyNr<0) || (*keyNr>=MAXKEYS)) {*total=-1; return;} 
  key = &(KEY[*keyNr]);  
  if (!key->active) {*total=-2;} 
  else { 
    *total=key->totalpoints;
    *spatialdim = key->spatialdim;
    *timespacedim = key->timespacedim;
    for (d=0; d<MAXDIM; d++) lengths[d]=key->length[d];
    *grid = (int) key->grid;
    *distr = (int) key->distribution; 
    *maxdim = MAXDIM;
   }
}


SEXP Logi(bool* V, int n, int max, long *mem) {
  int i;
  SEXP dummy;
  (*mem) += n * sizeof(bool);
  if (n>max) return TooLarge(&n, 1);
  PROTECT(dummy=allocVector(LGLSXP, n));
  for (i=0; i<n; i++) LOGICAL(dummy)[i] = V[i];
  UNPROTECT(1);
  return dummy;
}

SEXP Num(double* V, int n, int max, long *mem) {
  int i;
  SEXP dummy;
  (*mem) += n * sizeof(double);
  if (n>max) return TooLarge(&n, 1);
  PROTECT(dummy=allocVector(REALSXP, n));
  for (i=0; i<n; i++) REAL(dummy)[i] = V[i];
  UNPROTECT(1);
  return dummy;
}

SEXP Int(int *V, int n, int max, long *mem) {
  int i;
  SEXP dummy;
  (*mem) += n * sizeof(int);
  if (n>max) return TooLarge(&n, 1);
  PROTECT(dummy=allocVector(INTSXP, n));
  for (i=0; i<n; i++) INTEGER(dummy)[i] = V[i];
  UNPROTECT(1);
  return dummy;
}

SEXP Mat(double* V, int row, int col, int max, long *mem) {
  int i, n;
  n = row * col;
  (*mem) += n * sizeof(double);
  if (n>max) {
      int nn[2];
      nn[0] = row;
      nn[1] = col;
      return TooLarge(nn, 2);
  }
  SEXP dummy;
  PROTECT(dummy=allocMatrix(REALSXP, row, col));
  for (i=0; i<n; i++) REAL(dummy)[i] = V[i];
  UNPROTECT(1);
  return dummy;
}


SEXP TooLarge(int *n, int l){
#define nTooLarge 2 // mit op
  char *tooLarge[nTooLarge] = {"size", "msg"};
  int i;
  long mem=0;
  SEXP namevec, info;
  PROTECT(info=allocVector(VECSXP, nTooLarge));
  PROTECT(namevec = allocVector(STRSXP, nTooLarge));
  for (i=0; i<nTooLarge; i++) SET_VECTOR_ELT(namevec, i, mkChar(tooLarge[i]));
  setAttrib(info, R_NamesSymbol, namevec);
  i=0;
  SET_VECTOR_ELT(info, i++, Int(n, l, l, &mem));
  SET_VECTOR_ELT(info, i,
		 mkString("too many elements - increase max.elements"));
  UNPROTECT(2);
  return info;
}


SEXP GetModelInfo(covinfo_arraytype keycov, int nc, int totalparam, 
		  long totalpoints)
{
#define nmodelinfo 15 // mit op
  char *modelinfo[nmodelinfo] = 
      {"method", "dim", "truedim", "length", "aniso.idx",
       "name", "op" /* must keep name */, 
       "genuine_dim", "genuine.last.dim", 
       "simugrid", "processed", "param", "aniso", "x", "xsimugr"};
  SEXP model,  submodel[MAXCOV], namemodelvec[2];
  int i, exception, k, subi;
  long mem;
  covinfo_type *kc;

  // printf("0X\n");
  mem = 0;
  PROTECT(model = allocVector(VECSXP, nc));
  for (exception=0; exception <= 1; exception++) {
    int j;
    //  printf("%d \n", exception);
    PROTECT(namemodelvec[exception] = allocVector(STRSXP, nmodelinfo-exception));
    for (j=k=0; k<nmodelinfo; k++) {
	//     printf("%d %d\n", exception, k);
      if (exception && !strcmp("op", modelinfo[k])) continue;
      //   printf("a%d %d %s \n", exception, k, modelinfo[k]);
      SET_VECTOR_ELT(namemodelvec[exception], j++, mkChar(modelinfo[k]));
      //  printf("X%d %d\n", exception, k);
    }
  }
  nc--;
  for (i=0; i<=nc; i++) {
      // printf("0X\n");
    kc = &(keycov[i]);
    exception = i==nc;
    PROTECT(submodel[i] = allocVector(VECSXP, nmodelinfo - exception));
    setAttrib(submodel[i], R_NamesSymbol, namemodelvec[exception]);

    subi = 0;
    SET_VECTOR_ELT(submodel[i],subi++, mkString(METHODNAMES[kc->method]));
    SET_VECTOR_ELT(submodel[i], subi++, ScalarInteger(kc->dim));
    SET_VECTOR_ELT(submodel[i], subi++, ScalarInteger(kc->reduceddim));
    SET_VECTOR_ELT(submodel[i], subi++,
		   Int(kc->length, kc->simugrid ? kc->dim : 0, MAX_INT, &mem));
    // printf("X\n");
    SET_VECTOR_ELT(submodel[i], subi++, 
		   Int(kc->idx, kc->simugrid ? kc->dim :0, MAX_INT, &mem));
    // printf("Xa\n");
    SET_VECTOR_ELT(submodel[i], subi++, mkString(CovList[kc->nr].name));
    if (!exception) 
	SET_VECTOR_ELT(submodel[i], subi++, mkString(OP_SIGN[kc->op]));
    SET_VECTOR_ELT(submodel[i], subi++,
		   Logi(kc->genuine_dim, kc->simugrid ? kc->dim : 0, MAX_INT, 
			&mem));
    SET_VECTOR_ELT(submodel[i], subi++, 
		   ScalarLogical(kc->genuine_last_dimension));
    SET_VECTOR_ELT(submodel[i], subi++, ScalarLogical(kc->simugrid));
    SET_VECTOR_ELT(submodel[i], subi++, ScalarLogical(!kc->left));
    SET_VECTOR_ELT(submodel[i], subi++, (totalparam < 0) ? allocVector(VECSXP, 0)
		   : Num(kc->param, totalparam, MAX_INT, &mem));
    SET_VECTOR_ELT(submodel[i], subi++, (kc->dim<=0) ? allocVector(VECSXP, 0)
		   : Mat(kc->aniso, kc->dim, // kc->reduceddim
			 kc->dim, MAX_INT, &mem)
                    // should be matrix
	);
    //printf("Xb\n");
    if (kc->x == NULL) 
	SET_VECTOR_ELT(submodel[i], subi++, allocVector(VECSXP, 0));
    else
	SET_VECTOR_ELT(submodel[i], subi++, 
		       Mat(kc->x, kc->simugrid ? 3 : totalpoints, 
			   kc->reduceddim, MAX_INT, &mem));
    SET_VECTOR_ELT(submodel[i], subi++, 
		   Mat(kc->xsimugr, kc->simugrid ? 3 : 0, kc->dim, MAX_INT, &mem));

    assert(subi==nmodelinfo - (i==nc)); 
    SET_VECTOR_ELT(model, i, submodel[i]);
    UNPROTECT(1);
  }
  UNPROTECT(3); // model + namemodelvec
  return model;
}


SEXP GetMethodInfo(key_type *key, methodvalue_arraytype keymethod, 
		   bool ignore_active, int depth, int max) 
{
#define nmethodinfo 7 // mit op
  char *methodinfo[nmethodinfo] = 
    {"name", "in.use", "not.addable", "actcov", "covnr", "allocated", "mem"};
  SEXP submethod[MAXCOV], namemethodvec, method, nameSvec, S;
  int i, k, subi, tsdim;
  long totpts, mem;

  totpts = key->totalpoints;
  tsdim = key->timespacedim;
  PROTECT(method = allocVector(VECSXP, key->n_unimeth));
  PROTECT(namemethodvec = allocVector(STRSXP, nmethodinfo));
  for (k=0; k<nmethodinfo; k++)
       SET_VECTOR_ELT(namemethodvec, k, mkChar(methodinfo[k]));
  for (i=0; i<key->n_unimeth; i++) {
    mem = 0;
    methodvalue_type *meth;
    meth = &(key->meth[i]);
    PROTECT(submethod[i] = allocVector(VECSXP, nmethodinfo));
    setAttrib(submethod[i], R_NamesSymbol, namemethodvec);

    subi = 0;
    SET_VECTOR_ELT(submethod[i], subi++, mkString(METHODNAMES[meth->unimeth]));
    SET_VECTOR_ELT(submethod[i], subi++, 
		   ScalarLogical(meth->unimeth_alreadyInUse));
    SET_VECTOR_ELT(submethod[i], subi++, 
		   ScalarLogical(meth->incompatible));
    SET_VECTOR_ELT(submethod[i], subi++, ScalarInteger(meth->actcov));
    SET_VECTOR_ELT(submethod[i], subi++, 
		   Int(meth->covlist, meth->actcov, MAX_INT, &mem));
    
    int nS, nSlist[Forbidden + 1] =
	{12 /* CE */, 2 /*Cutoff*/, 2 /* Intr */, 10 /* TBM2 */, 10 /* TBM3 */, 
	 1 /*Sp */, 3 /* dir */, 4 /* nug */, 10 /* add */, 4 /* hyp */, 
	 1 /* spec */, 1 /* noth */, 11 /* maxmpp */, 4 /*extremalGauss */,
	 1 /* Forbidden */};
    assert(Forbidden == 14);
    assert(meth->unimeth < Forbidden);
    nS = (meth->S!=NULL && depth<=1) ? nSlist[meth->unimeth] : 0;
    PROTECT(S = allocVector(VECSXP, nS));
    PROTECT(nameSvec = allocVector(STRSXP, nS));
    k = 0;
    if (nS != 0) switch (meth->unimeth) {
	case CircEmbed : {
	    CE_storage* s;
	    s = (CE_storage*) meth->S;
	    SET_VECTOR_ELT(nameSvec, k, mkChar("size"));
	    SET_VECTOR_ELT(S, k++, Int(s->m, tsdim, MAX_INT, &mem));
	    SET_VECTOR_ELT(nameSvec, k, mkChar("totalsize"));
	    SET_VECTOR_ELT(S, k++, ScalarInteger(s->mtot));
	    SET_VECTOR_ELT(nameSvec, k, mkChar("trials"));
	    SET_VECTOR_ELT(S, k++, ScalarInteger(s->trials));
	    SET_VECTOR_ELT(nameSvec, k, mkChar("positivedefinite"));
	    SET_VECTOR_ELT(S, k++, ScalarLogical(s->positivedefinite));
	    SET_VECTOR_ELT(nameSvec, k, mkChar("smallest.neg.real"));
	    SET_VECTOR_ELT(S, k++, ScalarReal(s->smallestRe));
	    SET_VECTOR_ELT(nameSvec, k, mkChar("largest.abs.imag"));
	    SET_VECTOR_ELT(S, k++, ScalarReal(s->largestAbsIm));
	    SET_VECTOR_ELT(nameSvec, k, mkChar("fft"));
	    SET_VECTOR_ELT(S, k++, s->c == NULL ? ScalarLogical(false)
			   : Mat(s->c, 2, s->mtot, max, &mem));	    
	    SET_VECTOR_ELT(nameSvec, k, mkChar("invfft"));
	    SET_VECTOR_ELT(S, k++, s->d == NULL ? ScalarLogical(false)
			   : Mat(s->d, 2, s->mtot, max, &mem));	    
	    SET_VECTOR_ELT(nameSvec, k, mkChar("next_new"));
	    SET_VECTOR_ELT(S, k++, ScalarLogical(s->new_simulation_next));
	    SET_VECTOR_ELT(nameSvec, k, mkChar("curSimuPosition"));
	    SET_VECTOR_ELT(S, k++, Int(s->cur_square, tsdim, MAX_INT, &mem));
	    SET_VECTOR_ELT(nameSvec, k, mkChar("simupositions"));
	    SET_VECTOR_ELT(S, k++, Int(s->max_squares, tsdim, MAX_INT, &mem));
	    SET_VECTOR_ELT(nameSvec, k, mkChar("segmentLength"));
	    {
	      int dummy[MAXDIM], i;
	      for (i=0; i< tsdim; i++) 
		  dummy[i] = s-> square_seg[i] / s->cumm[i];
	      SET_VECTOR_ELT(S, k++, Int(dummy, tsdim, MAX_INT, &mem));
	    }
	    } break;
	case CircEmbedCutoff : case CircEmbedIntrinsic : {
	    localCE_storage* s;
	    s = (localCE_storage*) meth->S;
	    SET_VECTOR_ELT(nameSvec, k, mkChar("correctionTerms"));
	    {
	      SEXP corr;
	      int v;
	      PROTECT(corr=allocVector(VECSXP, s->key.ncov));
	      for (v=0; v < s->key.ncov; v++) {
		SET_VECTOR_ELT(corr, v, s->correction[v] == NULL 
			       ? ScalarLogical(false) 
			       : (CovList[s->key.cov[v].nr].localtype == 
				  CircEmbedIntrinsic 
				  ? Mat((double*)s->correction[v], tsdim, tsdim,
					MAX_INT, &mem)
				  :  mkChar("error")
				   )
		    );
	      }
	      UNPROTECT(1);
	      SET_VECTOR_ELT(S, k++, corr);	
	    }
	    SET_VECTOR_ELT(nameSvec, k, mkChar("new"));
	    SET_VECTOR_ELT(S, k++, InternalGetKeyInfo(&(s->key), ignore_active,
						      depth + 1, max));
	    
	} break;
	case TBM2: case TBM3 : {
	    TBM_storage* s;
	    s = (TBM_storage*) meth->S;
	    SET_VECTOR_ELT(nameSvec, k, mkChar("aniso"));
	    SET_VECTOR_ELT(S, k++, 
			   Mat(s->aniso, tsdim, s->reduceddim, MAX_INT, &mem));
	    SET_VECTOR_ELT(nameSvec, k, mkChar("simugrid"));
	    SET_VECTOR_ELT(S, k++, ScalarLogical(s->simugrid));	
	    SET_VECTOR_ELT(nameSvec, k, mkChar("simuspatialdim"));
	    SET_VECTOR_ELT(S, k++, ScalarInteger(s->simuspatialdim));	
	    SET_VECTOR_ELT(nameSvec, k, mkChar("ce_dim"));
	    SET_VECTOR_ELT(S, k++, ScalarInteger(s->ce_dim));
	    SET_VECTOR_ELT(nameSvec, k, mkChar("reduceddim"));
	    SET_VECTOR_ELT(S, k++, ScalarInteger(s->reduceddim));		
	    SET_VECTOR_ELT(nameSvec, k, mkChar("center"));
	    SET_VECTOR_ELT(S, k++, Num(s->center, s->simuspatialdim,
				       MAX_INT, &mem));
	    SET_VECTOR_ELT(nameSvec, k, mkChar("x"));
	    SET_VECTOR_ELT(S, k++, (s->x == NULL) 
			   ? allocVector(VECSXP, 0)
			   : Mat(s->x, s->reduceddim, s->simugrid ? 3 : totpts, 
				 max, &mem));
	    SET_VECTOR_ELT(nameSvec, k, mkChar("xsimgr"));
	    SET_VECTOR_ELT(S, k++, Mat(s->xsimugr, s->simugrid ? 3 : 0,
				       s->timespacedim, MAX_INT, &mem));
	    SET_VECTOR_ELT(nameSvec, k, mkChar("simuline"));
	    SET_VECTOR_ELT(S, k++, s->simuline==NULL 
			   ? allocVector(VECSXP,0) 
			   : (s->ce_dim==1 
			      ? Num(s->simuline, s->key.length[0], max, &mem)
			      : Mat(s->simuline, s->key.length[0], 
				    s->key.length[1], max, &mem)
			     )
	      );
	    SET_VECTOR_ELT(nameSvec, k, mkChar("new"));
	    SET_VECTOR_ELT(S, k++, InternalGetKeyInfo(&(s->key), ignore_active,
						      depth + 1, max));
	} break;
	case SpectralTBM: {
	    spectral_storage* s;
	    s = (spectral_storage*) meth->S;
	    SET_VECTOR_ELT(nameSvec, k, mkChar("AmplitudeOK"));
	    SET_VECTOR_ELT(S, k++, ScalarLogical(s->randomAmplitude[0] != NULL));
	} break;
	case Direct : {
	    direct_storage* s;
	    s = (direct_storage*) meth->S;
	    char *invm[NoFurtherInversionMethod + 1] =
		{"Cholesky", "SVD", "None"};
	    SET_VECTOR_ELT(nameSvec, k, mkChar("invmethod"));
	    SET_VECTOR_ELT(S, k++, (s->method < NoFurtherInversionMethod) ?
			   mkString(invm[s->method]) :  ScalarLogical(false));
	    SET_VECTOR_ELT(nameSvec, k, mkChar("sqrtCov"));
	    SET_VECTOR_ELT(S, k++, s->U == NULL ? ScalarLogical(false)
			   : Mat(s->U, totpts, totpts, max, &mem));
	    SET_VECTOR_ELT(nameSvec, k, mkChar("dummy"));
	    SET_VECTOR_ELT(S, k++, s->G == NULL ? ScalarLogical(false)
			   : Num(s->G, totpts + 1, max, &mem));	    
	    
	}  break;
	case Nugget : {
	    nugget_storage* s;
	    s = (nugget_storage*) meth->S;
	    SET_VECTOR_ELT(nameSvec, k, mkChar("simple"));
	    SET_VECTOR_ELT(S, k++, ScalarLogical(s->simple));	
	    SET_VECTOR_ELT(nameSvec, k, mkChar("simugrid"));
	    SET_VECTOR_ELT(S, k++, ScalarLogical(s->simugrid));	
	    SET_VECTOR_ELT(nameSvec, k, mkChar("srqtnugget"));
	    SET_VECTOR_ELT(S, k++, ScalarReal(s->sqrtnugget));	
	    SET_VECTOR_ELT(nameSvec, k, mkChar("internalsort"));
	    SET_VECTOR_ELT(S, k++, 
			     Int(s->pos, (s->pos == NULL) ? 0 : totpts, max, &mem));	
	  }  break;
	case AdditiveMpp : {
	    mpp_storage* s;
	    s = (mpp_storage*) meth->S;
	    SET_VECTOR_ELT(nameSvec, k, mkChar("integral"));
	    SET_VECTOR_ELT(S, k++, ScalarReal(s->integral));
	    SET_VECTOR_ELT(nameSvec, k, mkChar("integralsq"));
	    SET_VECTOR_ELT(S, k++, ScalarReal(s->integralsq));
	    SET_VECTOR_ELT(nameSvec, k, mkChar("effectiveRadius"));
	    SET_VECTOR_ELT(S, k++, ScalarReal(s->effectiveRadius));
	    SET_VECTOR_ELT(nameSvec, k, mkChar("effectivearea"));
	    SET_VECTOR_ELT(S, k++, ScalarReal(s->effectivearea));
	    SET_VECTOR_ELT(nameSvec, k, mkChar("addradius"));
	    SET_VECTOR_ELT(S, k++, ScalarReal(s->addradius));
	    SET_VECTOR_ELT(nameSvec, k, mkChar("min"));
	    SET_VECTOR_ELT(S, k++, Num(s->min, tsdim, MAX_INT, &mem));	    
	    SET_VECTOR_ELT(nameSvec, k, mkChar("length"));
	    SET_VECTOR_ELT(S, k++, Num(s->length, tsdim, MAX_INT, &mem)); 
	    SET_VECTOR_ELT(nameSvec, k, mkChar("internal.constants"));
	    SET_VECTOR_ELT(S, k++, Num(s->c, 6, MAX_INT, &mem)); 
	    SET_VECTOR_ELT(nameSvec, k, mkChar("primitiveFctOK"));
	    SET_VECTOR_ELT(S, k++, ScalarLogical(s->MppFct != NULL));
	    SET_VECTOR_ELT(nameSvec, k, mkChar("dim"));
	    SET_VECTOR_ELT(S, k++, ScalarInteger(s->dim));
	}  break;
	case Hyperplane : {
	    hyper_storage* s;
	    s = (hyper_storage*) meth->S;
	    SET_VECTOR_ELT(nameSvec, k, mkChar("rx"));
	    SET_VECTOR_ELT(S, k++, Num(s->rx, tsdim, MAX_INT, &mem)); 
	    SET_VECTOR_ELT(nameSvec, k, mkChar("center"));
	    SET_VECTOR_ELT(S, k++, Num(s->center, tsdim, MAX_INT, &mem)); 
	    SET_VECTOR_ELT(nameSvec, k, mkChar("radius"));
	    SET_VECTOR_ELT(S, k++, ScalarReal(s->radius));	
	    SET_VECTOR_ELT(nameSvec, k, mkChar("HyperplaneFctOK"));
	    SET_VECTOR_ELT(S, k++, ScalarLogical(s->hyperplane != NULL));
	} break;
	case MaxMpp : {
	    mpp_storage* s;
	    s = (mpp_storage*) meth->S;
	    SET_VECTOR_ELT(nameSvec, k, mkChar("integralpos")); 
	    SET_VECTOR_ELT(S, k++, ScalarReal(s->integralpos));
	    SET_VECTOR_ELT(nameSvec, k, mkChar("factor"));
	    SET_VECTOR_ELT(S, k++, ScalarReal(s->factor));
	    SET_VECTOR_ELT(nameSvec, k, mkChar("maxheight"));
	    SET_VECTOR_ELT(S, k++, ScalarReal(s->maxheight));	    
	    SET_VECTOR_ELT(nameSvec, k, mkChar("effectiveRadius"));
	    SET_VECTOR_ELT(S, k++, ScalarReal(s->effectiveRadius));
	    SET_VECTOR_ELT(nameSvec, k, mkChar("effectivearea"));
	    SET_VECTOR_ELT(S, k++, ScalarReal(s->effectivearea));
	    SET_VECTOR_ELT(nameSvec, k, mkChar("addradius"));
	    SET_VECTOR_ELT(S, k++, ScalarReal(s->addradius));
	    SET_VECTOR_ELT(nameSvec, k, mkChar("min"));
	    SET_VECTOR_ELT(S, k++, Num(s->min, tsdim, MAX_INT, &mem));	    
	    SET_VECTOR_ELT(nameSvec, k, mkChar("length"));
	    SET_VECTOR_ELT(S, k++, Num(s->length, tsdim, MAX_INT, &mem)); 
	    SET_VECTOR_ELT(nameSvec, k, mkChar("internal.constants"));
	    SET_VECTOR_ELT(S, k++, Num(s->c, 6, MAX_INT, &mem)); 
	    SET_VECTOR_ELT(nameSvec, k, mkChar("primitiveFctOK"));
	    SET_VECTOR_ELT(S, k++, ScalarLogical(s->MppFct != NULL));
	    SET_VECTOR_ELT(nameSvec, k, mkChar("dim"));
	    SET_VECTOR_ELT(S, k++, ScalarInteger(s->dim));
	} break;
	case ExtremalGauss : {
	  SEXP dummy, array;
	  int i, dim;
	  extremes_storage* s;
	  s = (extremes_storage*) meth->S;
	  SET_VECTOR_ELT(nameSvec, k, mkChar("inv_mean_pos"));
	  SET_VECTOR_ELT(S, k++, ScalarReal(s->inv_mean_pos));
	  SET_VECTOR_ELT(nameSvec, k, mkChar("assumedmax"));
	  SET_VECTOR_ELT(S, k++, ScalarReal(s->assumedmax));
	  SET_VECTOR_ELT(nameSvec, k, mkChar("field")); 
	  
	  dim = key->grid ? tsdim : 1;
	  if (dim==1) {
	    SET_VECTOR_ELT(S, k++, Num(s->rf, totpts, max, &mem));	
	  } else {
	    PROTECT(dummy=allocVector(INTSXP, dim));
	    for (i=0; i<dim; i++) INTEGER(dummy)[i] = key->length[i];
	    PROTECT(array=allocArray(REALSXP, dummy));
	    for (i=0; i<totpts; i++) REAL(array)[i] = s->rf[i];
	    SET_VECTOR_ELT(S, k++, array);
	    UNPROTECT(2);
	  }
	  SET_VECTOR_ELT(nameSvec, k, mkChar("new"));
	  SET_VECTOR_ELT(S, k++, InternalGetKeyInfo(&(s->key), ignore_active,
						    depth + 1, max));
	} break;
	default :  
	    SET_VECTOR_ELT(nameSvec, k, mkChar("calling.method"));
	    SET_VECTOR_ELT(S, k++, ScalarLogical(meth->unimeth));
	    
    }
    assert(k==nS);
 
    
    setAttrib(S, R_NamesSymbol, nameSvec);
    SET_VECTOR_ELT(submethod[i], subi++, ScalarInteger(mem));
    SET_VECTOR_ELT(submethod[i], subi++, S);
    UNPROTECT(2); // method + namemethodvec
 
    assert(subi==nmethodinfo); 
    SET_VECTOR_ELT(method, i, submethod[i]);
    UNPROTECT(1);
  }
  UNPROTECT(2); // method + namemethodvec
  return method;
}

SEXP InternalGetKeyInfo(key_type *key, bool ignore_active, int depth, int max)
{
#define ninfo 16
  char *infonames[ninfo] = 
    {"active" /* must be first */, "stop",
       "anisotropy", "compatible", "grid", "time.comp",
       "distrib.", "timespacedim", "totalparam", "spatial.dim", "trend.mode", 
       "spatial.pts", "total.pts", "mean", "model", "method" 
    };
  int ni, actninfo, i;
  SEXP info, namevec; 

//  printf("max=%f", (double) max); 

  actninfo = (key->active || ignore_active) ? ninfo : 1;
  PROTECT(info=allocVector(VECSXP, actninfo));
  PROTECT(namevec = allocVector(STRSXP, actninfo));
  for (i=0; i<actninfo; i++) SET_VECTOR_ELT(namevec, i, mkChar(infonames[i]));
  setAttrib(info, R_NamesSymbol, namevec);
  ni = 0;
  SET_VECTOR_ELT(info, ni++, ScalarLogical(key->active));
  if (actninfo > 1) {
    SET_VECTOR_ELT(info, ni++, ScalarLogical(key->stop)); 
    SET_VECTOR_ELT(info, ni++, ScalarLogical(key->anisotropy)); 
    SET_VECTOR_ELT(info, ni++, ScalarLogical(key->compatible)); 
    SET_VECTOR_ELT(info, ni++, ScalarLogical(key->grid)); 
    SET_VECTOR_ELT(info, ni++, ScalarLogical(key->Time)); 

    SET_VECTOR_ELT(info, ni++, mkString(DISTRNAMES[key->distribution]));
    SET_VECTOR_ELT(info, ni++, ScalarInteger(key->timespacedim));
    SET_VECTOR_ELT(info, ni++, ScalarInteger(key->totalparam));
    //
    SET_VECTOR_ELT(info, ni++, ScalarInteger(key->spatialdim));
    //
    SET_VECTOR_ELT(info, ni++, ScalarInteger(key->TrendModus));
    //
    SET_VECTOR_ELT(info, ni++, ScalarInteger((int) key->spatialtotalpoints));
    SET_VECTOR_ELT(info, ni++, ScalarInteger((int) key->totalpoints));
    //
    SET_VECTOR_ELT(info, ni++, ScalarReal(key->mean));
    //
    SET_VECTOR_ELT(info, ni++, GetModelInfo(key->cov, key->ncov, key->totalparam,
					    key->totalpoints));
    SET_VECTOR_ELT(info, ni++, 
		   GetMethodInfo(key, key->meth, ignore_active, depth, max));
  }
  assert(ni==actninfo);
  UNPROTECT(2); // info + name
  return info;
}

SEXP GetExtKeyInfo(SEXP keynr, SEXP Ignoreactive, SEXP max_elements) 
{ // extended
    int knr;   
    knr = INTEGER(keynr)[0];
    if ((knr<0) || (knr>=MAXKEYS)) {
	return allocVector(VECSXP, 0);
    }
    return InternalGetKeyInfo(&(KEY[knr]), LOGICAL(Ignoreactive)[0], 0,
			      INTEGER(max_elements)[0]);
}


void GetCornersOfGrid(key_type *key, int Stimespacedim, double *aniso, 
		      double *sxx)
{
  // returning sequence (#=2^MAXDIM) vectors of dimension s->simutimespacedim
  double sx[ZWEIHOCHMAXDIM * MAXDIM];
  int index,i,k,g,endforM1,l,n;
  long endfor;
  char j[MAXDIM];      
  endfor = 1 << key->timespacedim;
  endforM1 = endfor -1;
  for (i=0; i<key->timespacedim; i++) j[i]=0;
  for (index=i=0; i<endfor; i++) {
    for (k=0; k<key->timespacedim; k++) {
      sx[index] = key->x[k][XSTART];
      if (j[k]) 
	sx[index] += key->x[k][XSTEP] * (double) (key->length[k] - 1);
      index++;
    }
    k=0; (j[k])++;
    if (i!=endforM1)
      while(j[k]>=2) { assert(j[k]==2); j[k]=0;  k++; (j[k])++;}	
  }
  endfor *= key->timespacedim;
  for (index=l=0; index<endfor; index+=key->timespacedim) {
    for (n=k=0; k<Stimespacedim; k++) {
      register double dummy;
      dummy = 0.0;
      for (g=0; g<key->timespacedim; g++) 
	dummy += aniso[n++] * sx[index + g];
      sxx[l++] = dummy;
    }
  }
}

void GetCornersOfElement(double *x[MAXDIM], int timespacedim, 
			 covinfo_type *keycov, double *sxx) {
  double sx[ZWEIHOCHMAXDIM * MAXDIM];
  int index,i,k,g,n,endforM1,l;
  long endfor;
  char j[MAXDIM];      
  endfor = 1 << timespacedim;
  endforM1 = endfor -1;
  for (i=0; i<timespacedim; i++) j[i]=0;
  for (index=i=0; i<endfor; i++) {
    for (k=0; k<timespacedim; k++) {
      if (j[k]) sx[index] = x[k][XSTEP]; else sx[index]=0.0;
      index++;
    }
    k=0; (j[k])++;
    if (i!=endforM1)
      while(j[k]>=2) { assert(j[k]==2); j[k]=0;  k++; (j[k])++;}	
  }
  endfor *= timespacedim;
  for (index=l=0; index<endfor; index+=timespacedim) {
    n = 0;
    for (k=0; k<keycov->reduceddim; k++) {
      register double dummy;
      dummy = 0.0;
      for (g=0; g<timespacedim; g++) 
        dummy += keycov->aniso[n++] * sx[index + g];
      sxx[l++] = dummy;
    }
  }
}


void GetCenterAndDiameter(key_type *key, bool simugrid, int simuspatialdim, 
			  int reduceddim, double *x, aniso_type aniso,
			  double *center, double *lx, double *diameter)
{
    // center and length of surrounding rectangle of the simulation area
    // length of diagonal of the rectangle
  double distsq;
  int d;  

  distsq = 0.0; // max distance
  if (simugrid) {
    // diameter of the grid
    for (d=0; d<simuspatialdim; d++) {
      lx[d] = x[XENDD[d]] - x[XSTARTD[d]];
      center[d] = 0.5 * lx[d] + x[XSTARTD[d]];
      //       printf("getcenteranddiam %d %f %f %d %f %f %f\n", d, center[d], 
//	       x[XSTARTD[d]], key->length[d], x[XSTEPD[d]], x[XENDD[d]], lx[d]);
      distsq += lx[d] * lx[d];
    }
  } else { // not simugrid
    double min[MAXDIM], max[MAXDIM], *xx, sxx[ZWEIHOCHMAXDIM * MAXDIM]; 
    int endfor, i;
    for (d=0; d<MAXDIM; d++) {min[d]=RF_INF; max[d]=RF_NEGINF;}
    if (key->grid) { // key->grid     
      GetCornersOfGrid(key, reduceddim, aniso, sxx);      
      endfor = (1 << key->timespacedim) * reduceddim;
      xx = sxx;
    } else { // not key->grid
      endfor = key->totalpoints * reduceddim;
      // determine componentwise min and max (for the diameter)
      xx = x;
    }
    // to determine the diameter of the grid determine as approxmation
    // componentwise min and max corner
    for (i=0; i<endfor; ) {
      for (d=0; d<reduceddim; d++, i++) {
        //temporal part need not be considered, but for ease included
	if (xx[i]<min[d]) min[d] = xx[i];
	if (xx[i]>max[d]) max[d] = xx[i];
      }
    }
    // max distance, upperbound
    distsq = 0.0;
    for (d=0; d< simuspatialdim; d++) { // not here no time component!
//	printf("%d min %f max %f\n", d, min[d], max[d]);
      lx[d] = max[d] - min[d];
      distsq += lx[d] * lx[d];
      center[d] = 0.5 * (max[d] + min[d]); 
    }
  } // !simugrid
  *diameter = sqrt(distsq);
//  printf("*dimater=%f\n", *diameter);
}


void GetRangeCornerDistances(key_type *key, double *sxx, int Stimespacedim,
			     int Ssimuspatialdim, double *min, double *max) {
  long endfor,indexx,index;
  int d,i,k;
  endfor = 1 << key->timespacedim;
  *min = RF_INF;
  *max = RF_NEGINF;
  for (index=i=0; i<endfor; i++, index+=Stimespacedim)
    for (indexx=k=0; k<i; k++, indexx+=Stimespacedim) {
      register double sum = 0.0, dummy;
      for (d=0; d<Ssimuspatialdim; d++) {
	dummy = sxx[index+d] - sxx[indexx+d];
	sum += dummy * dummy;
      }
      if ((sum > 0) && (sum < *min)) *min = sum; 
      if ((sum > 0) && (sum > *max)) *max = sum; 
    }
  *min = sqrt(*min);
  *max = sqrt(*max);
}


void niceFFTnumber(int *n) {
  *n = (int) NiceFFTNumber( (unsigned long) *n);
}
bool ok_n(int n, int *f, int nf) // taken from fourier.c of R
{
  int i;
  for (i = 0; i < nf; i++)
    while(n % f[i] == 0) if ((n /= f[i]) == 1) return TRUE;
  return n == 1;
}
int nextn(int n, int *f, int nf) // taken from fourier.c of R
{
    while(!ok_n(n, f, nf)) { n++; }
  return n;
}

bool HOMEMADE_NICEFFT=false;
unsigned long NiceFFTNumber(unsigned long n) {
#define F_NUMBERS 3
  int f[F_NUMBERS]={2,3,5}; 
  unsigned long i,ii,j,jj,l,ll,min, m=1;
  if (HOMEMADE_NICEFFT) {
    if (n<=1) return n;
    for (i=0; i<F_NUMBERS; i++) 
      while ( ((n % f[i])==0) && (n>10000)) { m*=f[i]; n/=f[i]; }
    if (n>10000) {
      while (n>10000) {m*=10; n/=10;}
      n++;
    }
    min = 10000000;
    for (i=0, ii=1; i<=14; i++, ii<<=1) { // 2^14>10.000
      if (ii>=n) { if (ii<min) min=ii; break; }
      for (j=0, jj=ii; j<=9; j++, jj*=3) {// 3^9>10.000
	if (jj>=n) { if (jj<min) min=jj; break; }
	
	//for (k=0, kk=jj; k<=6; k++, kk*=5) {// 5^6>10.000
	//if (kk>=n) { if (kk<min) min=kk; break; }
	//for (l=0, ll=kk; l<=5; l++, ll*=7) {// 7^5>10.000
	
	// instead of (if 7 is included)
	for (l=0, ll=jj; l<=6; l++, ll*=5) {// 5^5>10.000
	  	  
	  if (ll>=n) { 
	    if (ll<min) min=ll;
	    break;
	  }
	  //}
	}
      }
    }
    return m*min;
  } else { // not HOMEMADE_NICEFFT
#define F_NUMBERS 3
    int f[F_NUMBERS]={2,3,5}; 
    return nextn(n, f, F_NUMBERS);
  }
}




