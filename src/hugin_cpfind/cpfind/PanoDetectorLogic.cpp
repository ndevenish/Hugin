// -*- c-basic-offset: 4 ; tab-width: 4 -*-
/*
* Copyright (C) 2007-2008 Anael Orlinski
*
* This file is part of Panomatic.
*
* Panomatic is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* Panomatic is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Panomatic; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "ImageImport.h"

#include "PanoDetector.h"
#include <iostream>
#include <fstream>
#include <boost/foreach.hpp>
#include <vigra/distancetransform.hxx>
#include "vigra/impex.hxx"   // debug image save

#include <localfeatures/Sieve.h>
#include <localfeatures/PointMatch.h>
#include <localfeatures/RansacFiltering.h>
#include <localfeatures/KeyPointIO.h>
#include <localfeatures/CircularKeyPointDescriptor.h>

/*
#include "KDTree.h"
#include "KDTreeImpl.h"
*/
#include "Utils.h"
#include "Tracer.h"

#include <algorithms/nona/ComputeImageROI.h>
#include <algorithms/optimizer/PTOptimizer.h>
#include <nona/RemappedPanoImage.h>
#include <nona/ImageRemapper.h>

#include <time.h>

#define TRACE_IMG(X) {if (iPanoDetector.getVerbose() > 1) { TRACE_INFO("i" << ioImgInfo._number << " : " << X << endl);} }
#define TRACE_PAIR(X) {if (iPanoDetector.getVerbose() > 1){ TRACE_INFO("i" << ioMatchData._i1->_number << " <> " \
                "i" << ioMatchData._i2->_number << " : " << X << endl)}}

using namespace std;
using namespace lfeat;
using namespace HuginBase;
using namespace AppBase;
using namespace HuginBase::Nona;
using namespace hugin_utils;


static ZThread::FastMutex aPanoToolsMutex;

// define a Keypoint insertor
class KeyPointVectInsertor : public lfeat::KeyPointInsertor
{
public:
    KeyPointVectInsertor(KeyPointVect_t& iVect) : _v(iVect) {};
    inline virtual void operator()(const lfeat::KeyPoint& k)
    {
        _v.push_back(KeyPointPtr(new lfeat::KeyPoint(k)));
    }

private:
    KeyPointVect_t& _v;

};


// define a sieve extractor
class SieveExtractorKP : public lfeat::SieveExtractor<KeyPointPtr>
{
public:
    SieveExtractorKP(KeyPointVect_t& iV) : _v(iV) {};
    inline virtual void operator()(const KeyPointPtr& k)
    {
        _v.push_back(k);
    }
private:
    KeyPointVect_t& _v;
};

class SieveExtractorMatch : public lfeat::SieveExtractor<lfeat::PointMatchPtr>
{
public:
    SieveExtractorMatch(lfeat::PointMatchVector_t& iM) : _m(iM) {};
    inline virtual void operator()(const lfeat::PointMatchPtr& m)
    {
        _m.push_back(m);
    }
private:
    lfeat::PointMatchVector_t& _m;
};

bool PanoDetector::LoadKeypoints(ImgData& ioImgInfo, const PanoDetector& iPanoDetector)
{
    TRACE_IMG("Loading keypoints...");

    ImageInfo info = lfeat::loadKeypoints(ioImgInfo._keyfilename, ioImgInfo._kp);
    ioImgInfo._loadFail = (info.filename.size() == 0);

    // update ImgData
    if(ioImgInfo._needsremap)
    {
        ioImgInfo._detectWidth = max(info.width,info.height);
        ioImgInfo._detectHeight = max(info.width,info.height);
        ioImgInfo._projOpts.setWidth(ioImgInfo._detectWidth);
        ioImgInfo._projOpts.setHeight(ioImgInfo._detectHeight);
    }
    else
    {
        ioImgInfo._detectWidth = info.width;
        ioImgInfo._detectHeight = info.height;
    };
    ioImgInfo._descLength = info.dimensions;

    return true;
}

vigra::RGBValue<float> gray2RGB(float const& v)
{
    return vigra::RGBValue<float>(v,v,v);
}

template <class SrcImageIterator, class SrcAccessor>
void applyMaskAndCrop(vigra::triple<SrcImageIterator, SrcImageIterator, SrcAccessor> img, const HuginBase::SrcPanoImage& SrcImg)
{
    vigra::Diff2D imgSize = img.second - img.first;

    // create dest y iterator
    SrcImageIterator yd(img.first);
    // loop over the image and transform
    for(int y=0; y < imgSize.y; ++y, ++yd.y)
    {
        // create x iterators
        SrcImageIterator xd(yd);
        for(int x=0; x < imgSize.x; ++x, ++xd.x)
        {
            if(!SrcImg.isInside(vigra::Point2D(x,y)))
            {
                *xd=0;
            };
        }
    }
}

