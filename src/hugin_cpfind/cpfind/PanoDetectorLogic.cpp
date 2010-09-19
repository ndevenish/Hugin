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

#include <localfeatures/Sieve.h>
#include <localfeatures/PointMatch.h>
#include <localfeatures/RansacFiltering.h>
#include <localfeatures/KeyPointIO.h>
#include <localfeatures/CircularKeyPointDescriptor.h>

#include "KDTree.h"
#include "KDTreeImpl.h"
#include "Utils.h"
#include "Tracer.h"

#include <algorithms/nona/ComputeImageROI.h>
#include <nona/RemappedPanoImage.h>
#include <nona/ImageRemapper.h>

#include <time.h>

#define TRACE_IMG(X) TRACE_INFO("i" << ioImgInfo._number << " : " << X << endl)
#define TRACE_PAIR(X) TRACE_INFO("i" << ioMatchData._i1->_number << " <> " \
								 "i" << ioMatchData._i2->_number << " : " << X << endl)

using namespace std;
using namespace lfeat;
using namespace HuginBase;
using namespace AppBase;
using namespace HuginBase::Nona;
using namespace hugin_utils;

// define a Keypoint insertor
class KeyPointVectInsertor : public lfeat::KeyPointInsertor
{
public:
	KeyPointVectInsertor(KeyPointVect_t& iVect) : _v(iVect) {};
	inline virtual void operator()(const lfeat::KeyPoint &k) 
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
	inline virtual void operator()(const KeyPointPtr &k)
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
	inline virtual void operator()(const lfeat::PointMatchPtr &m)
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

bool PanoDetector::AnalyzeImage(ImgData& ioImgInfo, const PanoDetector& iPanoDetector)
{
	vigra::DImage final_img(ioImgInfo._detectWidth, ioImgInfo._detectHeight);
    vigra::BImage final_mask;

    try
    {
        ioImgInfo._loadFail=false;
    	// Remap image to stereographic if needed
	    if(ioImgInfo._needsremap)
    	{
            TRACE_IMG("Remapping image...");
    		// Load and format images
            MultiProgressDisplay* progress=new DummyMultiProgressDisplay();
	    	FileRemapper<vigra::BRGBImage, vigra::BImage> remapper;	//TODO : Grayscale images case
		    progress->pushTask(ProgressTask("", "", 1.0));
	    	RemappedPanoImage<vigra::BRGBImage, vigra::BImage> * remapped = 
		    	remapper.getRemapped(iPanoDetector._panoramaInfoCopy, ioImgInfo._projOpts, ioImgInfo._number, 
			    							vigra::Rect2D(0,0,ioImgInfo._detectWidth, ioImgInfo._detectHeight), *progress);
    		vigra::BRGBImage RGBimg = remapped->m_image;
	    	final_mask = remapped->m_mask;
		    progress->popTask();
       	    remapper.release(remapped);

    		// Convert to grayscale double format
            TRACE_IMG("Convert to greyscale double...");
	    	vigra::copyImage(
		    	RGBimg.upperLeft(),
			    RGBimg.lowerRight(),
    			vigra::RGBToGrayAccessor<vigra::RGBValue<double> >(),
	    		final_img.upperLeft(),
		    	vigra::DImage::Accessor());
        
            delete progress;
	    }
        else
        {
            // Simply load images
	        vigra::ImageImportInfo aImageInfo(ioImgInfo._name.c_str());
            vigra::BRGBImage RGBimg(aImageInfo.width(), aImageInfo.height());
            vigra::BImage mask;

            TRACE_IMG("Loading image...");
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
                    if(iPanoDetector._panoramaInfoCopy.getImage(ioImgInfo._number).hasActiveMasks())
                    {
                        mask.resize(aImageInfo.size().width(),aImageInfo.size().height(),255);
                    };
                }
                else
                {
                    TRACE_INFO("Image with multiple alpha channels are not supported");
                    ioImgInfo._loadFail = true;
                    return false;
                };
            };

            if(iPanoDetector._panoramaInfoCopy.getImage(ioImgInfo._number).hasActiveMasks())
            {
                //copy mask from pto file into alpha layer
                vigra_ext::applyMask(vigra::destImageRange(mask), iPanoDetector._panoramaInfoCopy.getImage(ioImgInfo._number).getActiveMasks());
            };

            if (iPanoDetector._downscale)
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
                if(mask.width()>0 && mask.height()>0)
                {
                    final_mask.resize(ioImgInfo._detectWidth, ioImgInfo._detectHeight);          
                    vigra::copyImage(srcImageRange(mask),destImage(final_mask));
                    mask.resize(0,0);
                };
            }
        }

        // Build integral image
        TRACE_IMG("Build integral image...");
        ioImgInfo._ii.init(final_img.begin(), ioImgInfo._detectWidth,ioImgInfo._detectHeight);

        // compute distance map
        if(final_mask.width()>0 && final_mask.height()>0)
        {
            TRACE_IMG("Building distance map...");
            //apply threshold, in case loaded mask contains other values than 0 and 255
            vigra::transformImage(srcImageRange(final_mask), destImage(final_mask),
                vigra::Threshold<vigra::BImage::PixelType, vigra::BImage::PixelType>(1, 255, 0, 255));
            ioImgInfo._distancemap.resize(final_mask.width(),final_mask.height(),0);
            vigra::distanceTransform(srcImageRange(final_mask), destImage(ioImgInfo._distancemap), 255, 2);
        };
    }
    catch (std::exception & e)
    {
        TRACE_INFO("An error happened while loading image : caught exception: " << e.what() << endl);
        ioImgInfo._loadFail=true;
        return false;
    }

