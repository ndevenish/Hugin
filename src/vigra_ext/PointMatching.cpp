// -*- c-basic-offset: 4 -*-

/** @file PointMatching.cpp
 *
 *  @brief implementation of PointMatching Class
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


#include "vigra_ext/PointMatching.h"

#include "common/utils.h"
#include "common/stl_utils.h"

using namespace vigra_ext;

/** run sift feature detector on a bunch of images */
void vigra_ext::extractSIFT(const std::vector<std::string> & imgfiles,
                            double scale,
                            SIFTFeatureTable & ftable,
                            utils::MultiProgressDisplay & pdisp,
                            const std::string & keypointsExe
                            )
{
    ftable.clear();
    ftable.resize(imgfiles.size());
    for(unsigned int i=0; i < imgfiles.size() ; i++) {

        vigra::BImage img;

//        pdisp.progressMessage("SIFT feature detection",
//                              100 * i/imgfiles.size());
        std::string filename = imgfiles[i];
        DEBUG_DEBUG("loading image " << filename);
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
            vigra::BRGBImage timg(info.width(), info.height());
            vigra::importImage(info, destImage(timg));
            vigra::copyImage(timg.upperLeft(),
                             timg.lowerRight(),
                             vigra::RGBToGrayAccessor<vigra::RGBValue<unsigned char> >(),
                             img.upperLeft(),
                             vigra::BImage::Accessor());
        }

        if (scale != 1.0) {
            // resize images
            int width  = (int) round(img.width() * scale);
            int height = (int) round(img.height() * scale);
            vigra::BImage t(width,height);
            vigra::resizeImageLinearInterpolation(vigra::srcImageRange(img),
                                                  vigra::destImageRange(t));
            img = t;
        }
        DEBUG_DEBUG("starting sift detector");
        // detect SIFT features
        vigra_ext::loweDetectSIFT(srcImageRange(img), ftable[i], keypointsExe);
        DEBUG_DEBUG( ftable[i].size() << " features found");
    }
}


/** run sift feature detector on a bunch of images */
void vigra_ext::extractSIFT2(const std::vector<std::string> & imgfiles,
                             double scale1,
                             SIFTFeatureTable & ftable1,
                             double scale2,
                             SIFTFeatureTable & ftable2,
                             utils::MultiProgressDisplay & pdisp,
                             const std::string & keypointsExe
                             )
{
    ftable1.clear();
    ftable1.resize(imgfiles.size());
    ftable2.clear();
    ftable2.resize(imgfiles.size());
    for(unsigned int i=0; i < imgfiles.size() ; i++) {

//        pdisp.progressMessage("SIFT feature detection",
//                              100 * i/imgfiles.size());
        vigra::BImage img;

        std::string filename = imgfiles[i];
        DEBUG_DEBUG("loading image " << filename);
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
            vigra::BRGBImage timg(info.width(), info.height());
            vigra::importImage(info, destImage(timg));
            vigra::copyImage(timg.upperLeft(),
                             timg.lowerRight(),
                             vigra::RGBToGrayAccessor<vigra::RGBValue<unsigned char> >(),
                             img.upperLeft(),
                             vigra::BImage::Accessor());
        }

        {
            // resize image
            int width  = (int) round(img.width() * scale1);
            int height = (int) round(img.height() * scale1);
            vigra::BImage t(width,height);
            vigra::resizeImageLinearInterpolation(vigra::srcImageRange(img),
                                                  vigra::destImageRange(t));
            DEBUG_DEBUG("starting sift detector, img size (" << width << "x" << height << ")");
            // detect SIFT features
            vigra_ext::loweDetectSIFT(srcImageRange(t), ftable1[i], keypointsExe);
            DEBUG_DEBUG("finished: " << ftable1[i].size() << " features found");
        }
        {
            // resize image
            int width  = (int) round(img.width() * scale2);
            int height = (int) round(img.height() * scale2);
            vigra::BImage t(width,height);
            vigra::resizeImageLinearInterpolation(vigra::srcImageRange(img),
                                                  vigra::destImageRange(t));
            DEBUG_DEBUG("starting sift detector, img size(" << width << "x" << height << ")");
            // detect SIFT features
            vigra_ext::loweDetectSIFT(srcImageRange(t), ftable2[i], keypointsExe);
            DEBUG_DEBUG(width << "x" << height << " size matching: " << ftable2[i].size() << " features found");
        }
    }
}




