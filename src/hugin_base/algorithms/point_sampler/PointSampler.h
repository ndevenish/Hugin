// -*- c-basic-offset: 4 -*-
/** @file RandomPointSampler.h
*
*  @author Pablo d'Angelo <pablo.dangelo@web.de>
*
*  $Id: RandomPointSampler.h,v 1.10 2007/05/10 06:26:46 dangelo Exp $
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

#ifndef _POINTSAMPLER_H
#define _POINTSAMPLER_H

#include <ctime>

#include <hugin_shared.h>
#include <algorithms/PanoramaAlgorithm.h>

#include <boost/random.hpp>
#include <vigra/impex.hxx>
#include <vigra_ext/utils.h>
#include <vigra_ext/Pyramid.h>
#include <appbase/ProgressReporterOld.h>
#include <panodata/PanoramaData.h>

namespace HuginBase
{

    class IMPEX PointSampler : public TimeConsumingPanoramaAlgorithm
    {
        protected:
            ///
            PointSampler(PanoramaData& panorama, AppBase::ProgressDisplay* progressDisplay,
                         std::vector<vigra::FRGBImage*> images,
                         int nPoints)
                : TimeConsumingPanoramaAlgorithm(panorama, progressDisplay),
                  o_images(images), o_numPoints(nPoints)
            {};
        
        public:        
            ///
            virtual ~PointSampler() {};
        
        
        public:
            /// for compatibility deprecated
            static void extractPoints(PanoramaData& pano, std::vector<vigra::FRGBImage*> images, int nPoints,
                                      bool randomPoints, AppBase::ProgressReporter& progress,
                                      std::vector<vigra_ext::PointPairRGB>& points);
        
        protected:
            ///
            typedef vigra_ext::ImageInterpolator<vigra::FRGBImage::const_traverser,
                                                 vigra::FRGBImage::ConstAccessor,
                                                 vigra_ext::interp_cubic           > InterpolImg;
            
            ///
            void sampleAndExtractPoints(AppBase::ProgressReporter& progress);
            
            ///
            virtual void samplePoints(const std::vector<InterpolImg>& imgs,
                                      const std::vector<vigra::FImage*>& voteImgs,
                                      const std::vector<SrcPanoImage>& src,
                                      const PanoramaOptions& dest,
                                      float minI,
                                      float maxI,
                                      std::vector<std::multimap<double,vigra_ext::PointPairRGB> >& radiusHist,
                                      unsigned& nGoodPoints,
                                      unsigned& nBadPoints,
                                      AppBase::ProgressReporter& progress) =0;
            
            /** extract some random points out of the bins.
            *  This should ensure that the radius distribution is
            *  roughly uniform
            */
            template<class PointPairClass>
            static void sampleRadiusUniform(const std::vector<std::multimap<double,PointPairClass> >& radiusHist,
                                            unsigned nPoints,
                                            std::vector<PointPairClass>& selectedPoints,
                                            AppBase::ProgressReporter& progress);
            
        public:
            ///
            virtual bool modifiesPanoramaData() const
                { return false; }
            
            ///
            virtual bool runAlgorithm();

            PointSampler & execute()
            {
				run();
                return *this;
            }
            
        public:
            ///
            typedef std::vector<vigra_ext::PointPairRGB> PointPairs;
            
            ///
            PointPairs getResultPoints()
            {
                //[TODO] debug unsuccessful
                
                return o_resultPoints;
            }
            
            
        protected:
            std::vector<vigra::FRGBImage*> o_images;
            int o_numPoints;
            PointPairs o_resultPoints;
    };


    /**
     *
     */
    class AllPointSampler : public  PointSampler
    {
        public:
            ///
            AllPointSampler(PanoramaData& panorama, AppBase::ProgressDisplay* progressDisplay,
                               std::vector<vigra::FRGBImage*> images,
                               int nPoints)
             : PointSampler(panorama, progressDisplay, images, nPoints)
            {};

            ///
            virtual ~AllPointSampler() {};
            
            
        public:
            /** sample all points inside a panorama and check for create a
             *  bins of point pairs that include a specific radius.
             */
            template <class Img, class VoteImg, class PP>
            static void sampleAllPanoPoints(const std::vector<Img> &imgs,
                                     const std::vector<VoteImg *> &voteImgs,
                                     const std::vector<SrcPanoImage> & src,
                                     const PanoramaOptions & dest,
                                     int nPoints,
                                     float minI,
                                     float maxI,
                                     std::vector<std::multimap<double, PP > > & radiusHist,
                                     unsigned & nGoodPoints,
                                     unsigned & nBadPoints,
                                     AppBase::ProgressReporter & progress);
            
        protected:
            ///
            virtual void samplePoints(const std::vector<InterpolImg>& imgs,
                                      const std::vector<vigra::FImage*>& voteImgs,
                                      const std::vector<SrcPanoImage>& src,
                                      const PanoramaOptions& dest,
                                      float minI,
                                      float maxI,
                                      std::vector<std::multimap<double,vigra_ext::PointPairRGB> >& radiusHist,
                                      unsigned& nGoodPoints,
                                      unsigned& nBadPoints,
                                      AppBase::ProgressReporter& progress)
            {
                sampleAllPanoPoints(imgs,
                                    voteImgs,
                                    src,
                                    dest,
                                     o_numPoints,
                                    minI,
                                    maxI,
                                    radiusHist,
                                    nGoodPoints,
                                    nBadPoints,
                                    progress);
            }
            
    };


    /**
     *
     */
    class RandomPointSampler : public  PointSampler
    {
        public:
            ///
            RandomPointSampler(PanoramaData& panorama, AppBase::ProgressDisplay* progressDisplay,
                               std::vector<vigra::FRGBImage*> images,
                               int nPoints)
             : PointSampler(panorama, progressDisplay, images, nPoints)
            {};

            ///
            virtual ~RandomPointSampler() {};
            
            
        public:
            template <class Img, class VoteImg, class PP>
            static void sampleRandomPanoPoints(const std::vector<Img> imgs,
                                               const std::vector<VoteImg *> &voteImgs,
                                               const std::vector<SrcPanoImage> & src,
                                               const PanoramaOptions & dest,
                                               int nPoints,
                                               float minI,
                                               float maxI,
                                               std::vector<std::multimap<double, PP > > & radiusHist,
                                               unsigned & nBadPoints,
                                               AppBase::ProgressReporter & progress);
            
        protected:
            ///
            virtual void samplePoints(const std::vector<InterpolImg>& imgs,
                                      const std::vector<vigra::FImage*>& voteImgs,
                                      const std::vector<SrcPanoImage>& src,
                                      const PanoramaOptions& dest,
                                      float minI,
                                      float maxI,
                                      std::vector<std::multimap<double,vigra_ext::PointPairRGB> >& radiusHist,
                                      unsigned& nGoodPoints,
                                      unsigned& nBadPoints,
                                      AppBase::ProgressReporter& progress)
            {
                 sampleRandomPanoPoints(imgs,
                                        voteImgs,
                                        src,
                                        dest,
                                         5*o_numPoints,
                                        minI,
                                        maxI,
                                        radiusHist,
                                        nBadPoints,
                                        progress);
            }
            
    };

    
    
} // namespace
    
    


    
//==============================================================================
//  templated methods


