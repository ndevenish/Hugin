// -*- c-basic-offset: 4 -*-

/** @file ExtractPoints.h
 *
 *  @brief helper procedure for photometric optimizer on command line
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
 *
 */

/**
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

#include <hugin_basic.h>

#include <vigra/error.hxx>
#include <vigra/impex.hxx>

#include <vigra_ext/ransac.h>
#include <vigra_ext/VigQuotientEstimator.h>
#include <vigra_ext/Pyramid.h>
#include <vigra_ext/Interpolators.h>
#include <vigra_ext/impexalpha.hxx>
#include <algorithms/point_sampler/PointSampler.h>

using namespace std;
using namespace vigra;
using namespace vigra_ext;
using namespace AppBase;
using namespace HuginBase;

template<class ImageType>
std::vector<ImageType *> loadImagesPyr(std::vector<std::string> files, int pyrLevel, int verbose)
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
                reduceToNextLevel(*tImg, *tImg2);
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
void loadImgsAndExtractPoints(Panorama pano, int nPoints, int pyrLevel, bool randomPoints, AppBase::ProgressDisplay& progress, std::vector<vigra_ext::PointPairRGB> & points, int verbose)
{
    // extract file names
    std::vector<std::string> files;
    for (size_t i=0; i < pano.getNrOfImages(); i++)
        files.push_back(pano.getImage(i).getFilename());
    
    std::vector<vigra::FRGBImage*> images;
    
    // try to load the images.
    images = loadImagesPyr<vigra::FRGBImage>(files, pyrLevel, verbose);
    
    progress.startSubtask("Sampling points",0.0);
    if(randomPoints)
        points = RandomPointSampler(pano, &progress, images, nPoints).execute().getResultPoints();
    else
        points = AllPointSampler(pano, &progress, images, nPoints).execute().getResultPoints();
    progress.finishSubtask();
}