// save some intermediate images to disc if defined
//#define DEBUG_LOADING_REMAPPING
bool PanoDetector::AnalyzeImage(ImgData& ioImgInfo, const PanoDetector& iPanoDetector)
{
    vigra::DImage final_img;
    vigra::BImage final_mask;

    try
    {
        ioImgInfo._loadFail=false;

        TRACE_IMG("Load image...");
        vigra::ImageImportInfo aImageInfo(ioImgInfo._name.c_str());
        vigra::FRGBImage RGBimg(aImageInfo.width(), aImageInfo.height());
        vigra::BImage mask;

        if (aImageInfo.numBands() == 1)
        {
            // grayscale image, convert to RGB. This is kind of stupid, but celeste wants RGB images...
            vigra::FImage tmpImg(aImageInfo.width(), aImageInfo.height());
            if (aImageInfo.numExtraBands() == 0)
            {
                vigra::importImage(aImageInfo, destImage(tmpImg));
            }
            else if (aImageInfo.numExtraBands() == 1)
            {
                mask.resize(aImageInfo.size());
                importImageAlpha(aImageInfo, destImage(tmpImg), destImage(mask));
            }
            else
            {
                TRACE_INFO("Image with multiple alpha channels are not supported");
                ioImgInfo._loadFail = true;
                return false;
            }
            //vigra::GrayToRGBAccessor<vigra::RGBValue<float> > ga;
            RGBimg.resize(aImageInfo.size());
            vigra::transformImage(srcImageRange(tmpImg), destImage(RGBimg), &gray2RGB);
        }
        else
        {
            if(aImageInfo.numExtraBands() == 1)
            {
                mask.resize(aImageInfo.size());
                importImageAlpha(aImageInfo, destImage(RGBimg), destImage(mask));
            }
            else
            {
                if (aImageInfo.numExtraBands() == 0)
                {
                    vigra::importImage(aImageInfo, destImage(RGBimg));
                }
                else
                {
                    TRACE_INFO("Image with multiple alpha channels are not supported");
                    ioImgInfo._loadFail = true;
                    return false;
                };
            };
        }

        //convert image, so that all (rgb) values are between 0 and 1
        if(aImageInfo.getPixelType() == std::string("FLOAT") || aImageInfo.getPixelType() == std::string("DOUBLE"))
        {
            vigra::RGBToGrayAccessor<vigra::RGBValue<float> > ga;
            vigra::FindMinMax<float> minmax;   // init functor
            vigra::inspectImage(srcImageRange(RGBimg, ga),minmax);
            double minVal = minmax.min;
            double maxVal = minmax.max;
            vigra_ext::applyMapping(srcImageRange(RGBimg), destImage(RGBimg), minVal, maxVal, 0);
        }
        else
        {
            vigra::transformImage(srcImageRange(RGBimg), destImage(RGBimg),
                                  vigra::functor::Arg1()/vigra::functor::Param(vigra_ext::getMaxValForPixelType(aImageInfo.getPixelType())));
        };

        if(ioImgInfo._needsremap)
        {
            TRACE_IMG("Remap image...");
            RemappedPanoImage<vigra::FRGBImage,vigra::BImage>* remapped=new RemappedPanoImage<vigra::FRGBImage,vigra::BImage>;
            vigra::FImage ffImg;
            MultiProgressDisplay* progress=new DummyMultiProgressDisplay();
            remapped->setPanoImage(iPanoDetector._panoramaInfoCopy.getImage(ioImgInfo._number),
                                   ioImgInfo._projOpts, ioImgInfo._projOpts.getROI());
            if(mask.width()>0)
            {
                remapped->remapImage(vigra::srcImageRange(RGBimg),vigra::srcImage(mask),vigra_ext::INTERP_CUBIC,*progress);
            }
            else
            {
                remapped->remapImage(vigra::srcImageRange(RGBimg),vigra_ext::INTERP_CUBIC,*progress);
            };
            RGBimg.resize(0,0);
            mask.resize(0,0);
            RGBimg=remapped->m_image;
            mask=remapped->m_mask;
            delete remapped;
            delete progress;
        }
        else
        {
            const SrcPanoImage& SrcImg=iPanoDetector._panoramaInfoCopy.getImage(ioImgInfo._number);
            if(SrcImg.hasActiveMasks() || (SrcImg.getCropMode()!=SrcPanoImage::NO_CROP && !SrcImg.getCropRect().isEmpty()))
            {
                if(mask.width()!=aImageInfo.width() || mask.height()!=aImageInfo.height())
                {
                    mask.resize(aImageInfo.size().width(),aImageInfo.size().height(),255);
                };
                //copy mask and crop from pto file into alpha layer
                applyMaskAndCrop(vigra::destImageRange(mask), SrcImg);
            };
        };

        if(iPanoDetector.getCeleste())
        {
            vigra::FRGBImage scaled(ioImgInfo._detectWidth,ioImgInfo._detectHeight);
            if(iPanoDetector._downscale && (!ioImgInfo._needsremap))
            {
                TRACE_IMG("Resize image...");
                vigra::resizeImageNoInterpolation(srcImageRange(RGBimg),destImageRange(scaled));
                if(mask.width()>0 && mask.height()>0)
                {
                    final_mask.resize(ioImgInfo._detectWidth, ioImgInfo._detectHeight);
                    vigra::resizeImageNoInterpolation(srcImageRange(mask),destImageRange(final_mask));
                };
            }
            else
            {
                vigra::copyImage(srcImageRange(RGBimg),destImage(scaled));
                if(mask.width()>0 && mask.height()>0)
                {
                    final_mask.resize(ioImgInfo._detectWidth, ioImgInfo._detectHeight);
                    vigra::copyImage(srcImageRange(mask),destImage(final_mask));
                };
            };
            RGBimg.resize(0,0);
            mask.resize(0,0);
            TRACE_IMG("Mask areas with clouds...");
            vigra::UInt16RGBImage image16(scaled.size());
            vigra::transformImage(srcImageRange(scaled),destImage(image16),vigra::functor::Arg1()*vigra::functor::Param(65535));
            int radius=iPanoDetector.getCelesteRadius();
            if(iPanoDetector._downscale)
            {
                radius>>= 1;
            };
            if(radius<2)
            {
                radius=2;
            };
            vigra::BImage celeste_mask=celeste::getCelesteMask(iPanoDetector.svmModel,image16,radius,iPanoDetector.getCelesteThreshold(),800,true,false);
#ifdef DEBUG_LOADING_REMAPPING
            // DEBUG: export celeste mask
            std::ostringstream maskfilename;
            maskfilename << ioImgInfo._name << "_celeste_mask.JPG";
            vigra::ImageExportInfo maskexinfo(maskfilename.str().c_str());
            vigra::exportImage(srcImageRange(celeste_mask), maskexinfo);
#endif
            image16.resize(0,0);
            if(final_mask.width()>0)
            {
                vigra::copyImageIf(srcImageRange(celeste_mask),srcImage(final_mask),destImage(final_mask));
            }
            else
            {
                final_mask.resize(ioImgInfo._detectWidth,ioImgInfo._detectHeight);
                vigra::copyImage(srcImageRange(celeste_mask),destImage(final_mask));
            };
            celeste_mask.resize(0,0);
            TRACE_IMG("Convert to greyscale double...");
            final_img.resize(ioImgInfo._detectWidth,ioImgInfo._detectHeight);
            vigra::copyImage(scaled.upperLeft(), scaled.lowerRight(), vigra::RGBToGrayAccessor<vigra::RGBValue<double> >(),
                             final_img.upperLeft(), vigra::DImage::Accessor());
            scaled.resize(0,0);
        }
        else
        {
            //without celeste
            final_img.resize(ioImgInfo._detectWidth,ioImgInfo._detectHeight);
            if (iPanoDetector._downscale && !ioImgInfo._needsremap)
            {
                // Downscale and convert to grayscale double format
                TRACE_IMG("Resize to greyscale double...");
                vigra::resizeImageNoInterpolation(
                    RGBimg.upperLeft(),
                    RGBimg.upperLeft() + vigra::Diff2D(aImageInfo.width(), aImageInfo.height()),
                    vigra::RGBToGrayAccessor<vigra::RGBValue<double> >(),
                    final_img.upperLeft(),
                    final_img.lowerRight(),
                    vigra::DImage::Accessor());
                RGBimg.resize(0,0);
                //downscale mask
                if(mask.width()>0 && mask.height()>0)
                {
                    final_mask.resize(ioImgInfo._detectWidth, ioImgInfo._detectHeight);
                    vigra::resizeImageNoInterpolation(srcImageRange(mask),destImageRange(final_mask));
                    mask.resize(0,0);
                };
            }
            else
            {
                // convert to grayscale
                TRACE_IMG("Convert to greyscale double...");
                vigra::copyImage(RGBimg.upperLeft(), RGBimg.lowerRight(), vigra::RGBToGrayAccessor<vigra::RGBValue<double> >(),
                                 final_img.upperLeft(), vigra::DImage::Accessor());
                RGBimg.resize(0,0);
                if(mask.width()>0 && mask.height()>0)
                {
                    final_mask.resize(ioImgInfo._detectWidth, ioImgInfo._detectHeight);
                    vigra::copyImage(srcImageRange(mask),destImage(final_mask));
                    mask.resize(0,0);
                };
            };
        };

        //now scale image from 0..1 to 0..255
        vigra::transformImage(srcImageRange(final_img),destImage(final_img),vigra::functor::Arg1()*vigra::functor::Param(255));

#ifdef DEBUG_LOADING_REMAPPING
        // DEBUG: export remapped
        std::ostringstream filename;
        filename << ioImgInfo._name << "_grey.JPG";
        vigra::ImageExportInfo exinfo(filename.str().c_str());
        vigra::exportImage(srcImageRange(final_img), exinfo);
#endif

        // Build integral image
        TRACE_IMG("Build integral image...");
        ioImgInfo._ii.init(final_img);
        final_img.resize(0,0);

        // compute distance map
        if(final_mask.width()>0 && final_mask.height()>0)
        {
            TRACE_IMG("Build distance map...");
            //apply threshold, in case loaded mask contains other values than 0 and 255
            vigra::transformImage(srcImageRange(final_mask), destImage(final_mask),
                                  vigra::Threshold<vigra::BImage::PixelType, vigra::BImage::PixelType>(1, 255, 0, 255));
            ioImgInfo._distancemap.resize(final_mask.width(),final_mask.height(),0);
            vigra::distanceTransform(srcImageRange(final_mask), destImage(ioImgInfo._distancemap), 255, 2);
#ifdef DEBUG_LOADING_REMAPPING
            std::ostringstream maskfilename;
            maskfilename << ioImgInfo._name << "_mask.JPG";
            vigra::ImageExportInfo maskexinfo(maskfilename.str().c_str());
            vigra::exportImage(srcImageRange(final_mask), maskexinfo);
            std::ostringstream distfilename;
            distfilename << ioImgInfo._name << "_distancemap.JPG";
            vigra::ImageExportInfo distexinfo(distfilename.str().c_str());
            vigra::exportImage(srcImageRange(ioImgInfo._distancemap), distexinfo);
#endif
            final_mask.resize(0,0);
        };
    }
    catch (std::exception& e)
    {
        TRACE_INFO("An error happened while loading image : caught exception: " << e.what() << endl);
        ioImgInfo._loadFail=true;
        return false;
    }

    return true;
}


