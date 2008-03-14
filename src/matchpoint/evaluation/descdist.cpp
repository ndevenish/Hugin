#include <mex.h>
#include <math.h>

#include <stdio.h>

// This mex function implements the equivalent of:
//
//   function y = example_mat(x)
//   y = 2 * x + 1;
//   return

// [y, z] = foo(a, b, c)
//
//  nrhs = 3
//  a : prhs[0]
//  b : prhs[1]
//  c : prhs[2]
//
//  nlhs = 2
//  y : plhs[0]
//  z : plhs[1]

void mexFunction(int nlhs,       mxArray *plhs[],
                 int nrhs, const mxArray *prhs[])
{
  // must have single double array input.
  if (nrhs != 2) {  printf("in nrhs != 2"); return; }
  if (mxGetClassID(prhs[0]) != mxDOUBLE_CLASS) { printf("input must be a double array\n"); return; }
  if (mxGetClassID(prhs[1]) != mxDOUBLE_CLASS) { printf("input must be a double array\n"); return; }
  
  // must have single output.
  if (nlhs != 3) { printf("out nlhs != 3\n"); return; }
  
  // get size of x.
  int const num_dims1 = mxGetNumberOfDimensions(prhs[0]);
  int const *dims1 = mxGetDimensions(prhs[0]);
  int const num_dims2 = mxGetNumberOfDimensions(prhs[1]);
  int const *dims2 = mxGetDimensions(prhs[1]);
  if(dims1[0]<9 || dims2[0]<9 || dims1[0]!=dims2[0] ){ printf("dims != 9\n"); return; }
  
  // create new array of the same size as x.

  int *odim = new int[2];
  odim[0]=dims2[1];
  odim[1]=dims1[1];
  plhs[0] = mxCreateNumericArray(2, odim, mxDOUBLE_CLASS, mxREAL);
  plhs[1] = mxCreateNumericArray(2, odim, mxDOUBLE_CLASS, mxREAL);
  plhs[2] = mxCreateNumericArray(2, odim, mxDOUBLE_CLASS, mxREAL);
  
  // get pointers to beginning of x and y.
  double const *feat1 = (double *)mxGetData(prhs[0]);
  double const *feat2 = (double *)mxGetData(prhs[1]);
  double       *desc_out = (double *)mxGetData(plhs[0]);
  double       *mdesc_out = (double *)mxGetData(plhs[1]);
  double       *sdesc_out = (double *)mxGetData(plhs[2]);

  float *feat1a = new float[9];
  float *feat2a = new float[9];
  float *tdesc_out = new float[dims2[1]*dims1[1]];

   for(int j=0;j<dims1[1];j++){    
    for (int i=0; i<dims2[1]; i++){
      desc_out[j*dims2[1]+i]=1000000.0;
      sdesc_out[j*dims2[1]+i]=0;
    }
   } 


  float max_dist,fac,dist,dx,dy,bna,bua,descd,ov;
  for(int j=0,f1=0;j<dims1[1];j++,f1+=dims1[0]){    
    for (int i=0,f2=0; i<dims2[1]; i++,f2+=dims1[0]){
      descd=0;
      for(int v=9;v<dims1[0];v++){
	descd+=((feat1[f1+v]-feat2[f2+v])*(feat1[f1+v]-feat2[f2+v]));
      }
      descd=sqrt(descd);
      tdesc_out[j*dims2[1]+i]=descd;  
      mdesc_out[j*dims2[1]+i]=descd;        
    }
  }
  
  float minr=100,ratio,minratio;
  int mini=0;
  int minj=0;
  
  int dnbr=0;
  
  do{
    minr=1000000;
    for(int j=0;j<dims1[1];j++){    
      for (int i=0; i<dims2[1]; i++){
	if(minr>tdesc_out[j*dims2[1]+i]){
	  minr=tdesc_out[j*dims2[1]+i];
	  mini=i;
	  minj=j;
	}
      }
    }
    if(minr<1000000){
      for(int j=0;j<dims1[1];j++){
	tdesc_out[j*dims2[1]+mini]=1000000;
	}   
      for (int i=0; i<dims2[1]; i++){
	tdesc_out[minj*dims2[1]+i]=1000000;
      }
      desc_out[minj*dims2[1]+mini]=dnbr++;//minr
      minratio=1000000.0;
      for (int i=0; i<dims2[1]; i++){
	ratio=mdesc_out[minj*dims2[1]+i]/mdesc_out[minj*dims2[1]+mini];
	if(ratio<minratio && i!=mini && ratio>1)
	  minratio=ratio;	
      }
      sdesc_out[minj*dims2[1]+mini]=minratio;
    }
  }while(minr<1000000);
  //printf("\nstart rn\n");


  /*
  for(int j=0;j<dims1[1];j++){    
    minratio=1000000.0;
    minr=1000000;
    for (int i=0; i<dims2[1]; i++){
      tdesc_out[j*dims2[1]+i]=0;
      if(minr>mdesc_out[j*dims2[1]+i]){
	minr=mdesc_out[j*dims2[1]+i];
	mini=i;
      }
    }
    minratio=1000000.0;
    for (int i=0; i<dims2[1]; i++){
      ratio=mdesc_out[j*dims2[1]+i]/mdesc_out[j*dims2[1]+mini];
      if(ratio<minratio && i!=mini )
	minratio=ratio;	
    }
    tdesc_out[j*dims2[1]+mini]=minratio;
  }

  //  printf("start rn 1-to-1\n");

  do{
    minr=0;
    for(int j=0;j<dims1[1];j++){    
      for (int i=0; i<dims2[1]; i++){
	if(minr<tdesc_out[j*dims2[1]+i]){
	  minr=tdesc_out[j*dims2[1]+i];
	  mini=i;
	  minj=j;
	}
      }
    }
    if(minr>1){
      for(int j=0;j<dims1[1];j++){
	  tdesc_out[j*dims2[1]+mini]=0;
      }   
      for (int i=0; i<dims2[1]; i++){
	  tdesc_out[minj*dims2[1]+i]=0;
      }
      sdesc_out[minj*dims2[1]+mini]=minr;
    }
    
  }while(minr>1);
  

  */


  delete []odim;
  delete []tdesc_out;
  delete []feat1a;
  delete []feat2a;
}
