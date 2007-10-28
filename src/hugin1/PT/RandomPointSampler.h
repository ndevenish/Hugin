// -*- c-basic-offset: 4 -*-
/** @file RandomPointSampler.h
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

#ifndef _Hgn1_RANDOM_POINT_SAMPLER_H
#define _Hgn1_RANDOM_POINT_SAMPLER_H

#include <algorithms/point_sampler/PointSampler.h>

#include <vigra/functorexpression.hxx>
#include <vigra_ext/impexalpha.hxx>
#include "PT/Panorama.h"

namespace PT
{
    
    
    inline void extractPoints(Panorama pano, std::vector<vigra::FRGBImage*> images, int nPoints,
                              bool randomPoints, AppBase::ProgressReporter & progress,
                              std::vector<vigra_ext::PointPairRGB> & points  )
    {
        HuginBase::PointSampler::extractPoints(pano, images, nPoints, randomPoints, progress, points);
    }
    
    
    template<class ImageType>
        std::vector<ImageType *> loadImagesPyr(std::vector<std::string> files, int pyrLevel, int verbose=0)
    {
            typedef typename ImageType::value_type PixelType;
            std::vector<ImageType *> srcImgs;
            for (size_t i=0; i < files.size(); i++) {
                ImageType * tImg = new ImageType();
                ImageType * tImg2 = new ImageType();
                vigra::ImageImportInfo info(files[i].c_str());
                tImg->resize(info.size());
                if (verbose)
                    std::cout << "loading: " << files[i] << std::endl;
                
                if (info.numExtraBands() == 1) {
                    // dummy mask
                    vigra::BImage mask(info.size());
                    vigra::importImageAlpha(info, vigra::destImage(*tImg), vigra::destImage(mask));
                } else {
                    vigra::importImage(info, vigra::destImage(*tImg));
                }
                float div = 1;
                if (strcmp(info.getPixelType(), "UINT8") == 0) {
                    div = 255;
                } else if (strcmp(info.getPixelType(), "UINT16") == 0) {
                    div = (1<<16)-1;
                }
                
                if (pyrLevel) {
                    ImageType * swap;
                    // create downscaled image
                    if (verbose > 0) {
                        std::cout << "downscaling: ";
                    }
                    for (int l=pyrLevel; l > 0; l--) {
                        if (verbose > 0) {
                            std::cout << tImg->size().x << "x" << tImg->size().y << "  " << std::flush;
                        }
                        vigra_ext::reduceToNextLevel(*tImg, *tImg2);
                        swap = tImg;
                        tImg = tImg2;
                        tImg2 = swap;
                    }
                    if (verbose > 0)
                        std::cout << std::endl;
                }
                if (div > 1) {
                    div = 1/div;
                    transformImage(vigra::srcImageRange(*tImg), vigra::destImage(*tImg),
                                   vigra::functor::Arg1()*vigra::functor::Param(div));
                }
                srcImgs.push_back(tImg);
                delete tImg2;
            }
            return srcImgs;
    }
    
    
    // needs 2.0 progress steps
    inline void loadImgsAndExtractPoints(Panorama pano, int nPoints, int pyrLevel, bool randomPoints, AppBase::ProgressReporter& progress, std::vector<vigra_ext::PointPairRGB> & points  )
    {
        // extract file names
        std::vector<std::string> files;
        for (size_t i=0; i < pano.getNrOfImages(); i++)
            files.push_back(pano.getImage(i).getFilename());
        
        std::vector<vigra::FRGBImage*> images;
        
        // try to load the images.
        images = loadImagesPyr<vigra::FRGBImage>(files, pyrLevel, 1);
        
        HuginBase::PointSampler::extractPoints(pano, images, nPoints, randomPoints, progress, points);
    }
};

#endif
