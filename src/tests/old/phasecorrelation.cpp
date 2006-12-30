// -*- c-basic-offset: 4 -*-

/** @file phasecorrelation.cpp
 *
 *  @brief test program to phase correlate two images
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <config.h>
#include "panoinc.h"
#include "PT/SimpleStitcher.h"
#include "PT/PhaseCorrelation.h"

using namespace vigra;
using namespace PT;
using namespace std;

static void usage(const char * name)
{
    cerr << name << " image1 image2 output" << endl
         << "   correlates image1 and image2, write output in matlab ascii format" << endl;
}

void loadBImage(vigra::BImage & img, const std::string & filename)
{
    // load image
    vigra::ImageImportInfo info(filename.c_str());
    // FIXME.. check for grayscale / color
    img.resize(info.width(), info.height());
    if(info.isGrayscale())
    {
        // import the image just read
        importImage(info, destImage(img));
    } else {
        // convert to greyscale
        BRGBImage timg(info.width(), info.height());
        importImage(info, destImage(timg));
        vigra::copyImage(timg.upperLeft(),
                         timg.lowerRight(),
                         RGBToGrayAccessor<RGBValue<unsigned char> >(),
                         img.upperLeft(),
                         BImage::Accessor());
    }
}

int main(int argc, char *argv[])
{
    if (argc !=4) {
        usage(argv[0]);
        return 1;
    }

    BImage first;
    loadBImage(first,argv[1]);
    BImage second;
    loadBImage(second, argv[2]);

    // create fftw plans, they are the same for all images
    fftwnd_plan fftw_p, fftw_pinv;
    fftw_p    = fftw2d_create_plan(first.width(), first.height(), FFTW_FORWARD, FFTW_ESTIMATE);
    fftw_pinv = fftw2d_create_plan(first.width(), first.height(), FFTW_BACKWARD, FFTW_ESTIMATE);


    FDiff2D transl = PT::phaseCorrelation(srcImageRange(first),
                                          srcImageRange(second),
                                          fftw_p, fftw_pinv,
                                          argv[3]);

    fftwnd_destroy_plan(fftw_p);
    fftwnd_destroy_plan(fftw_pinv);
}