bool PanoDetector::FindKeyPointsInImage(ImgData& ioImgInfo, const PanoDetector& iPanoDetector)
{
    TRACE_IMG("Find keypoints...");

    // setup the detector
    KeyPointDetector aKP;

    // detect the keypoints
    KeyPointVectInsertor aInsertor(ioImgInfo._kp);
    aKP.detectKeypoints(ioImgInfo._ii, aInsertor);

    TRACE_IMG("Found "<< ioImgInfo._kp.size() << " interest points.");

    return true;
}

bool PanoDetector::FilterKeyPointsInImage(ImgData& ioImgInfo, const PanoDetector& iPanoDetector)
{
    TRACE_IMG("Filtering keypoints...");

    lfeat::Sieve<lfeat::KeyPointPtr, lfeat::KeyPointPtrSort > aSieve(iPanoDetector.getSieve1Width(),
            iPanoDetector.getSieve1Height(),
            iPanoDetector.getSieve1Size());

    // insert the points in the Sieve if they are not masked
    double aXF = (double)iPanoDetector.getSieve1Width() / (double)ioImgInfo._detectWidth;
    double aYF = (double)iPanoDetector.getSieve1Height() / (double)ioImgInfo._detectHeight;

    bool distmap_valid=(ioImgInfo._distancemap.width()>0 && ioImgInfo._distancemap.height()>0);
    BOOST_FOREACH(KeyPointPtr& aK, ioImgInfo._kp)
    {
        if(distmap_valid)
        {
            if(aK->_x > 0 && aK->_x < ioImgInfo._distancemap.width() && aK->_y > 0 && aK->_y < ioImgInfo._distancemap.height()
                    && ioImgInfo._distancemap((int)(aK->_x),(int)(aK->_y)) >aK->_scale*8)
            {
                //cout << " dist from border:" << ioImgInfo._distancemap((int)(aK->_x),(int)(aK->_y)) << " required dist: " << aK->_scale*12 << std::endl;
                aSieve.insert(aK, (int)(aK->_x * aXF), (int)(aK->_y * aYF));
            }
        }
        else
        {
            aSieve.insert(aK, (int)(aK->_x * aXF), (int)(aK->_y * aYF));
        };
    }

    // pull remaining values from the sieve
    ioImgInfo._kp.clear();

    // make an extractor and pull the points
    SieveExtractorKP aSieveExt(ioImgInfo._kp);
    aSieve.extract(aSieveExt);

    TRACE_IMG("Kept " << ioImgInfo._kp.size() << " interest points.");

    return true;

}

