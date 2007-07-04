// -*- c-basic-offset: 4 -*-
/** @file RandomPointSampler.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: RandomPointSampler.h 1998 2007-05-10 06:26:46Z dangelo $
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

#include "PointSampler.h"

#include <algorithms/basic/CalculateOptimalScale.h>
//#include <fstream>
//#include <algorithm>
//#include <ctime>
//
//#include <boost/random.hpp>
//
//#include <vigra/convolution.hxx>
//#include <vigra/impex.hxx>
//#include <vigra_ext/impexalpha.hxx>
////#include <common/math.h>
//
//#include <vigra_ext/VignettingCorrection.h>
//#include <vigra_ext/Pyramid.h>
//#include <vigra_ext/impexalpha.hxx>
//
//#include <panodata/PanoImage.h>
////#include <panotools/PanoToolsInterface.h>


namespace HuginBase
{


bool PointSampler::runAlgorithm()
{
    // is this correct? how much progress requierd?
    AppBase::ProgressReporter* progRep = 
        AppBase::ProgressReporterAdaptor::newProgressReporter(getProgressDisplay(), 1.0); 
    
    sampleAndExtractPoints(*progRep);
    
    delete progRep;

    if(hasProgressDisplay())
    {
        if(getProgressDisplay()->wasCancelled())
            cancelAlgorithm();
    }

    return wasCancelled();
}



void PointSampler::sampleAndExtractPoints(AppBase::ProgressReporter & progress)
{
    PanoramaData& pano = *(o_panorama.getNewCopy()); // don't forget to delete!
    std::vector<vigra::FRGBImage*>& images = o_images;
    int& nPoints = o_numPoints;
    std::vector<vigra_ext::PointPairRGB>& points = o_resultPoints;
    
    
    
    std::vector<vigra::FImage *> lapImgs;
    std::vector<vigra::Size2D> origsize;
    std::vector<SrcPanoImage> srcDescr;
    
    // convert to interpolating images
    typedef vigra_ext::ImageInterpolator<vigra::FRGBImage::const_traverser, vigra::FRGBImage::ConstAccessor, vigra_ext::interp_cubic> InterpolImg;
    std::vector<InterpolImg> interpolImages;
    
    vigra_ext::interp_cubic interp;
    // adjust sizes in panorama
    for (unsigned i=0; i < pano.getNrOfImages(); i++)
    {
        SrcPanoImage simg = pano.getSrcImage(i);
        origsize.push_back(simg.getSize());
        simg.resize(images[i]->size());
        srcDescr.push_back(simg);
        pano.setSrcImage(i, simg);
        interpolImages.push_back(InterpolImg(srcImageRange(*(images[i])), interp, false));
        
        vigra::FImage * lap = new vigra::FImage(images[i]->size());
        vigra::laplacianOfGaussian(srcImageRange(*(images[i]), vigra::GreenAccessor<vigra::RGBValue<float> >()), destImage(*lap), 1);
        lapImgs.push_back(lap);
    }
    
    
    // extract the overlapping points.
//    PanoramaOptions opts = pano.getOptions();
//    double scale = CalculateOptimalScale::calcOptimalPanoScale(pano.getSrcImage(0),opts);
//    opts.setWidth(utils::roundi(opts.getWidth()*scale), true);    
//    pano.setOptions(opts);
    SetWidthOptimal(pano).run();
    
    // if random points.
    // extract random points.
    // need to get the maximum gray value here..
    
    std::vector<std::multimap<double, vigra_ext::PointPairRGB> > radiusHist(10);
    
    unsigned nGoodPoints = 0;
    unsigned nBadPoints = 0;
    
    
    // call the samplePoints method of this class
    progress.setMessage("sampling points");
    samplePoints(interpolImages,
                 lapImgs,
                 srcDescr,
                 pano.getOptions(),
                 1/255.0,
                 250/255.0,
                 radiusHist,
                 nGoodPoints,
                 nBadPoints,
                 progress);
        
    // select points with low laplacian of gaussian values.
    progress.setMessage("extracting good points");
    sampleRadiusUniform(radiusHist, nPoints, points, progress);
    
    for (size_t i=0; i < images.size(); i++) {
        delete images[i];
        delete lapImgs[i];
    }
    // scale point coordinates to fit into original panorama.
    double scaleF = origsize[0].x / (float) images[0]->size().x;
    for (size_t i=0; i < points.size(); i++) {
        // scale coordiantes
        points[i].p1.x = points[i].p1.x * scaleF;
        points[i].p1.y = points[i].p1.y * scaleF;
        points[i].p2.x = points[i].p2.x * scaleF;
        points[i].p2.y = points[i].p2.y * scaleF;
    }
    
    delete &pano; // deleting the NewCopy
}



#if 0

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
void loadImgsAndExtractPoints(Panorama pano, int nPoints, int pyrLevel, bool randomPoints, AppBase::ProgressReporter & progress, std::vector<vigra_ext::PointPairRGB> & points  )
{
    // extract file names
    std::vector<std::string> files;
    for (size_t i=0; i < pano.getNrOfImages(); i++)
        files.push_back(pano.getImage(i).getFilename());

    std::vector<vigra::FRGBImage*> images;

    // try to load the images.
    images = loadImagesPyr<vigra::FRGBImage>(files, pyrLevel, 1);

    extractPoints(pano, images, nPoints, randomPoints, progress, points);
}

#endif //0


}; // namespace

