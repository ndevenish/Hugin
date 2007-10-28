// -*- c-basic-offset: 4 -*-
/** @file Stitcher.cpp
 *
 *  Contains various routines used for stitching panoramas.
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
 *
 *  This is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "Stitcher.h"
// somehow these are still set after panorama.h has been included
#undef DIFFERENCE
#undef min
#undef max
#undef MIN
#undef MAX


using namespace std;
using namespace vigra;

void HuginBase::Nona::stitchPanoRGB_32_float(const PanoramaData & pano,
                                  const PanoramaOptions & opts,
                                  AppBase::MultiProgressDisplay & progress,
                                  const std::string & basename,
                                  const UIntSet & usedImgs,
                                  const char * pixelType)
{
    if (strcmp(pixelType, "INT32") == 0 ) {
        stitchPanoIntern<IRGBImage,BImage>(pano, opts, progress, basename, usedImgs);
    } else if (strcmp(pixelType, "UINT32") == 0 ) {
        stitchPanoIntern<UInt32RGBImage,BImage>(pano, opts, progress, basename, usedImgs);
    } else if (strcmp(pixelType, "FLOAT") == 0 ) {
        stitchPanoIntern<FRGBImage,BImage>(pano, opts, progress, basename, usedImgs);
    } else if (strcmp(pixelType, "DOUBLE") == 0 ) {
        stitchPanoIntern<DRGBImage,BImage>(pano, opts, progress, basename, usedImgs);
    } else {
        UTILS_THROW(std::runtime_error, "Unsupported pixel type: " << pixelType );
        return;
    }
}