bool PanoDetector::MakeKeyPointDescriptorsInImage(ImgData& ioImgInfo, const PanoDetector& iPanoDetector)
{
    TRACE_IMG("Make keypoint descriptors...");

    // build a keypoint descriptor
    CircularKeyPointDescriptor aKPD(ioImgInfo._ii);

    // vector for keypoints with more than one orientation
    KeyPointVect_t kp_new_ori;
    BOOST_FOREACH(KeyPointPtr& aK, ioImgInfo._kp)
    {
        double angles[4];
        int nAngles = aKPD.assignOrientation(*aK, angles);
        for (int i=0; i < nAngles; i++)
        {
            // duplicate Keypoint with additional angles
            KeyPointPtr aKn = KeyPointPtr ( new lfeat::KeyPoint ( *aK ) );
            aKn->_ori = angles[i];
            kp_new_ori.push_back(aKn);
        }
    }
    ioImgInfo._kp.insert(ioImgInfo._kp.end(), kp_new_ori.begin(), kp_new_ori.end());

    BOOST_FOREACH(KeyPointPtr& aK, ioImgInfo._kp)
    {
        aKPD.makeDescriptor(*aK);
    }
    // store the descriptor length
    ioImgInfo._descLength = aKPD.getDescriptorLength();
    return true;
}