#if 0
	// DEBUG: export remapped image
    std::ostringstream filename;
    filename << ioImgInfo._name << "STEREO.JPG"; // opts.getOutputExtension()
    vigra::ImageExportInfo exinfo(filename.str().c_str());
    vigra::exportImage(srcImageRange(final_img), exinfo);
#endif

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
	
	BOOST_FOREACH(KeyPointPtr& aK, ioImgInfo._kp)
	{
        bool valid=(ioImgInfo._distancemap.width()==0 || ioImgInfo._distancemap.height()==0);
        if(!valid)
        {
            if(aK->_x > 0 && aK->_x < ioImgInfo._distancemap.width() && aK->_y > 0 && aK->_y < ioImgInfo._distancemap.height())
            {
                valid=(ioImgInfo._distancemap((int)(aK->_x),(int)(aK->_y))>aK->_scale*10);
            };
        };
        if(valid)
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
		BOOST_FOREACH(KeyPointPtr& aK, ioImgInfo._kp)
		{
			aKPD.assignOrientation(*aK);
			aKPD.makeDescriptor(*aK);
		}
		// store the descriptor length
		ioImgInfo._descLength = aKPD.getDescriptorLength();
	return true;
}

bool PanoDetector::BuildKDTreesInImage(ImgData& ioImgInfo, const PanoDetector& iPanoDetector)
{
	TRACE_IMG("Build KDTree...");

	// build a vector of KDElemKeyPointPtr

	int aCount = 0;
	BOOST_FOREACH(KeyPointPtr& aK, ioImgInfo._kp)
	{
		ioImgInfo._kdv.push_back(KDElemKeyPoint(aK, aCount++));
	}

	ioImgInfo._kd = KPKDTreePtr(new KDTreeSpace::KDTree<KDElemKeyPoint, double>(ioImgInfo._kdv, ioImgInfo._descLength));

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
	KPKDTree& aKDTree2 = *(ioMatchData._i2->_kd);

	// storage for sorted 2 best matches
	typedef KDTreeSpace::BestMatch<KDElemKeyPoint>		BM_t;
	std::set<BM_t, std::greater<BM_t> >	aBestMatches;

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
	for (unsigned aKIt = 0; aKIt < ioMatchData._i1->_kdv.size(); ++aKIt)
	{
		// find the 2 best nearest neighbors
		aBestMatches.clear();
		aBestMatches = aKDTree2.getNearestNeighboursBBF(ioMatchData._i1->_kdv[aKIt], 2, iPanoDetector.getKDTreeSearchSteps());
		
		if (aBestMatches.size() < 2)
			continue;
		
		const BM_t& aBest = *(++aBestMatches.begin());
		const BM_t& aSecond = *(aBestMatches.begin());

		// accept the match if the second match is far enough
		// put a lower value for stronger matching default 0.15
		if (aBest._distance > iPanoDetector.getKDTreeSecondDistance()  * aSecond._distance) 
			continue;

		// check if the kdtree match number is already in the already matched set
		if (aAlreadyMatched.find(aBest._match->_n) != aAlreadyMatched.end())
		{
			// add to delete list and continue
			aBadMatch.insert(aBest._match->_n);
			continue;
		}

		// add the match number in already matched set
		aAlreadyMatched.insert(aBest._match->_n);

		// add the match to the unfiltered list
		aUnfilteredMatches.push_back(TmpPair_t(ioMatchData._i1->_kp[aKIt], aBest._match->_n));

	}

	// now filter and fill the vector of matches
	BOOST_FOREACH(TmpPair_t& aP, aUnfilteredMatches)
	{
		// if the image2 match number is in the badmatch set, skip it.
		if (aBadMatch.find(aP.second) != aBadMatch.end())
			continue;

		// add the match in the output vector
		ioMatchData._matches.push_back(lfeat::PointMatchPtr( new lfeat::PointMatch(aP.first, ioMatchData._i2->_kp[aP.second])));

	}

	TRACE_PAIR("Found " << ioMatchData._matches.size() << " matches.");
	return true;
}


