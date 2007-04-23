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

#ifndef _RANDOM_POINT_SAMPLER_H
#define _RANDOM_POINT_SAMPLER_H

#include <fstream>
#include <algorithm>
#include <ctime>

#include <boost/random.hpp>

#include <vigra/convolution.hxx>
#include <vigra/impex.hxx>
#include <vigra_ext/impexalpha.hxx>
#include <common/math.h>

#include <vigra_ext/VignettingCorrection.h>
#include <vigra_ext/Pyramid.h>

#include <PT/PanoImage.h>
#include <PT/PanoToolsInterface.h>
//#include <PT/SpaceTransform.h>

namespace PT
{


    
/** sample all points inside a panorama and check for create a
 *  bins of point pairs that include a specific radius.
 *
 */
template <class Img, class VoteImg, class PP>
void sampleAllPanoPoints(const std::vector<Img> &imgs,
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
                         utils::ProgressReporter & progress)
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
            FDiff2D panoPnt(x,y);
            for (unsigned i=0; i< nImg; i++) {
            // transform pixel
                FDiff2D p1;
                transf[i]->transformImgCoord(p1, panoPnt);
                vigra::Point2D p1Int(p1.toDiff2D());
                // is inside:
                if (!src[i].isInside(p1Int)) {
                    // point is outside image
                    continue;
                }
                PixelType i1;
                vigra::UInt8 maskI;
                if (imgs[i](p1.x,p1.y, i1, maskI)){
                    float im1 = getMaxComponent(i1);
                    if (minI > im1 || maxI < im1 || maskI == 0) {
                        // ignore pixels that are too dark or bright
                        continue;
                    }
                    double r1 = utils::norm((p1 - src[i].getRadialVigCorrCenter())/maxr);

                    // check inner image
                    for (unsigned j=i+1; j < nImg; j++) {
                        FDiff2D p2;
                        transf[j]->transformImgCoord(p2, panoPnt);
                        vigra::Point2D p2Int(p2.toDiff2D());
                        if (!src[j].isInside(p2Int)) {
                            // point is outside image
                            continue;
                        }
                        PixelType i2;
                        vigra::UInt8 maskI2;
                        if (imgs[j](p2.x, p2.y, i2, maskI2)){
                            float im2 = getMaxComponent(i2);
                            if (minI > im2 || maxI < im2 || maskI2 == 0) {
                                // ignore pixels that are too dark or bright
                                continue;
                            }
                            double r2 = utils::norm((p2 - src[j].getRadialVigCorrCenter())/maxr);
                            // add pixel
                            const VoteImg & vimg1 =  *voteImgs[i];
                            const VoteImg & vimg2 =  *voteImgs[j];
                            double laplace = utils::sqr(vimg1[p1Int]) + utils::sqr(vimg2[p2Int]);
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
                            } else if (map1->rend()->first > map2->rend()->first) {
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


/** extract some random points out of the bins.
 *  This should ensure that the radius distribution is
 *  roughly uniform
 */
inline void sampleRadiusUniform(const std::vector<std::multimap<double, vigra_ext::PointPair> > & radiusHist,
                         unsigned nPoints,
                         std::vector<vigra_ext::PointPair> &selectedPoints)
{
    typedef std::vector<std::multimap<double, vigra_ext::PointPair> > TBins;
    // reserve selected points..
    int drawsPerBin = nPoints / radiusHist.size();
    // reallocate output vector.
    selectedPoints.reserve(drawsPerBin*radiusHist.size());
    for (TBins::const_iterator bin= radiusHist.begin();
         bin != radiusHist.end(); ++bin) 
    {
        unsigned i=drawsPerBin;
        // copy into vector
        for (std::multimap<double, vigra_ext::PointPair>::const_iterator it= (*bin).begin();
             it != (*bin).end(); ++it) 
        {
            selectedPoints.push_back(it->second);
            // do not extract more than drawsPerBin Point pairs.
            --i;
            if (i == 0)
                break;
        }
    }
}

inline void sampleRadiusUniformRGB(const std::vector<std::multimap<double, vigra_ext::PointPairRGB> > & radiusHist,
                         unsigned nPoints,
                         std::vector<vigra_ext::PointPairRGB> &selectedPoints,
                         utils::ProgressReporter & progress)
{
    typedef std::vector<std::multimap<double, vigra_ext::PointPairRGB> > TBins;
    // reserve selected points..
    int drawsPerBin = nPoints / radiusHist.size();
    // reallocate output vector.
    selectedPoints.reserve(drawsPerBin*radiusHist.size());
    for (TBins::const_iterator bin= radiusHist.begin();
         bin != radiusHist.end(); ++bin) 
    {
        unsigned i=drawsPerBin;
        // copy into vector
        for (std::multimap<double, vigra_ext::PointPairRGB>::const_iterator it= (*bin).begin();
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
void sampleRandomPanoPoints(const std::vector<Img> imgs,
                            const std::vector<VoteImg *> &voteImgs,
                            const std::vector<SrcPanoImage> & src,
                            const PanoramaOptions & dest,
                            int nPoints,
                            float minI,
                            float maxI,
                            //std::vector<PP> &points,
                            std::vector<std::multimap<double, PP > > & radiusHist,
                            unsigned & nBadPoints,
                            utils::ProgressReporter & progress)
{
    typedef typename Img::PixelType PixelType;

    vigra_precondition(imgs.size() > 1, "sampleRandomPanoPoints: At least two images required");
    vigra_precondition(imgs.size() == src.size(), "number of src images doesn't match");
    
    unsigned nImg = imgs.size();

    vigra::Size2D srcSize = src[0].getSize();
    double maxr = sqrt(((double)srcSize.x)*srcSize.x + ((double)srcSize.y)*srcSize.y) / 2.0;
    unsigned nBins = radiusHist.size();
    unsigned pairsPerBin = nPoints / nBins;

    int allPoints = nPoints;

    // create an array of transforms.
    //std::vector<SpaceTransform> transf(imgs.size());
    std::vector<PTools::Transform *> transf(imgs.size());

    // initialize transforms, and interpolating accessors
    for(unsigned i=0; i < imgs.size(); i++) {
        vigra_precondition(src[i].getSize() == srcSize, "images need to have the same size");
        transf[i] = new PTools::Transform;
        transf[i]->createTransform(src[i], dest);
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

    for (unsigned maxTry = nPoints*5; nPoints > 0 && maxTry > 0; maxTry--) {
        unsigned x = randX();
        unsigned y = randY();
        FDiff2D panoPnt(x,y);
        for (unsigned i=0; i< nImg; i++) {
            // transform pixel
            PixelType i1;
            FDiff2D p1;
            transf[i]->transformImgCoord(p1, panoPnt);
            vigra::Point2D p1Int(p1.toDiff2D());
            // check if pixel is valid
            if (!src[i].isInside(p1Int)) {
                // point is outside image
                continue;
            }
            vigra::UInt8 maskI;
            if ( imgs[i](p1.x,p1.y, i1, maskI)){
                float im1 = getMaxComponent(i1);
                if (minI > im1 || maxI < im1) {
                    // ignore pixels that are too dark or bright
                    continue;
                }
                double r1 = utils::norm((p1 - src[i].getRadialVigCorrCenter())/maxr);
                for (unsigned j=i+1; j < nImg; j++) {
                    PixelType i2;
                    FDiff2D p2;
                    transf[j]->transformImgCoord(p2, panoPnt);
                    // check if a pixel is inside the source image
                    vigra::Point2D p2Int(p2.toDiff2D());
                    if (!src[j].isInside(p2Int)) {
                        // point is outside image
                        continue;
                    }
                    vigra::UInt8 maskI2;
                    if (imgs[j](p2.x, p2.y, i2, maskI2)){
                        float im2 = getMaxComponent(i2);
                        if (minI > im2 || maxI < im2) {
                            // ignore pixels that are too dark or bright
                            continue;
                        }
                        // TODO: add check for gradient radius.
                        double r2 = utils::norm((p2 - src[j].getRadialVigCorrCenter())/maxr);
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
                            double laplace = utils::sqr(vimg1[p1Int]) + utils::sqr(vimg2[p2Int]);
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
                            } else if (map1->rend()->first > map2->rend()->first) {
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
        if (((int)pc)%(allPoints/10) == 0) {
            progress.increaseProgress(1.0/10);
        }
    }
    for(unsigned i=0; i < imgs.size(); i++) {
        delete transf[i];
    }
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

inline void extractPoints(Panorama pano, std::vector<vigra::FRGBImage*> images, int nPoints,
                   bool randomPoints, utils::ProgressReporter & progress,
                   std::vector<vigra_ext::PointPairRGB> & points  )
{
    std::vector<vigra::FImage *> lapImgs;
    std::vector<vigra::Size2D> origsize;
    std::vector<SrcPanoImage> srcDescr;

    // convert to interpolating images
    typedef vigra_ext::ImageInterpolator<vigra::FRGBImage::const_traverser, vigra::FRGBImage::ConstAccessor, vigra_ext::interp_cubic> InterpolImg;
    std::vector<InterpolImg> interpolImages;

    vigra_ext::interp_cubic interp;
    // adjust sizes in panorama
    for (unsigned i=0; i < pano.getNrOfImages(); i++) {
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

    PanoramaOptions opts = pano.getOptions();
    // extract the overlapping points.
    double scale = calcOptimalPanoScale(pano.getSrcImage(0),opts);

    opts.setWidth(utils::roundi(opts.getWidth()*scale), true);
    pano.setOptions(opts);

    // if random points.
    // extract random points.
    // need to get the maximum gray value here..


    std::vector<std::multimap<double, vigra_ext::PointPairRGB> > radiusHist(10);

    unsigned nGoodPoints = 0;
    unsigned nBadPoints = 0;
    if (randomPoints) {
        progress.setMessage("sampling random points");
        PT::sampleRandomPanoPoints(interpolImages,
                                   lapImgs, srcDescr,
                                   pano.getOptions(),
                                   nPoints*5,
                                   1/255.0,
                                   250/255.0,
                                   radiusHist,
                                   nBadPoints,
                                   progress);
    } else {
        progress.setMessage("sampling all points");
        sampleAllPanoPoints(interpolImages,
                            lapImgs, srcDescr,
                            opts,
                            nPoints, 1/255.0, 250/255.0,
                            radiusHist,
                            nGoodPoints,
                            nBadPoints,
                            progress);
    }
    // select points with low laplacian of gaussian values.
    progress.setMessage("extracting good points");
    PT::sampleRadiusUniformRGB(radiusHist, nPoints, points, progress);

    for (size_t i=0; i < images.size(); i++) {
        delete images[i];
        if (!randomPoints) delete lapImgs[i];
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
}

// needs 2.0 progress steps
inline void loadImgsAndExtractPoints(Panorama pano, int nPoints, int pyrLevel, bool randomPoints, utils::ProgressReporter & progress, std::vector<vigra_ext::PointPairRGB> & points  )
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

}; // namespace

#endif