bool PanoDetector::RemapBackKeypoints(ImgData& ioImgInfo, const PanoDetector& iPanoDetector)
{

    double scale=iPanoDetector._downscale ? 2.0:1.0;

    if (!ioImgInfo._needsremap)
    {
        if(scale != 1.0)
        {
            BOOST_FOREACH(KeyPointPtr& aK, ioImgInfo._kp)
            {
                aK->_x *= scale;
                aK->_y *= scale;
                aK->_scale *= scale;
            }
        };
    }
    else
    {
        TRACE_IMG("Remapping back keypoints...");
        HuginBase::PTools::Transform trafo1;
        trafo1.createTransform(iPanoDetector._panoramaInfoCopy.getSrcImage(ioImgInfo._number),
                               ioImgInfo._projOpts);

        int dx1 = ioImgInfo._projOpts.getROI().left();
        int dy1 = ioImgInfo._projOpts.getROI().top();

        BOOST_FOREACH(KeyPointPtr& aK, ioImgInfo._kp)
        {
            double xout, yout;
            if(trafo1.transformImgCoord(xout, yout, aK->_x + dx1, aK->_y+ dy1))
            {
                // downscaling is take care of by the remapping transform
                // no need for multiplying the scale factor...
                aK->_x=xout;
                aK->_y=yout;
                aK->_scale *= scale;
            }
        }
    }
    return true;
}

bool PanoDetector::BuildKDTreesInImage(ImgData& ioImgInfo, const PanoDetector& iPanoDetector)
{
    TRACE_IMG("Build KDTree...");

    if(ioImgInfo._kp.size()==0)
    {
        return false;
    };
    // build a vector of KDElemKeyPointPtr

    // create feature vector matrix for flann
    ioImgInfo._flann_descriptors = flann::Matrix<double>(new double[ioImgInfo._kp.size()*ioImgInfo._descLength],
                                   ioImgInfo._kp.size(), ioImgInfo._descLength);
    int i = 0;
    BOOST_FOREACH(KeyPointPtr& aK, ioImgInfo._kp)
    {
        memcpy(ioImgInfo._flann_descriptors[i++], aK->_vec, sizeof(double)*ioImgInfo._descLength);
    }

    // build query structure
    ioImgInfo._flann_index = new flann::Index<flann::L2<double> > (ioImgInfo._flann_descriptors, flann::KDTreeIndexParams(4));
    ioImgInfo._flann_index->buildIndex();

    return true;
}