bool PanoDetector::RansacMatchesInPair(MatchData& ioMatchData, const PanoDetector& iPanoDetector)
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
        thresholdDistance*=5;
	aRansacFilter.setDistanceThreshold(thresholdDistance);
	aRansacFilter.filter(ioMatchData._matches, aRemovedMatches);
	
	TRACE_PAIR("Removed " << aRemovedMatches.size() << " matches. " << ioMatchData._matches.size() << " remaining.");

	if (iPanoDetector.getTest())
		TestCode::drawRansacMatches(ioMatchData._i1->_name, ioMatchData._i2->_name, ioMatchData._matches, 
									aRemovedMatches, aRansacFilter, iPanoDetector.getDownscale());

	return true;

}

bool PanoDetector::FilterMatchesInPair(MatchData& ioMatchData, const PanoDetector& iPanoDetector)
{
	TRACE_PAIR("Clustering matches...");

	if (ioMatchData._matches.size() < 2)
		return true;

	// compute min,max of x,y for image1

	double aMinX = numeric_limits<double>::max();
	double aMinY = numeric_limits<double>::max();
	double aMaxX = -numeric_limits<double>::max();
	double aMaxY = -numeric_limits<double>::max();

	BOOST_FOREACH(PointMatchPtr& aM, ioMatchData._matches)
	{
		if (aM->_img1_x < aMinX) aMinX = aM->_img1_x;
		if (aM->_img1_x > aMaxX) aMaxX = aM->_img1_x;

		if (aM->_img1_y < aMinY) aMinY = aM->_img1_y;
		if (aM->_img1_y > aMaxY) aMaxY = aM->_img1_y;
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

bool PanoDetector::RemapBackMatches(MatchData& ioMatchData, const PanoDetector& iPanoDetector)
{
    TRACE_PAIR("Remapping back matches...");

    HuginBase::PTools::Transform trafo1, trafo2;
    trafo1.createTransform(iPanoDetector._panoramaInfoCopy.getSrcImage(ioMatchData._i1->_number),
                           ioMatchData._i1->_projOpts);
    trafo2.createTransform(iPanoDetector._panoramaInfoCopy.getSrcImage(ioMatchData._i2->_number),
                           ioMatchData._i2->_projOpts);

    double xout,yout;
    double scale=iPanoDetector._downscale ? 2.0:1.0;
    BOOST_FOREACH(PointMatchPtr& aM, ioMatchData._matches)
    {
        if(ioMatchData._i1->_needsremap)
        {
            if(trafo1.transformImgCoord(xout, yout, aM->_img1_x, aM->_img1_y))
            {
                aM->_img1_x=xout;
                aM->_img1_y=yout;
            }
        }
        else
        {
            if(!ioMatchData._i1->_hasakeyfile)
            {
                // we don't do scale, if we loaded keyfiles from disc, because
                // in keyfiles the keypoints are always for image with full resolution
                aM->_img1_x*=scale;
                aM->_img1_y*=scale;
            };
        }
        if(ioMatchData._i2->_needsremap)
        {
            if(trafo2.transformImgCoord(xout, yout, aM->_img2_x, aM->_img2_y))
            {
                 aM->_img2_x=xout;
                 aM->_img2_y=yout;
            }
        }
        else
        {
            if(!ioMatchData._i2->_hasakeyfile)
            {
                aM->_img2_x*=scale;
                aM->_img2_y*=scale;
            };
        }
    }
    return true;
}

void PanoDetector::writeOutput()
{
	// Write output pto file

	ofstream aOut(_outputFile.c_str(), ios_base::trunc);
	if( !aOut ) {
		cerr << "ERROR : "
			<< "Couldn't open file '" << _outputFile << "'!" << endl; //STS
		return;
	}

	aOut << "# pto project file generated by panomatic" << endl << endl;

	AppBase::DocumentData::ReadWriteError err = _panoramaInfo->writeData(aOut);
	if (err != AppBase::DocumentData::SUCCESSFUL) {
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
		if (getDownscale())
		{
		writer.writeKeypoint ( aK->_x * 2.0, aK->_y * 2.0, aK->_scale * 2.0, aK->_ori,
		                       imgInfo._descLength, aK->_vec );
		}
		else
		{
		writer.writeKeypoint ( aK->_x, aK->_y, aK->_scale, aK->_ori,
		                       imgInfo._descLength, aK->_vec );
		}
	}
	writer.writeFooter();
}

