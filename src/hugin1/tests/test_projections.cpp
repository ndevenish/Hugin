/** @file test_projections.cpp
 *
 *  @brief Simple test for the forward and backward translation
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
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

#include "PT/PanoToolsInterface.h"

using namespace PT;
using namespace PTools;

int main(int argc, char ** argv)
{
    SrcPanoImage src;
    src.setHFOV(100);
    src.setSize(vigra::Size2D(101,101));

    PanoramaOptions opts;

    bool failed = false;

    double eps = 1e-8;

    for (int srcp=0; srcp < (int) SrcPanoImage::EQUIRECTANGULAR; srcp++) {
        src.setProjection( (SrcPanoImage::Projection) srcp);
        for (int destp=0; destp < (int) PanoramaOptions::EQUI_PANINI; destp++) 
        {
            opts.setProjection((PanoramaOptions::ProjectionFormat) destp);
            opts.setHFOV(100, false);
            opts.setWidth(101,false);
            opts.setHeight(101);

            PTools::Transform ptTrans;
            ptTrans.createTransform(src, opts);
            PTools::Transform ptInvTrans;
            ptInvTrans.createInvTransform(src, opts);

            for (int x=0; x < 101; x+=25) {
                for (int y=0; y < 101; y+=25) {
                    FDiff2D srcC(x,y);
                    FDiff2D destC;
                    ptTrans.transformImgCoord(destC, srcC);
                    FDiff2D srcC2;
                    ptInvTrans.transformImgCoord(srcC2, destC);
                    double errx = srcC.x - srcC2.x;
                    double erry = srcC.y - srcC2.y;
                    if (fabs(errx) > eps || fabs(erry) > eps) {
                        failed=true;
                        fprintf(stderr, "%d -> %d, failed: %f,%f (%f, %f) != %f,%f, error: %f,%f\n", srcp, destp, srcC.x, srcC.y, destC.x, destC.y, srcC2.x, srcC2.y, errx, erry);
                    }
                }
            }
        }
    }
    if (failed) {
        return 1;
    }
    return 0;
}

