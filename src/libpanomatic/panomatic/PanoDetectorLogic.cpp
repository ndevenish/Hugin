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
	
	std::string keyfilename = ioImgInfo._name;
	keyfilename.append(".key");

	ImageInfo info = lfeat::loadKeypoints(keyfilename, ioImgInfo._kp);
	ioImgInfo._loadFail = (info.filename.size() == 0);

	// update ImgData
	ioImgInfo._detectWidth = info.width;
	ioImgInfo._detectHeight = info.height;
	ioImgInfo._descLength = info.dimensions;

	return true;
}

/*bool PanoDetector::AnalyzeImage(ImgData& ioImgInfo, const PanoDetector& iPanoDetector)
{
	TRACE_IMG("Analyze image...");
	try
	{
      ioImgInfo._loadFail = false;
			
		//get image info
      vigra::ImageImportInfo aImageInfo(ioImgInfo._name.c_str());

	    int aNewImgWidth = aImageInfo.width();
	    int aNewImgHeight = aImageInfo.height();

	    if (iPanoDetector.getDownscale())
	    {
		    aNewImgWidth >>= 1;
		    aNewImgHeight >>= 1;
	    }

	    vigra::DImage aImageDouble(aNewImgWidth, aNewImgHeight);

	    if(aImageInfo.isGrayscale())
	    {
		    if (iPanoDetector.getDownscale())
		    {
			    TRACE_IMG("Load greyscale...");
			    vigra::DImage aImageG(aImageInfo.width(), aImageInfo.height());
			    importImage(aImageInfo, destImage(aImageG));
			    vigra::resizeImageNoInterpolation(
				    aImageG.upperLeft(),
				    aImageG.upperLeft() + vigra::Diff2D(aNewImgWidth * 2, aNewImgHeight * 2),
				    vigra::DImage::Accessor(),
				    aImageDouble.upperLeft(),
				    aImageDouble.lowerRight(),
				    vigra::DImage::Accessor());
		    }
		    else
		    {
			    TRACE_IMG("Load greyscale...");
			    importImage(aImageInfo, destImage(aImageDouble));
		    }
	    }
	    else
	    {
		    TRACE_IMG("Load RGB...");
		    //open the image in RGB
		    vigra::DRGBImage aImageRGB(aImageInfo.width(), aImageInfo.height());

            if(aImageInfo.numExtraBands() == 1) 
            {
                vigra::BImage aAlpha(aImageInfo.size());
                //importImageAlpha(aImageInfo, destImage(aImageRGB), destImage(aAlpha));
            } 
            else if (aImageInfo.numExtraBands() == 0) 
            {
                vigra::importImage(aImageInfo, destImage(aImageRGB));

            }
            else
            {
                TRACE_INFO("Image with multiple alpha channels are not supported");
                ioImgInfo._loadFail = true;
                return false;
            }

		    if (iPanoDetector.getDownscale())
		    {
			    TRACE_IMG("Resize to greyscale double...");
			    vigra::resizeImageNoInterpolation(
					    aImageRGB.upperLeft(),
					    aImageRGB.upperLeft() + vigra::Diff2D(aNewImgWidth * 2, aNewImgHeight * 2),
					    vigra::RGBToGrayAccessor<vigra::RGBValue<double> >(),
					    aImageDouble.upperLeft(),
					    aImageDouble.lowerRight(),
					    vigra::DImage::Accessor());

		    }
		    else
		    {			
			    // convert to greyscale
			    TRACE_IMG("Convert to greyscale double...");
			    vigra::copyImage(	aImageRGB.upperLeft(),
				    aImageRGB.lowerRight(),
				    vigra::RGBToGrayAccessor<vigra::RGBValue<double> >(),
				    aImageDouble.upperLeft(),
				    vigra::DImage::Accessor());
		    }
	    }

        // store info
        ioImgInfo._detectWidth = aNewImgWidth;
        ioImgInfo._detectHeight = aNewImgHeight;

        TRACE_IMG("Build integral image...");
        // create integral image
        ioImgInfo._ii.init(aImageDouble.begin(), aNewImgWidth, aNewImgHeight);
    }
    catch (std::exception & e)
    {
        TRACE_INFO("An error happened while loading image : caught exception: " << e.what() << endl);
        return false;
    }

	return true;
}
*/