bool PanoDetector::FreeMemoryInImage(ImgData& ioImgInfo, const PanoDetector& iPanoDetector)
{
    TRACE_IMG("Freeing memory...");

    ioImgInfo._ii.clean();
    ioImgInfo._distancemap.resize(0,0);

    return true;
}


bool PanoDetector::FindMatchesInPair(MatchData& ioMatchData, const PanoDetector& iPanoDetector)
{
    TRACE_PAIR("Find Matches...");

    // retrieve the KDTree of image 2
    flann::Index<flann::L2<double> > * index2 = ioMatchData._i2->_flann_index;

    // retrieve query points from image 1
    flann::Matrix<double> & query = ioMatchData._i1->_flann_descriptors;

    // storage for sorted 2 best matches
    int nn = 2;
    flann::Matrix<int> indices(new int[query.rows*nn], query.rows, nn);
    flann::Matrix<double> dists(new double[query.rows*nn], query.rows, nn);

    // perform matching using flann
    index2->knnSearch(query, indices, dists, nn, flann::SearchParams(iPanoDetector.getKDTreeSearchSteps()));

    //typedef KDTreeSpace::BestMatch<KDElemKeyPoint>		BM_t;
    //std::set<BM_t, std::greater<BM_t> >	aBestMatches;

    // store the matches already found to avoid 2 points in image1
    // match the same point in image2
    // both matches will be removed.
    set<int> aAlreadyMatched;
    set<int> aBadMatch;

    // unfiltered vector of matches
    typedef std::pair<KeyPointPtr, int> TmpPair_t;
    std::vector<TmpPair_t>	aUnfilteredMatches;

    //PointMatchVector_t aMatches;

    // go through all the keypoints of image 1
    for (unsigned aKIt = 0; aKIt < query.rows; ++aKIt)
    {
        // accept the match if the second match is far enough
        // put a lower value for stronger matching default 0.15
        if (dists[aKIt][0] > iPanoDetector.getKDTreeSecondDistance()  * dists[aKIt][1])
        {
            continue;
        }

        // check if the kdtree match number is already in the already matched set
        if (aAlreadyMatched.find(indices[aKIt][0]) != aAlreadyMatched.end())
        {
            // add to delete list and continue
            aBadMatch.insert(indices[aKIt][0]);
            continue;
        }

        // TODO: add check for duplicate matches (can happen if a keypoint gets multiple orientations)

        // add the match number in already matched set
        aAlreadyMatched.insert(indices[aKIt][0]);

        // add the match to the unfiltered list
        aUnfilteredMatches.push_back(TmpPair_t(ioMatchData._i1->_kp[aKIt], indices[aKIt][0]));
    }

    // now filter and fill the vector of matches
    BOOST_FOREACH(TmpPair_t& aP, aUnfilteredMatches)
    {
        // if the image2 match number is in the badmatch set, skip it.
        if (aBadMatch.find(aP.second) != aBadMatch.end())
        {
            continue;
        }

        // add the match in the output vector
        ioMatchData._matches.push_back(lfeat::PointMatchPtr( new lfeat::PointMatch(aP.first, ioMatchData._i2->_kp[aP.second])));
    }

    TRACE_PAIR("Found " << ioMatchData._matches.size() << " matches.");
    return true;
}

bool PanoDetector::RansacMatchesInPair(MatchData& ioMatchData, const PanoDetector& iPanoDetector)
{
    // Use panotools model for wide angle lenses
    RANSACOptimizer::Mode rmode = iPanoDetector._ransacMode;
    if (rmode == RANSACOptimizer::HOMOGRAPHY ||
            (rmode == RANSACOptimizer::AUTO && iPanoDetector._panoramaInfo->getImage(ioMatchData._i1->_number).getHFOV() < 65 &&
             iPanoDetector._panoramaInfo->getImage(ioMatchData._i2->_number).getHFOV() < 65))
    {
        return RansacMatchesInPairHomography(ioMatchData, iPanoDetector);
    }
    else
    {
        return RansacMatchesInPairCam(ioMatchData, iPanoDetector);
    }
}

