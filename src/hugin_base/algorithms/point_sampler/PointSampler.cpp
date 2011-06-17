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
#include <algorithms/nona/FitPanorama.h>
#include <algorithms/nona/CenterHorizontally.h>

namespace HuginBase
{

bool PointSampler::runAlgorithm()
{
    // is this correct? how much progress requierd?
    AppBase::ProgressReporter* progRep = 
        AppBase::ProgressReporterAdaptor::newProgressReporter(getProgressDisplay(), 2.0); 
    
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
    // center panorama and do fit to get minimum of black/unused space around panorama
    CenterHorizontally(pano).run();
    PanoramaOptions opts = pano.getOptions();
    CalculateFitPanorama fitPano(pano);
    fitPano.run();
    opts.setHFOV(fitPano.getResultHorizontalFOV());
    opts.setHeight(roundi(fitPano.getResultHeight()));
    pano.setOptions(opts);
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
    
    // scale point coordinates to fit into original panorama.
    for (size_t i=0; i < points.size(); i++) {
        double scaleF1 = origsize[points[i].imgNr1].x / (float) images[points[i].imgNr1]->size().x;
        double scaleF2 = origsize[points[i].imgNr2].x / (float) images[points[i].imgNr2]->size().x;
        points[i].p1.x = points[i].p1.x * scaleF1;
        points[i].p1.y = points[i].p1.y * scaleF1;
        points[i].p2.x = points[i].p2.x * scaleF2;
        points[i].p2.y = points[i].p2.y * scaleF2;
    }

    for (size_t i=0; i < images.size(); i++) {
        delete images[i];
        delete lapImgs[i];
    }
    
    delete &pano; // deleting the NewCopy
}



/// for compatibility deprecated
void PointSampler::extractPoints(PanoramaData& pano, std::vector<vigra::FRGBImage*> images, int nPoints,
                                 bool randomPoints, AppBase::ProgressReporter& progress,
                                 std::vector<vigra_ext::PointPairRGB>& points)
{
    PointSampler* sampler = (randomPoints)? (PointSampler*) new RandomPointSampler(pano, NULL, images, nPoints)
                                          : (PointSampler*) new AllPointSampler(pano, NULL, images, nPoints);
    
    sampler->sampleAndExtractPoints(progress);
    points = sampler->getResultPoints();
    
    delete sampler;
}


}; // namespace