#include <boost/random.hpp>
#include <panotools/PanoToolsInterface.h>


namespace HuginBase {


template<class PointPairClass>
void PointSampler::sampleRadiusUniform(const std::vector<std::multimap<double, PointPairClass> >& radiusHist,
                                       unsigned nPoints,
                                       std::vector<PointPairClass> &selectedPoints,
                                       AppBase::ProgressReporter & progress)
{
    typedef std::multimap<double,PointPairClass> PointPairMap;
    
    // reserve selected points..
    int drawsPerBin = nPoints / radiusHist.size();
    // reallocate output vector.
    selectedPoints.reserve(drawsPerBin*radiusHist.size());
    for (typename std::vector<PointPairMap>::const_iterator bin = radiusHist.begin();
         bin != radiusHist.end(); ++bin) 
    {
        unsigned i=drawsPerBin;
        // copy into vector
        for (typename PointPairMap::const_iterator it= (*bin).begin();
             it != (*bin).end(); ++it) 
        {
            selectedPoints.push_back(it->second);
            // do not extract more than drawsPerBin Point pairs.
            --i;
            if (i == 0)
                break;
        }
        progress.increaseProgress(1.0/radiusHist.size());
    }
}
    
    
    
template <class Img, class VoteImg, class PP>
void AllPointSampler::sampleAllPanoPoints(const std::vector<Img> &imgs,
                                          const std::vector<VoteImg *> &voteImgs,
                                          const std::vector<SrcPanoImage> & src,
                                          const PanoramaOptions & dest,
                                          int nPoints,
                                          float minI,
                                          float maxI,
                                          //std::vector<vigra_ext::PointPair> &points,
                                          std::vector<std::multimap<double, PP > > & radiusHist,
                                          unsigned & nGoodPoints,
                                          unsigned & nBadPoints,
                                          AppBase::ProgressReporter& progress)
{
    typedef typename Img::PixelType PixelType;

    // use 10 bins
    radiusHist.resize(10);
    unsigned pairsPerBin = nPoints / radiusHist.size();

    nGoodPoints = 0;
    nBadPoints = 0;
    vigra_precondition(imgs.size() > 1, "sampleAllPanoPoints: At least two images required");
    vigra_precondition(imgs.size() == src.size(), "number of src images doesn't match");
    
    unsigned nImg = imgs.size();

    vigra::Size2D srcSize = src[0].getSize();
    double maxr = sqrt(((double)srcSize.x)*srcSize.x + ((double)srcSize.y)*srcSize.y) / 2.0;

    // create an array of transforms.
    //std::vector<SpaceTransform> transf(imgs.size());
    std::vector<PTools::Transform*> transf(imgs.size());

    // initialize transforms, and interpolating accessors
    for(unsigned i=0; i < imgs.size(); i++) {
        vigra_precondition(src[i].getSize() == srcSize, "images need to have the same size");
        transf[i] = new PTools::Transform;
        transf[i]->createTransform(src[i], dest);
    }

    for (int y=dest.getROI().top(); y < dest.getROI().bottom(); ++y) {
        for (int x=dest.getROI().left(); x < dest.getROI().right(); ++x) {
            hugin_utils::FDiff2D panoPnt(x,y);
            for (unsigned i=0; i< nImg; i++) {
            // transform pixel
                hugin_utils::FDiff2D p1;
                if(!transf[i]->transformImgCoord(p1, panoPnt))
                    continue;
                vigra::Point2D p1Int(p1.toDiff2D());
                // is inside:
                if (!src[i].isInside(p1Int)) {
                    // point is outside image
                    continue;
                }
                PixelType i1;
                vigra::UInt8 maskI;
                if (imgs[i](p1.x,p1.y, i1, maskI)){
                    float im1 = vigra_ext::getMaxComponent(i1);
                    if (minI > im1 || maxI < im1 || maskI == 0) {
                        // ignore pixels that are too dark or bright
                        continue;
                    }
                    double r1 = hugin_utils::norm((p1 - src[i].getRadialVigCorrCenter())/maxr);

                    // check inner image
                    for (unsigned j=i+1; j < nImg; j++) {
                        hugin_utils::FDiff2D p2;
                        if(!transf[j]->transformImgCoord(p2, panoPnt))
                            continue;
                        vigra::Point2D p2Int(p2.toDiff2D());
                        if (!src[j].isInside(p2Int)) {
                            // point is outside image
                            continue;
                        }
                        PixelType i2;
                        vigra::UInt8 maskI2;
                        if (imgs[j](p2.x, p2.y, i2, maskI2)){
                            float im2 = vigra_ext::getMaxComponent(i2);
                            if (minI > im2 || maxI < im2 || maskI2 == 0) {
                                // ignore pixels that are too dark or bright
                                continue;
                            }
                            double r2 = hugin_utils::norm((p2 - src[j].getRadialVigCorrCenter())/maxr);
                            // add pixel
                            const VoteImg & vimg1 =  *voteImgs[i];
                            const VoteImg & vimg2 =  *voteImgs[j];
                            double laplace = hugin_utils::sqr(vimg1[p1Int]) + hugin_utils::sqr(vimg2[p2Int]);
                            int bin1 = (int)(r1*10);
                            int bin2 = (int)(r2*10);
                            // a center shift might lead to radi > 1.
                            if (bin1 > 9) bin1 = 9;
                            if (bin2 > 9) bin2 = 9;

                            PP pp;
                            if (im1 <= im2) {
                                // choose i1 to be smaller than i2
                                pp = PP(i, i1, p1, r1,   j, i2, p2, r2);
                            } else {
                                pp = PP(j, i2, p2, r2,   i, i1, p1, r1);
                            }

                            // decide which bin should be used.
                            std::multimap<double, PP> * map1 = &radiusHist[bin1];
                            std::multimap<double, PP> * map2 = &radiusHist[bin2];
                            std::multimap<double, PP> * destMap;
                            if (map1->size() == 0) {
                                destMap = map1;
                            } else if (map2->size() == 0) {
                                destMap = map2;
                            } else if (map1->size() < map2->size()) {
                                destMap = map1;
                            } else if (map1->size() > map2->size()) {
                                destMap = map2;
                            } else if (map1->rbegin()->first > map2->rbegin()->first) {
                                // heuristic: insert into bin with higher maximum laplacian filter response
                                // (higher probablity of misregistration).
                                destMap = map1;
                            } else {
                                destMap = map2;
                            }
                            // insert
                            destMap->insert(std::make_pair(laplace,pp));
                            // remove last element if too many elements have been gathered
                            if (destMap->size() > pairsPerBin) {
                                destMap->erase((--(destMap->end())));
                            }
                            nGoodPoints++;
                        }
                    }
                }
            }
        }
        int h = dest.getROI().bottom() - dest.getROI().top();
        if ((y-dest.getROI().top())%(h/10) == 0) {
            progress.increaseProgress(0.1);
        }
    }

    for(unsigned i=0; i < imgs.size(); i++) {
        delete transf[i];
    }
}



template <class Img, class VoteImg, class PP>
void RandomPointSampler::sampleRandomPanoPoints(const std::vector<Img> imgs,
                                                const std::vector<VoteImg *> &voteImgs,
                                                const std::vector<SrcPanoImage> & src,
                                                const PanoramaOptions & dest,
                                                int nPoints,
                                                float minI,
                                                float maxI,
                                                //std::vector<PP> &points,
                                                std::vector<std::multimap<double, PP > > & radiusHist,
                                                unsigned & nBadPoints,
                                                AppBase::ProgressReporter & progress)
{
    typedef typename Img::PixelType PixelType;

    vigra_precondition(imgs.size() > 1, "sampleRandomPanoPoints: At least two images required");
    vigra_precondition(imgs.size() == src.size(), "number of src images doesn't match");
    
    unsigned nImg = imgs.size();

    unsigned nBins = radiusHist.size();
    unsigned pairsPerBin = nPoints / nBins;

    int allPoints = nPoints;

    // create an array of transforms.
    //std::vector<SpaceTransform> transf(imgs.size());
    std::vector<PTools::Transform *> transf(imgs.size());
    std::vector<double> maxr(imgs.size());

    // initialize transforms, and interpolating accessors
    for(unsigned i=0; i < imgs.size(); i++) {
        // same size is not needed?
//        vigra_precondition(src[i].getSize() == srcSize, "images need to have the same size");
        transf[i] = new PTools::Transform;
        transf[i]->createTransform(src[i], dest);
        vigra::Size2D srcSize = src[i].getSize();
        maxr[i] = sqrt(((double)srcSize.x)*srcSize.x + ((double)srcSize.y)*srcSize.y) / 2.0;
    }
    // init random number generator
    boost::mt19937 rng;
    // start with a different seed every time.
    rng.seed(static_cast<unsigned int>(std::time(0)));
    // randomly sample points.
    boost::uniform_int<> distribx(dest.getROI().left(), dest.getROI().right()-1);
    boost::uniform_int<> distriby(dest.getROI().top(), dest.getROI().bottom()-1);

    boost::variate_generator<boost::mt19937&, boost::uniform_int<> >
            randX(rng, distribx);             // glues randomness with mapping
    boost::variate_generator<boost::mt19937&, boost::uniform_int<> >
            randY(rng, distriby);             // glues randomness with mapping

    double percentReported = 0.0;
    
    for (unsigned maxTry = nPoints*5; nPoints > 0 && maxTry > 0; maxTry--) {
        unsigned x = randX();
        unsigned y = randY();
        hugin_utils::FDiff2D panoPnt(x,y);
        for (unsigned i=0; i< nImg; i++) {
            // transform pixel
            PixelType i1;
            hugin_utils::FDiff2D p1;
            if(!transf[i]->transformImgCoord(p1, panoPnt))
                continue;
            vigra::Point2D p1Int(p1.toDiff2D());
            // check if pixel is valid
            if (!src[i].isInside(p1Int)) {
                // point is outside image
                continue;
            }
            vigra::UInt8 maskI;
            if ( imgs[i](p1.x,p1.y, i1, maskI)){
                float im1 = vigra_ext::getMaxComponent(i1);
                if (minI > im1 || maxI < im1) {
                    // ignore pixels that are too dark or bright
                    continue;
                }
                double r1 = hugin_utils::norm((p1 - src[i].getRadialVigCorrCenter())/maxr[i]);
                for (unsigned j=i+1; j < nImg; j++) {
                    PixelType i2;
                    hugin_utils::FDiff2D p2;
                    if(!transf[j]->transformImgCoord(p2, panoPnt))
                        continue;
                    // check if a pixel is inside the source image
                    vigra::Point2D p2Int(p2.toDiff2D());
                    if (!src[j].isInside(p2Int)) {
                        // point is outside image
                        continue;
                    }
                    vigra::UInt8 maskI2;
                    if (imgs[j](p2.x, p2.y, i2, maskI2)){
                        float im2 = vigra_ext::getMaxComponent(i2);
                        if (minI > im2 || maxI < im2) {
                            // ignore pixels that are too dark or bright
                            continue;
                        }
                        // TODO: add check for gradient radius.
                        double r2 = hugin_utils::norm((p2 - src[j].getRadialVigCorrCenter())/maxr[j]);
#if 0
                        // add pixel
                        if (im1 <= im2) {
                            points.push_back(PP(i, i1, p1, r1,   j, i2, p2, r2) );
                        } else {
                            points.push_back(PP(j, i2, p2, r2,   i, i1, p1, r1) );
                        }
#else
                            // add pixel
                            const VoteImg & vimg1 =  *voteImgs[i];
                            const VoteImg & vimg2 =  *voteImgs[j];
                            double laplace = hugin_utils::sqr(vimg1[p1Int]) + hugin_utils::sqr(vimg2[p2Int]);
                            size_t bin1 = (size_t)(r1*nBins);
                            size_t bin2 = (size_t)(r2*nBins);
                            // a center shift might lead to radi > 1.
                            if (bin1+1 > nBins) bin1 = nBins-1;
                            if (bin2+1 > nBins) bin2 = nBins-1;

                            PP pp;
                            if (im1 <= im2) {
                                // choose i1 to be smaller than i2
                                pp = PP(i, i1, p1, r1,   j, i2, p2, r2);
                            } else {
                                pp = PP(j, i2, p2, r2,   i, i1, p1, r1);
                            }

                            // decide which bin should be used.
                            std::multimap<double, PP> * map1 = &radiusHist[bin1];
                            std::multimap<double, PP> * map2 = &radiusHist[bin2];
                            std::multimap<double, PP> * destMap;
                            if (map1->size() == 0) {
                                destMap = map1;
                            } else if (map2->size() == 0) {
                                destMap = map2;
                            } else if (map1->size() < map2->size()) {
                                destMap = map1;
                            } else if (map1->size() > map2->size()) {
                                destMap = map2;
							} else if (map1->rbegin()->first > map2->rbegin()->first) {
                                // heuristic: insert into bin with higher maximum laplacian filter response
                                // (higher probablity of misregistration).
                                destMap = map1;
                            } else {
                                destMap = map2;
                            }
                            // insert
                            destMap->insert(std::make_pair(laplace,pp));
                            // remove last element if too many elements have been gathered
                            if (destMap->size() > pairsPerBin) {
                                destMap->erase((--(destMap->end())));
                            }
//                            nGoodPoints++;
#endif
                        nPoints--;
                    }
                }
            }
        }
        double pc = (allPoints - nPoints);
        double percentNow = (pc / allPoints) * 100.0;
        if (percentNow - percentReported >= 10) {
            percentReported = percentNow;
            progress.increaseProgress(0.1);
        }
    }
    for(unsigned i=0; i < imgs.size(); i++) {
        delete transf[i];
    }
    
    DEBUG_INFO("Point sampled: " << allPoints-nPoints)
    progress.increaseProgress(1.0 - percentReported/100.0);
}


} // namespace
#endif //_H