bool PanoDetector::AnalyzeImage(ImgData& ioImgInfo, const PanoDetector& iPanoDetector)
{
	// Define Progress
	AppBase::StreamMultiProgressDisplay progress(cout);

	// Load and format images
	FileRemapper<vigra::BRGBImage, vigra::BImage> remapper;	//TODO : Grayscale images case
	progress.pushTask(ProgressTask("Loading Images", "", 1.0));

	vigra::DImage final_img(ioImgInfo._detectWidth, ioImgInfo._detectHeight);

	// Remap image to stereographic if needed
	if(ioImgInfo._needsremap)
	{
		// Get the image remapped
		RemappedPanoImage<vigra::BRGBImage, vigra::BImage> * remapped = 
			remapper.getRemapped(iPanoDetector._panoramaInfoCopy, ioImgInfo._projOpts, ioImgInfo._number, 
										vigra::Rect2D(0,0,ioImgInfo._detectWidth, ioImgInfo._detectHeight), progress);
		vigra::BRGBImage RGBimg = remapped->m_image;

   	remapper.release(remapped);

		// Convert to grayscale double format
		vigra::copyImage(
			RGBimg.upperLeft(),
			RGBimg.lowerRight(),
			vigra::RGBToGrayAccessor<vigra::RGBValue<double> >(),
			final_img.upperLeft(),
			vigra::DImage::Accessor());

	} else {// Simply load images
	   vigra::ImageImportInfo aImageInfo(ioImgInfo._name.c_str());
		vigra::BRGBImage RGBimg(aImageInfo.width(), aImageInfo.height());
	   vigra::importImage(aImageInfo, destImage(RGBimg));

		if (iPanoDetector._downscale)
	   {	// Downscale and convert to grayscale double format
			vigra::resizeImageNoInterpolation(
				RGBimg.upperLeft(),
				RGBimg.upperLeft() + vigra::Diff2D(aImageInfo.width(), aImageInfo.height()),
				vigra::RGBToGrayAccessor<vigra::RGBValue<double> >(),
				final_img.upperLeft(),
				final_img.lowerRight(),
				vigra::DImage::Accessor());
		} else { // convert to grayscale
		   vigra::copyImage(
				RGBimg.upperLeft(),
			   RGBimg.lowerRight(),
			   vigra::RGBToGrayAccessor<vigra::RGBValue<double> >(),
			   final_img.upperLeft(),
			   vigra::DImage::Accessor());
		}
	}

	// Build integral image
   ioImgInfo._ii.init(final_img.begin(), ioImgInfo._detectWidth,
																 ioImgInfo._detectHeight);
	ioImgInfo._loadFail = false;

	// DEBUG: export remapped image
    std::ostringstream filename;
	 filename << ioImgInfo._name << "STEREO.JPG"; // opts.getOutputExtension()
	 vigra::ImageExportInfo exinfo(filename.str().c_str());
    vigra::exportImage(srcImageRange(final_img), exinfo);
	progress.popTask();
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

	// insert the points in the Sieve
	double aXF = (double)iPanoDetector.getSieve1Width() / (double)ioImgInfo._detectWidth;
	double aYF = (double)iPanoDetector.getSieve1Height() / (double)ioImgInfo._detectHeight;
	BOOST_FOREACH(KeyPointPtr& aK, ioImgInfo._kp)
		aSieve.insert(aK, (int)(aK->_x * aXF), (int)(aK->_y * aYF)); 

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
	aRansacFilter.setDistanceThreshold(iPanoDetector.getRansacDistanceThreshold());
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
								  ioMatchData._i1->_projOpts);

   double xout,yout;
	//double scale=iPanoDetector._downscale ? 2.0:1.0;
	BOOST_FOREACH(PointMatchPtr& aM, ioMatchData._matches)
	{
		if(trafo1.transformImgCoord(xout, yout, aM->_img1_x, aM->_img1_y))
		{
		   aM->_img1_x=xout;
		   aM->_img1_y=yout;
		}
		if(trafo2.transformImgCoord(xout, yout, aM->_img2_x, aM->_img2_y))
		{
		   aM->_img2_x=xout;
		   aM->_img2_y=yout;
		}
/*		aM->_img1_x*=scale;
		aM->_img1_y*=scale;
		aM->_img2_x*=scale;
		aM->_img2_y*=scale;*/
	}
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

	std::string keyfilename = imgInfo._name;
	keyfilename.append(".key");

	ofstream aOut(keyfilename.c_str(), ios_base::trunc);

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