// new code with fisheye aware ransac
bool PanoDetector::RansacMatchesInPairCam(MatchData& ioMatchData, const PanoDetector& iPanoDetector)
{
    TRACE_PAIR("RANSAC Filtering with Panorama model...");

    if (ioMatchData._matches.size() < (unsigned int)iPanoDetector.getMinimumMatches())
    {
        TRACE_PAIR("Too few matches ... removing all of them.");
        ioMatchData._matches.clear();
        return true;
    }

    if (ioMatchData._matches.size() < 6)
    {
        TRACE_PAIR("Not enough matches for RANSAC filtering.");
        return true;
    }

    // setup a panorama project with the two images.
    // is this threadsafe (is this read only access?)
    UIntSet imgs;
    int pano_i1 = ioMatchData._i1->_number;
    int pano_i2 = ioMatchData._i2->_number;
    imgs.insert(pano_i1);
    imgs.insert(pano_i2);
    int pano_local_i1 = 0;
    int pano_local_i2 = 1;
    if (pano_i1 > pano_i2)
    {
        pano_local_i1 = 1;
        pano_local_i2 = 0;
    }

    // perform ransac matching.
    // ARGH the panotools optimizer uses global variables is not reentrant
    std::vector<int> inliers;
    {
        ZThread::Guard<ZThread::FastMutex> g(aPanoToolsMutex);

        PanoramaData* panoSubset = iPanoDetector._panoramaInfo->getNewSubset(imgs);

        // create control point vector
        CPVector controlPoints(ioMatchData._matches.size());
        int i=0;
        BOOST_FOREACH(PointMatchPtr& aM, ioMatchData._matches)
        {
            controlPoints[i] = ControlPoint(pano_local_i1, aM->_img1_x, aM->_img1_y,
                                            pano_local_i2, aM->_img2_x, aM->_img2_y);
            i++;
        }
        panoSubset->setCtrlPoints(controlPoints);


        PT_setProgressFcn(ptProgress);
        PT_setInfoDlgFcn(ptinfoDlg);

        RANSACOptimizer::Mode rmode = iPanoDetector._ransacMode;
        if (rmode == RANSACOptimizer::AUTO)
        {
            rmode = RANSACOptimizer::RPY;
        }
        inliers = HuginBase::RANSACOptimizer::findInliers(*panoSubset, pano_local_i1, pano_local_i2,
                  iPanoDetector.getRansacDistanceThreshold(), rmode);
        PT_setProgressFcn(NULL);
        PT_setInfoDlgFcn(NULL);
        delete panoSubset;

        TRACE_PAIR("Removed " << controlPoints.size() - inliers.size() << " matches. " << inliers.size() << " remaining.");
        if (inliers.size() < 0.5 * controlPoints.size())
        {
            // more than 50% of matches were removed, ignore complete pair...
            TRACE_PAIR("RANSAC found more than 50% outliers, removing all matches");
            ioMatchData._matches.clear();
            return true;
        }
    }


    if (inliers.size() < (unsigned int)iPanoDetector.getMinimumMatches())
    {
        TRACE_PAIR("Too few matches ... removing all of them.");
        ioMatchData._matches.clear();
        return true;
    }

    // keep only inlier matches
    PointMatchVector_t aInlierMatches;
    aInlierMatches.reserve(inliers.size());

    BOOST_FOREACH(int idx, inliers)
    {
        aInlierMatches.push_back(ioMatchData._matches[idx]);
    }
    ioMatchData._matches = aInlierMatches;

    /*
    if (iPanoDetector.getTest())
    	TestCode::drawRansacMatches(ioMatchData._i1->_name, ioMatchData._i2->_name, ioMatchData._matches,
    								aRemovedMatches, aRansacFilter, iPanoDetector.getDownscale());
    */

    return true;
}

// homography based ransac matching
bool PanoDetector::RansacMatchesInPairHomography(MatchData& ioMatchData, const PanoDetector& iPanoDetector)
{
    TRACE_PAIR("RANSAC Filtering...");

    if (ioMatchData._matches.size() < (unsigned int)iPanoDetector.getMinimumMatches())
    {
        TRACE_PAIR("Too few matches ... removing all of them.");
        ioMatchData._matches.clear();
        return true;
    }

    if (ioMatchData._matches.size() < 6)
    {
        TRACE_PAIR("Not enough matches for RANSAC filtering.");
        return true;
    }

    PointMatchVector_t aRemovedMatches;

    Ransac aRansacFilter;
    aRansacFilter.setIterations(iPanoDetector.getRansacIterations());
    int thresholdDistance=iPanoDetector.getRansacDistanceThreshold();
    //increase RANSAC distance if the image were remapped to not exclude
    //too much points in this case
    if(ioMatchData._i1->_needsremap || ioMatchData._i2->_needsremap)
    {
        thresholdDistance*=5;
    }
    aRansacFilter.setDistanceThreshold(thresholdDistance);
    aRansacFilter.filter(ioMatchData._matches, aRemovedMatches);


    TRACE_PAIR("Removed " << aRemovedMatches.size() << " matches. " << ioMatchData._matches.size() << " remaining.");

    if (aRemovedMatches.size() > ioMatchData._matches.size())
    {
        // more than 50% of matches were removed, ignore complete pair...
        TRACE_PAIR("More than 50% outliers, removing all matches");
        ioMatchData._matches.clear();
        return true;
    }

    if (iPanoDetector.getTest())
        TestCode::drawRansacMatches(ioMatchData._i1->_name, ioMatchData._i2->_name, ioMatchData._matches,
                                    aRemovedMatches, aRansacFilter, iPanoDetector.getDownscale());

    return true;

}


