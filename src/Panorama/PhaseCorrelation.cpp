// Program  : basic.cc
// Purpose  : Basic Phase Correlation
// Author   : Lyndon Hill
// Last Edit: 13.08.2001 Adding comments
//            10.08.2001 Incept from phacor-fftw program

/*
   DISCLAIMER AND COPYRIGHT NOTICE

   This code is copyright Lyndon Hill, 10 August 2001. You are free to modify and copy it
   as you like but your work must cite my name properly. No commercial usage is allowed.

   The code is made available for research purposes only.
   USE AT YOUR OWN RISK. No warranty is implied.

   USAGE

   I use the following line to make this program:
   g++ -o basic basic.cc -Lfftw/lib -O2 -lfftw -lm

   You must have installed FFTW before use (http://www.fftw.org). The command line is

   basic image1 image0 width height

   All arguments are required. The images are expected to be in PGM format.

*/

#include <iostream>
#include <string>
using namespace std;

#include <stdlib.h>
//#include "fftw/include/fftw.h"

#include "PT/PhaseCorrelation.h"

// Access values stored in complex data type
static float wmag(fftw_complex *data, int nx, int ny, int px, int py);
static float rhei(fftw_complex *data, int nx, int ny, int px, int py);




// Function   : phase
// Purpose    : Calculate the phase correlation
// Author     : Lyndon Hill
// Last Change: 10.08.2001

