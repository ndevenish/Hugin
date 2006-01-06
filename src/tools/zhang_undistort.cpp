// -*- c-basic-offset: 4 -*-

/** @file zhang_undistort.cpp
 *
 *  @brief a simple program to undistort images using the zhang model,
 *         see CamChecker for a calibration program.
 *         http://www.loper.org/~matt/CamChecker/CamChecker_docs/html/index.html
 *
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
#include <fstream>
#include <sstream>

#include <vigra/error.hxx>
#include <vigra/impex.hxx>

#include <unistd.h>

#include "panoinc.h"
#include "vigra_ext/ImageTransforms.h"

#include <tiff.h>

using namespace vigra;
using namespace vigra_ext;
using namespace PT;
using namespace std;
using namespace utils;


static void usage(const char * name)
{
    cerr << name << ": undistort image, using zhang's algorithm" << std::endl
         << std::endl
         << "Usage: " << name  << " k0 k1 alpha beta u0 v0 input_image output_image" << std::endl
         << std::endl
         << "  The parameters can be found by using camchecker" << std::endl
         << "  http://loper.org/~matt/CamChecker/CamChecker_docs/html/index.html" << endl;
}

struct ZhangCalibData
{
    double alpha;
    double beta;
    double u0;
    double v0;
    double k0;
    double k1;
};

class UndistortZhang
{
public:
    UndistortZhang(ZhangCalibData & c_)
        : c(c_)
        { }



    void transformImgCoord(double & x_dest, double & y_dest,
                           double x_src, double y_src)
    {

        double xn, yn;
        xn = ( x_src - c.u0 ) / c.alpha;
        yn = ( y_src - c.v0 ) / c.beta;

        double d1 = xn*xn + yn*yn;
        double d2 = d1*d1;
        double s = c.k0*d1 + c.k1*d2;

        x_dest = x_src + (x_src - c.u0)* s;
        y_dest = y_src + (y_src - c.v0)* s;
#if 0
        printf("x:%f, y:%f   xn:%f, yn:%f, d1:%f, d2:%f, s:%f   x_s:%f, y_s:%f\n",
               x_src, y_src, xn, yn, d1, d2, s, x_dest, y_dest);

        char c;
        cin >> c;
#endif
    }

    ZhangCalibData c;
};


int main(int argc, char *argv[])
{
    if (argc != 9) {
        usage(argv[0]);
        return 1;
    }
    ZhangCalibData c;
    c.k0 = atof(argv[1]);
    c.k1 = atof(argv[2]);
    c.alpha = atof(argv[3]);
    c.beta = atof(argv[4]);
    c.u0 = atof(argv[5]);
    c.v0 = atof(argv[6]);

    char * input = argv[7];
    char * output = argv[8];

    // load input image
    vigra::ImageImportInfo info(input);

    vigra::BRGBImage distorted(info.width(), info.height());

    importImage(info, destImage(distorted));

    // create output image of appropriate size
    vigra::BRGBImage undistorted(distorted.size());
    vigra::BImage alpha(distorted.size());


    // functor to undistort an image
    UndistortZhang func(c);

    StreamMultiProgressDisplay disp(cout);

    
    
    // transform the image using cubic interpolation.
    transformImage(srcImageRange(distorted),
                   destImageRange(undistorted),
                   destImage(alpha),
                   Diff2D(1,1),
                   func,
                   false,
                   vigra_ext::INTERP_CUBIC,
                   disp);

    exportImage(srcImageRange(undistorted), vigra::ImageExportInfo(output));
    return 0;
}



