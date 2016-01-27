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
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "Stitcher.h"

#include <vigra/stdimage.hxx>

namespace HuginBase {
namespace Nona {

/** The main stitching function.
 *  This function delegates all the work to the other functions
 *
 *  Due to the compile memory requirements of the instantiated templates
 *  ( > 1GB, for all pixel types), the instatiations are divided into 4 separate
 *  cpp files
 */
void stitchPanorama(const PanoramaData & pano,
                        const PanoramaOptions & opt,
                        AppBase::ProgressDisplay* progress,
                        const std::string & basename,
                        const UIntSet & usedImgs,
                        const AdvancedOptions& advOptions)
{
    DEBUG_ASSERT(pano.getNrOfImages() > 0);

    // probe the first image to determine a suitable image type for stitching
    unsigned int imgNr = *(usedImgs.begin());
    std::string fname =  pano.getImage(imgNr).getFilename().c_str();
    DEBUG_DEBUG("Probing image: " << fname);
    vigra::ImageImportInfo info(fname.c_str());
    std::string pixelType = info.getPixelType();
    int bands = info.numBands();
    int extraBands = info.numExtraBands();

    // check if all other relevant images have the same type
    for (UIntSet::const_iterator it = usedImgs.begin()++; it != usedImgs.end(); ++it) {
        vigra::ImageImportInfo info2(pano.getImage(*it).getFilename().c_str());
        if ( pixelType != info2.getPixelType() ) {
            UTILS_THROW(std::runtime_error, "image " <<
                    pano.getImage(*it).getFilename() << " uses " <<
                    info2.getPixelType() << " valued pixel, while " <<
                    pano.getImage(0).getFilename() << " uses: " << pixelType);
            return;
        }

        if (info2.numBands() - info2.numExtraBands() != bands - extraBands) {
            UTILS_THROW(std::runtime_error, "image " <<
                    pano.getImage(*it).getFilename() << " has " <<
                    info2.numBands() << " channels, while " <<
                    pano.getImage(0).getFilename() << " uses: " << bands);
            return;
        }
    }
//    DEBUG_DEBUG("Output pixel type: " << pixelType);
    PanoramaOptions opts = opt;
    if (opts.outputMode == PanoramaOptions::OUTPUT_HDR) {
        if (opts.outputPixelType.size() == 0) {
            opts.outputPixelType = "FLOAT";
        }
    } else {
        // get the emor parameters.
        opts.outputEMoRParams = pano.getSrcImage(0).getEMoRParams();
        if (opts.outputPixelType.size() == 0) {
            opts.outputPixelType = pixelType;
        } else {
            // if output format is specified, use output format as stitching format
            // TODO: this will fail when going down in precision: UINT16 -> UINT8
            pixelType = opts.outputPixelType;
        }
    }

#if 1
    if (opts.outputMode == PanoramaOptions::OUTPUT_HDR) {
        if (bands == 1 || (bands == 2 && extraBands == 1)) {
            stitchPanoIntern<vigra::FImage, vigra::BImage>(pano, opts, progress, basename, usedImgs, advOptions);
        } else if (bands == 3 || (bands == 4 && extraBands == 1)) {
            stitchPanoIntern<vigra::FRGBImage, vigra::BImage>(pano, opts, progress, basename, usedImgs, advOptions);
        } else {
            DEBUG_ERROR("unsupported depth, only images with 1 and 3 channel images are supported");
            throw std::runtime_error("unsupported depth, only images with 1 and 3 channel images are supported");
        }
    } else {
        // stitch the pano with a suitable image type
        if (bands == 1 || (bands == 2 && extraBands == 1)) {
            if (pixelType ==  "UINT8"||
                pixelType == "INT16" ||
                pixelType == "UINT16" )
            {
                stitchPanoGray_8_16(pano, opts, progress, basename, usedImgs, pixelType.c_str(), advOptions);
            } else {
                stitchPanoGray_32_float(pano, opts, progress, basename, usedImgs, pixelType.c_str(), advOptions);
            }
        } else if (bands == 3 || (bands == 4 && extraBands == 1)) {
            if (pixelType == "UINT8" ||
                pixelType == "INT16" ||
                pixelType == "UINT16" )
            {
                stitchPanoRGB_8_16(pano, opts, progress, basename, usedImgs, pixelType.c_str(), advOptions);
            } else {
                stitchPanoRGB_32_float(pano, opts, progress, basename, usedImgs, pixelType.c_str(), advOptions);
            }
        }
    }
#else
    // always stitch with float images.
    if (bands == 1 || (bands == 2 && extraBands == 1)) {
        stitchPanoIntern<FImage,BImage>(pano, opts, progress, basename, usedImgs);
    } else if (bands == 3 || (bands == 4 && extraBands == 1)) {
        stitchPanoIntern<FRGBImage,BImage>(pano, opts, progress, basename, usedImgs);
    } else {
        DEBUG_ERROR("unsupported depth, only images with 1 and 3 channel images are supported");
        throw std::runtime_error("unsupported depth, only images with 1 and 3 channel images are supported");
        return;
    }
#endif
}


} // namespace
} // namespace