bool PanoDetector::FilterMatchesInPair(MatchData& ioMatchData, const PanoDetector& iPanoDetector)
{
    TRACE_PAIR("Clustering matches...");

    if (ioMatchData._matches.size() < 2)
    {
        return true;
    }

    // compute min,max of x,y for image1

    double aMinX = numeric_limits<double>::max();
    double aMinY = numeric_limits<double>::max();
    double aMaxX = -numeric_limits<double>::max();
    double aMaxY = -numeric_limits<double>::max();

    BOOST_FOREACH(PointMatchPtr& aM, ioMatchData._matches)
    {
        if (aM->_img1_x < aMinX)
        {
            aMinX = aM->_img1_x;
        }
        if (aM->_img1_x > aMaxX)
        {
            aMaxX = aM->_img1_x;
        }

        if (aM->_img1_y < aMinY)
        {
            aMinY = aM->_img1_y;
        }
        if (aM->_img1_y > aMaxY)
        {
            aMaxY = aM->_img1_y;
        }
    }

    double aSizeX = aMaxX - aMinX + 2; // add 2 so max/aSize is strict < 1
    double aSizeY = aMaxY - aMinY + 2;

    //

    Sieve<PointMatchPtr, PointMatchPtrSort> aSieve(iPanoDetector.getSieve2Width(),
            iPanoDetector.getSieve2Height(),
            iPanoDetector.getSieve2Size());

    // insert the points in the Sieve
    double aXF = (double)iPanoDetector.getSieve2Width() / aSizeX;
    double aYF = (double)iPanoDetector.getSieve2Height() / aSizeY;
    int aCount = 0;
    BOOST_FOREACH(PointMatchPtr& aM, ioMatchData._matches)
    {
        aSieve.insert(aM, (int)((aM->_img1_x - aMinX) * aXF), (int)((aM->_img1_y - aMinY) * aYF));
        aCount++;
    }

    // pull remaining values from the sieve
    ioMatchData._matches.clear();

    // make an extractor and pull the points
    SieveExtractorMatch aSieveExt(ioMatchData._matches);
    aSieve.extract(aSieveExt);

    TRACE_PAIR("Kept " << ioMatchData._matches.size() << " matches.");
    return true;
}

void PanoDetector::writeOutput()
{
    // Write output pto file

    ofstream aOut(_outputFile.c_str(), ios_base::trunc);
    if( !aOut )
    {
        cerr << "ERROR : "
             << "Couldn't open file '" << _outputFile << "'!" << endl; //STS
        return;
    }

    aOut << "# pto project file generated by Hugin's cpfind" << endl << endl;

    _panoramaInfo->removeDuplicateCtrlPoints();
    AppBase::DocumentData::ReadWriteError err = _panoramaInfo->writeData(aOut);
    if (err != AppBase::DocumentData::SUCCESSFUL)
    {
        cerr << "ERROR couldn't write to output file '" << _outputFile << "'!" << endl;
        return;
    }
}

void PanoDetector::writeKeyfile(ImgData& imgInfo)
{
    // Write output keyfile

    ofstream aOut(imgInfo._keyfilename.c_str(), ios_base::trunc);

    SIFTFormatWriter writer(aOut);

    int origImgWidth =  _panoramaInfo->getImage(imgInfo._number).getSize().width();
    int origImgHeight =  _panoramaInfo->getImage(imgInfo._number).getSize().height();

    ImageInfo img_info(imgInfo._name, origImgWidth, origImgHeight);

    writer.writeHeader ( img_info, imgInfo._kp.size(), imgInfo._descLength );

    BOOST_FOREACH ( KeyPointPtr& aK, imgInfo._kp )
    {
        writer.writeKeypoint ( aK->_x, aK->_y, aK->_scale, aK->_ori, aK->_score,
                               imgInfo._descLength, aK->_vec );
    }
    writer.writeFooter();
}

