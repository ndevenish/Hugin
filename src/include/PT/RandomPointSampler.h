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

#include <common/math.h>

#include <vigra_ext/VignettingCorrection.h>

#include <PT/PanoImage.h>
#include <PT/PanoToolsInterface.h>
//#include <PT/SpaceTransform.h>

namespace PT
{

/** sample all points inside a panorama and check for create a
 *  bins of point pairs that include a specific radius.
 *
 */
template <class Img, class VoteImg>
void sampleAllPanoPoints(const std::vector<Img> &imgs,
                         const std::vector<VoteImg *> &voteImgs,
                         const std::vector<SrcPanoImage> & src,
                         const PanoramaOptions & dest,
                         int nPoints,
                         typename Img::PixelType minI,
                         typename Img::PixelType maxI,
                         //std::vector<vigra_ext::PointPair> &points,
                         std::vector<std::multimap<double, vigra_ext::PointPair> > & radiusHist,
                         unsigned & nGoodPoints,
                         unsigned & nBadPoints,
                         utils::MultiProgressDisplay & progress)
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
                if (imgs[i](p1.x,p1.y, i1)){
                    if (minI > i1 || maxI < i1) {
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
                        if (imgs[j](p2.x, p2.y, i2)){
                            if (minI > i2 || maxI < i2) {
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

                            vigra_ext::PointPair pp;
                            if (i1 >= i2 && r1 <= r2) {
                                // ok, point is good. i1 is closer to centre, swap point
                                // so that i1 < i2
                                pp = vigra_ext::PointPair(j, i2, p2, r2,   i, i1, p1, r1);
                            } else if( i1 <= i2 && r1 >= r2) {
                                pp = vigra_ext::PointPair(i, i1, p1, r1,   j, i2, p2, r2);
                            } else {
                                // point is inconsistent
                                nBadPoints++;
                                continue;
                            }

                            // decide which bin should be used.
                            std::multimap<double, vigra_ext::PointPair> * map1 = &radiusHist[bin1];
                            std::multimap<double, vigra_ext::PointPair> * map2 = &radiusHist[bin2];
                            std::multimap<double, vigra_ext::PointPair> * destMap;
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
        if (h > 100) {
            if ((y-dest.getROI().top())%(h/20) == 0) {
                progress.setProgress(((double)y-dest.getROI().top())/h);
            }
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
void sampleRadiusUniform(const std::vector<std::multimap<double, vigra_ext::PointPair> > & radiusHist,
                         unsigned nPoints,
                         std::vector<vigra_ext::PointPair> &selectedPoints)
{
    // reserve selected points..
    int drawsPerBin = nPoints / radiusHist.size();
    // reallocate output vector.
    selectedPoints.reserve(drawsPerBin*radiusHist.size());
    for (std::vector<std::multimap<double, vigra_ext::PointPair> >::const_iterator bin= radiusHist.begin();
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

template <class Img>
void sampleRandomPanoPoints(const std::vector<Img> &imgs,
                            const std::vector<SrcPanoImage> & src,
                            const PanoramaOptions & dest,
                            int nPoints,
                            typename Img::PixelType minI,
                            typename Img::PixelType maxI,
                            std::vector<vigra_ext::PointPair> &points,
                            unsigned & nBadPoints)
{
    typedef typename Img::PixelType PixelType;

    vigra_precondition(imgs.size() > 1, "sampleRandomPanoPoints: At least two images required");
    vigra_precondition(imgs.size() == src.size(), "number of src images doesn't match");
    
    unsigned nImg = imgs.size();

    vigra::Size2D srcSize = src[0].getSize();
    double maxr = sqrt(((double)srcSize.x)*srcSize.x + ((double)srcSize.y)*srcSize.y) / 2.0;

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

// DGSW FIXME - Unreferenced
//	unsigned n_good=0;
//	unsigned n_bad=0;
    for (unsigned maxTry = nPoints*10; nPoints > 0 && maxTry > 0; maxTry--) {
        unsigned x = randX();
        unsigned y = randY();
        FDiff2D panoPnt(x,y);
        for (unsigned i=0; i< nImg; i++) {
            // transform pixel
            PixelType i1;
            FDiff2D p1;
            transf[i]->transformImgCoord(p1, panoPnt);
            // check if pixel is valid
            if (imgs[i](p1.x,p1.y, i1)){
                if (minI > i1 || maxI < i1) {
                    // ignore pixels that are too dark or bright
                    continue;
                }
		double r1 = utils::norm((p1 - src[i].getRadialVigCorrCenter())/maxr);
                for (unsigned j=i+1; j < nImg; j++) {
                    PixelType i2;
                    FDiff2D p2;
                    transf[j]->transformImgCoord(p2, panoPnt);
                    // check if a pixel is inside the source image
                    if (imgs[j](p2.x, p2.y, i2)){
                        if (minI > i2 || maxI < i2) {
                            // ignore pixels that are too dark or bright
                            continue;
                        }
			double r2 = utils::norm((p2 - src[j].getRadialVigCorrCenter())/maxr);
                        // add pixel
                        if (i1 >= i2 && r1 <= r2) {
                            // ok, point is good. i1 is closer to centre, swap point
                            // so that i1 < i2
                            points.push_back(vigra_ext::PointPair(j, i2, p2, r2,   i, i1, p1, r1) );
                            nPoints--;
                        } else if( i1 <= i2 && r1 >= r2) {
                            points.push_back(vigra_ext::PointPair(i, i1, p1, r1,   j, i2, p2, r2) );
                            nPoints--;
                        } else {
                            nBadPoints++;
                        }
                    }
                }
            }
        }
    }
    for(unsigned i=0; i < imgs.size(); i++) {
        delete transf[i];
    }
}

}; // namespace

#endif