FDiff2D PT::phase(fftw_complex *F1, fftw_complex *F2, int nx, int ny, fftwnd_plan pinv)
{
  int e, f;
  int dpos;
  int peakx = 0, peaky = 0;      // Position of the peak on the surface
//  float m1, m2;
  double rcps, icps, mag;

  float height = 0;
  float peakh = 0;           // Maximum height on PC surface

  float *b; b = new float [3];
  float *x; x = new float [3];

  FDiff2D result;

  fftw_complex *pcs;
  pcs = new fftw_complex[nx*ny];

  // Calculate Phase Correlation and put it into image 1

  for(e = 0; e < ny; e++)
  {
    for(f = 0; f < nx; f++)
    {
      dpos = e+f*ny;
      rcps = F1[dpos].re*F2[dpos].re + F1[dpos].im*F2[dpos].im;
      icps = F1[dpos].im*F2[dpos].re - F2[dpos].im*F1[dpos].re;
      mag = sqrt(rcps*rcps+icps*icps);

      if(mag == 0)
      {
        rcps = 0; icps = 0;   // Just in case
      }
      else
      {
        rcps /= mag;
        icps /= mag;
      }

      F1[dpos].re = rcps; F1[dpos].im = icps;
    }
  }

  // Inverse FFT

  fftwnd_one(pinv, F1, pcs);

  // FFT is still scaled by ny*nx

  for(e = 0; e < ny; e++)
  {
    for(f = 0; f < nx; f++)
    {
      dpos = e+f*ny;
      pcs[dpos].re = (pcs[dpos].re)/(ny*nx);
      pcs[dpos].im = (pcs[dpos].im/(ny*nx));
    }
  }

  // Peak searching routine

  for(e = 0; e < ny; e++)
  {
    for(f = 0; f < nx; f++)
    {
      dpos = e+f*ny;
      height = pcs[dpos].re;
      if(height > peakh)
      {
        peakx = f; peaky = e;
    peakh = height;
      }
    }
  }

  cout << "Integer peak at (" << peakx << "," << peaky << ") height " << peakh << "\n";

  /* Peak interpolation, using a simple quadratic function *****************/

  // Initialise some variables purely for interpolation

//  int i, j, k;
  float fpeakx, fpeaky;
  float topmag = 0.0;
  float above, below;
  float det;

  float *z;    z  = new float [3];
  float **w;   w  = new float * [3];
  float **Ct;  Ct = new float * [3];

  for(e = 0; e < 3; e++)
  {
    w[e]  = new float [3];
    Ct[e] = new float [3];
  }

  // Find quadratic on the x axis

  z[0] = wmag(pcs, nx, ny, peakx-1, peaky);
  z[1] = wmag(pcs, nx, ny, peakx,   peaky);
  z[2] = wmag(pcs, nx, ny, peakx+1, peaky);

  w[0][0] = (peakx-1)*(peakx-1); w[1][0] = peakx-1; w[2][0] = 1.0;
  w[0][1] = (peakx)*(peakx);     w[1][1] = peakx;   w[2][1] = 1.0;
  w[0][2] = (peakx+1)*(peakx+1); w[1][2] = peakx+1; w[2][2] = 1.0;

  // Invert w

  det = w[0][0]*(w[1][1]*w[2][2]-w[1][2]*w[2][1])
       -w[1][0]*(w[0][1]*w[2][2]-w[0][2]*w[2][1])
       +w[2][0]*(w[0][1]*w[1][2]-w[0][2]*w[1][1]);

  Ct[0][0] =   w[1][1]*w[2][2]-w[1][2]*w[2][1];
  Ct[0][1] = -(w[0][1]*w[2][2]-w[0][2]*w[2][1]);
  Ct[0][2] =   w[0][1]*w[1][2]-w[0][2]*w[1][1];
  Ct[1][0] = -(w[1][0]*w[2][2]-w[1][2]*w[2][0]);
  Ct[1][1] =   w[0][0]*w[2][2]-w[0][2]*w[2][0];
  Ct[1][2] = -(w[0][0]*w[1][2]-w[0][2]*w[1][0]);
  Ct[2][0] =   w[1][0]*w[2][1]-w[1][1]*w[2][0];
  Ct[2][1] = -(w[0][0]*w[2][1]-w[0][1]*w[2][0]);
  Ct[2][2] =   w[0][0]*w[1][1]-w[0][1]*w[1][0];

  w[0][0] = (Ct[0][0]/det);
  w[0][1] = (Ct[0][1]/det);
  w[0][2] = (Ct[0][2]/det);
  w[1][0] = (Ct[1][0]/det);
  w[1][1] = (Ct[1][1]/det);
  w[1][2] = (Ct[1][2]/det);
  w[2][0] = (Ct[2][0]/det);
  w[2][1] = (Ct[2][1]/det);
  w[2][2] = (Ct[2][2]/det);

  x[0] = w[0][0]*z[0]+w[1][0]*z[1]+w[2][0]*z[2];
  x[1] = w[0][1]*z[0]+w[1][1]*z[1]+w[2][1]*z[2];
  x[2] = w[0][2]*z[0]+w[1][2]*z[1]+w[2][2]*z[2];

  if(fabs(det) > 0)     // In case matrix w is uninvertible.
  {
    fpeakx = -x[1]/(2*x[0]);
    topmag = x[0]*(fpeakx*fpeakx)+x[1]*fpeakx+x[2];
  }
  else fpeakx = peakx;  // Revert to integer peak

  if(fpeakx < 0.0)
    fpeakx += nx;

  // Reform the quadratic for above and below (below first)

  if(fabs(det) > 0)
  {
    z[0] = wmag(pcs, nx, ny, peakx-1, peaky-1);
    z[1] = wmag(pcs, nx, ny, peakx, peaky-1);
    z[2] = wmag(pcs, nx, ny, peakx+1, peaky-1);

    x[0] = w[0][0]*z[0]+w[1][0]*z[1]+w[2][0]*z[2];
    x[1] = w[0][1]*z[0]+w[1][1]*z[1]+w[2][1]*z[2];
    x[2] = w[0][2]*z[0]+w[1][2]*z[1]+w[2][2]*z[2];

    below = (x[0]*fpeakx*fpeakx)+(x[1]*fpeakx)+x[2];

    z[0] = wmag(pcs, nx, ny, peakx-1, peaky+1);
    z[1] = wmag(pcs, nx, ny, peakx, peaky+1);
    z[2] = wmag(pcs, nx, ny, peakx+1, peaky+1);

    x[0] = w[0][0]*z[0]+w[1][0]*z[1]+w[2][0]*z[2];
    x[1] = w[0][1]*z[0]+w[1][1]*z[1]+w[2][1]*z[2];
    x[2] = w[0][2]*z[0]+w[1][2]*z[1]+w[2][2]*z[2];

    above = (x[0]*fpeakx*fpeakx)+(x[1]*fpeakx)+x[2];
  }
  else
  {
    below = wmag(pcs, nx, ny, peakx, peaky-1);
    above = wmag(pcs, nx, ny, peakx, peaky+1);
  }

  // Find quadratic on the y axis

  z[0] = below;
  z[1] = topmag;
  z[2] = above;

  w[0][0] = (peaky-1)*(peaky-1); w[1][0] = peaky-1; w[2][0] = 1.0;
  w[0][1] = (peaky)*(peaky);     w[1][1] = peaky;   w[2][1] = 1.0;
  w[0][2] = (peaky+1)*(peaky+1); w[1][2] = peaky+1; w[2][2] = 1.0;

  // Invert w

  det = w[0][0]*(w[1][1]*w[2][2]-w[1][2]*w[2][1])
       -w[1][0]*(w[0][1]*w[2][2]-w[0][2]*w[2][1])
       +w[2][0]*(w[0][1]*w[1][2]-w[0][2]*w[1][1]);


  Ct[0][0] =   w[1][1]*w[2][2]-w[1][2]*w[2][1];
  Ct[0][1] = -(w[0][1]*w[2][2]-w[0][2]*w[2][1]);
  Ct[0][2] =   w[0][1]*w[1][2]-w[0][2]*w[1][1];
  Ct[1][0] = -(w[1][0]*w[2][2]-w[1][2]*w[2][0]);
  Ct[1][1] =   w[0][0]*w[2][2]-w[0][2]*w[2][0];
  Ct[1][2] = -(w[0][0]*w[1][2]-w[0][2]*w[1][0]);
  Ct[2][0] =   w[1][0]*w[2][1]-w[1][1]*w[2][0];
  Ct[2][1] = -(w[0][0]*w[2][1]-w[0][1]*w[2][0]);
  Ct[2][2] =   w[0][0]*w[1][1]-w[0][1]*w[1][0];

  w[0][0] = (Ct[0][0]/det);
  w[0][1] = (Ct[0][1]/det);
  w[0][2] = (Ct[0][2]/det);
  w[1][0] = (Ct[1][0]/det);
  w[1][1] = (Ct[1][1]/det);
  w[1][2] = (Ct[1][2]/det);
  w[2][0] = (Ct[2][0]/det);
  w[2][1] = (Ct[2][1]/det);
  w[2][2] = (Ct[2][2]/det);

  x[0] = w[0][0]*z[0]+w[1][0]*z[1]+w[2][0]*z[2];
  x[1] = w[0][1]*z[0]+w[1][1]*z[1]+w[2][1]*z[2];
  x[2] = w[0][2]*z[0]+w[1][2]*z[1]+w[2][2]*z[2];

  if(fabs(det) > 0)     // In case matrix w is uninvertible.
  {
    fpeaky = -x[1]/(2*x[0]);
    topmag = x[0]*(fpeaky*fpeaky)+x[1]*fpeaky+x[2];
  }
  else fpeaky = peaky;  // Revert to integer position

  if(fpeaky < 0.0)
    fpeaky += ny;

  /*
     The peak position may appear very far from the origin because a motion
     of -1 will wrap to the other side. The height is also interpolated.
                                                                            */

  cout << "Interpolated peak at ";

  if(fpeakx < nx/2)  {
    cout << "(" << -fpeakx << ",";
    result.x = peakx;
  } else {
    cout << "(" << nx-fpeakx << " ";
    result.x = fpeakx - nx;
  }

  if(fpeaky < ny/2) {
    cout << -fpeaky;
    result.y = fpeaky;
  } else {
    cout << ny-fpeaky;
    result.y = fpeaky - ny;
  }
  cout << ") height " << topmag << "\n";

  // Free memory

  delete []pcs;
  delete []b;
  delete []z;

  for(e = 0; e < 3; e++)
  {
    delete [] w[e];
    delete [] Ct[e];
  }
  delete []w;
  delete []x;
  delete []Ct;
  return result;
}


// Function   : wmag
// Purpose    : Wrap around surface for PC
// Author     : Lyndon Hill
// Last Change: 14.4.99

float wmag(fftw_complex *data, int nx, int ny, int px, int py)
{
  if(px < 0)
    px = nx + px;
  if(px >= nx)
    px -= nx;

  if(py < 0)
    py = ny + py;
  if(py >= ny)
    py -= ny;

  float temp = rhei(data, nx, ny, px, py);
  return(temp);
}

// Function   : rhei
// Purpose    : Find the real height, simplifies access to complex data
// Author     : Lyndon Hill
// Last Change: 10.08.2001

float rhei(fftw_complex *data, int nx, int ny, int px, int py)
{
  int dpos = py + px*ny;
  float a = data[dpos].re;
  return(a);
}

