// -*- c-basic-offset: 4 -*-
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

#ifndef _PHASECORRELATION_H
#define _PHASECORRELATION_H

#include <stdlib.h>
#include <fftw.h>

#include "common/utils.h"
#include "common/math.h"

namespace PT {

// Calculate Phase Correlation
FDiff2D phase(fftw_complex *F1, fftw_complex *F2, int nx, int ny, fftwnd_plan pinv,
              std::string filename = "");

// Main Program
template <class SrcImageIterator, class SrcAccessor>
FDiff2D phaseCorrelation(vigra::triple<SrcImageIterator, SrcImageIterator, SrcAccessor> current,
                         vigra::triple<SrcImageIterator, SrcImageIterator, SrcAccessor> previous,
                         fftwnd_plan fftw_p, fftwnd_plan fftw_pinv, std::string filename="")
{

  /* Initialisation *****************/

  // Simple variables, counters, etc.

  int a, b;
//  int c, d, e, f, g, h;
  int N, dpos;

  int wide, high;

  float pi = 3.141592654;
  float X;

  float *x; x = new float [3];

  wide = current.second.x - current.first.x;
  high = current.second.y - current.first.y;
  DEBUG_DEBUG("using images with size: " << wide << "," << high);
  // Allocate memory for images
/* dangelo: we get vigra images...
  int **current;
  int **previous;
  current = new int * [wide];
  previous = new int * [wide];

  for(a = 0; a < wide; a++)
  {
    current[a] = new int [high];
    previous[a] = new int [high];
    if((!current[a]) || !(previous[a]))
    {
      std::cout << "Memory could not be allocated.\n";
      exit(-1);
    }
  }

  // Load images

  LoadPGM(argv[1], current, wide, high);
  LoadPGM(argv[2], previous, wide, high);

  std::cout << "Loaded images of dimensions " << wide << " x " << high << "\n";

*/
  /* Phase Correlation Preprocessing *****************/

/* dangelo: moved into copy loop below
  for(b = 0; b < high; b++)
  {
    for(a = 0; a < wide; a++)
    {
      X = cos(pi*(((float)(a)/(float)(wide)) - 0.5))*cos(pi*(((float)(b)/(float)(high)) - 0.5));
      current[a][b]  = (int)(X*current[a][b]);
      previous[a][b] = (int)(X*previous[a][b]);
    }
  }
*/

  // FFTW related code and variables

  N = wide*high;

  fftw_complex *inc, *fc;
//  fftw_complex *ffc;
  fftw_complex *inp, *fp;
//  fftw_complex *ffp;

  // allocate memory for complex data

  inc = new fftw_complex [N];
  inp = new fftw_complex [N];
  fc  = new fftw_complex [N];
  fp  = new fftw_complex [N];

  // Transfer images to FFTW input arrays
  // and
  // Input windowing for edges, raised cosine (=Hamming) window

  SrcImageIterator ycur(current.first);
  SrcImageIterator yprev(previous.first);
//  std::ofstream tf("band_img.txt");
  for(b = 0; b < high; b++, ++ycur.y, ++yprev.y)
  {
    SrcImageIterator xcur(ycur);
    SrcImageIterator xprev(yprev);
    for(a = 0; a < wide; a++, ++xcur.x, ++xprev.x)
    {

      X = cos(pi*(((float)(a)/(float)(wide)) - 0.5))*cos(pi*(((float)(b)/(float)(high)) - 0.5));
      dpos = b+a*high;
      inc[dpos].re = X * current.third(xcur);  inc[dpos].im = 0;
      inp[dpos].re = X * previous.third(xprev); inp[dpos].im = 0;
//      tf << inc[dpos].re << " ";
    }
//    tf << std::endl;
  }

  // Calculate FFT

  fftwnd_one(fftw_p, inc, fc);
  fftwnd_one(fftw_p, inp, fp);

  DEBUG_DEBUG("Calculated FFTs");

  /*
     You may wish to add some noise filtering etc here
                                                       */

  // Calculate Phase Correlation

  FDiff2D transl = phase(fc, fp, wide, high, fftw_pinv, filename);

  // Free memory and finish


  delete []inc;
  delete []inp;
  delete []fc;
  delete []fp;
  delete []x;

  return transl;
}

} // namespace

#endif // _PHASECORRELATION_H
