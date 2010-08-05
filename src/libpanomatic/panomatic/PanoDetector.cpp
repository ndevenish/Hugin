#include "PanoDetector.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <boost/foreach.hpp>

#include <time.h>

#include "zthread/Runnable.h"
#include "zthread/PoolExecutor.h"
#include "Utils.h"
#include "Tracer.h"

#include <algorithms/nona/ComputeImageROI.h>
#include <nona/RemappedPanoImage.h>
#include <nona/ImageRemapper.h>

#include "ImageImport.h"

#ifndef srandom
#define srandom srand
#endif	

using namespace std;
using namespace ZThread;
using namespace HuginBase;
using namespace AppBase;
using namespace HuginBase::Nona;
using namespace hugin_utils;


PanoDetector::PanoDetector() :	_outputFile("default.pto"),
	_writeAllKeyPoints(false),
	_sieve1Width(10), _sieve1Height(10), _sieve1Size(30), 
	_kdTreeSearchSteps(40), _kdTreeSecondDistance(0.15), _sieve2Width(5), _sieve2Height(5),
	_sieve2Size(2), _test(false), _cores(utils::getCPUCount()), _ransacIters(1000), _ransacDistanceThres(25),
	_minimumMatches(4), _linearMatch(false), _linearMatchLen(1), _downscale(true)
{
	_panoramaInfo = new Panorama();
}


bool PanoDetector::checkData()
{
	// test linear match data
	if (_linearMatchLen < 1)
	{
		std::cout << "Linear match length must be at least 1." << std::endl;
		return false;
	}
	
	
	// check the test mode
	if (_test)
	{
		if (_filesData.size() != 2)
		{
			std::cout << "In test mode you must provide exactly 2 images." << std::endl;
			return false;
		}
	}

	return true;
}

void PanoDetector::printDetails()
{
	cout << "Input file        : " << _inputFile << endl;
	if (_writeAllKeyPoints)
	{
		cout << "Output files      :  one keyfile per image" << endl;
	} else if (_keyPointsIdx.size() != 0) {
		cout << "Output file(s)      :  keyfile(s) for images";
			for (unsigned int i = 0; i < _keyPointsIdx.size(); ++i)
				cout << " " << _keyPointsIdx[i] ;
		cout << endl;
	} else {
		cout << "Output file       : " << _outputFile << endl;
	}
	cout << "Number of CPU     : " << _cores << endl << endl;
	cout << "Input image options" << endl;
	cout << "  Downscale to half-size : " << (_downscale?"yes":"no") << endl;
	if (_gradDescriptor) {
		cout << "Gradient based description" << endl;
	} 
	cout << "Sieve 1 Options" << endl;
	cout << "  Width : " << _sieve1Width << endl;
	cout << "  Height : " << _sieve1Height << endl;
	cout << "  Size : " << _sieve1Size << endl;
	cout << "  ==> Maximum keypoints per image : " << _sieve1Size * _sieve1Height * _sieve1Width << endl;
	cout << "KDTree Options" << endl;
	cout << "  Search steps : " << _kdTreeSearchSteps << endl;
	cout << "  Second match distance : " << _kdTreeSecondDistance << endl;
	cout << "Matching Options" << endl;
	cout << "  Mode : " << (_linearMatch?"Linear match":"All pairs"); 
	if (_linearMatch)
		cout << " with length of " << _linearMatchLen << " image" << endl;
	else
		cout << endl;
	cout << "  Distance threshold : " << _ransacDistanceThres << endl;
	cout << "RANSAC Options" << endl;
	cout << "  Iterations : " << _ransacIters << endl;
	cout << "  Distance threshold : " << _ransacDistanceThres << endl;
	cout << "Sieve 2 Options" << endl;
	cout << "  Width : " << _sieve2Width << endl;
	cout << "  Height : " << _sieve2Height << endl;
	cout << "  Size : " << _sieve2Size << endl;
	cout << "  ==> Maximum matches per image pair : " << _sieve2Size * _sieve2Height * _sieve2Width << endl;
}

// definition of a runnable class for image data
class ImgDataRunnable : public Runnable
{
public:
	ImgDataRunnable(PanoDetector::ImgData& iImageData, const PanoDetector& iPanoDetector) :
	  _imgData(iImageData), _panoDetector(iPanoDetector) {};

	  void run() 
	  {	
		  //if (!PanoDetector::AnalyzeImage(_imgData, _panoDetector)) return;
		  PanoDetector::FindKeyPointsInImage(_imgData, _panoDetector);
		  PanoDetector::FilterKeyPointsInImage(_imgData, _panoDetector);
		  PanoDetector::MakeKeyPointDescriptorsInImage(_imgData, _panoDetector);
		  PanoDetector::BuildKDTreesInImage(_imgData, _panoDetector);
		  PanoDetector::FreeMemoryInImage(_imgData, _panoDetector);
	  }
private:
	const PanoDetector&			_panoDetector;
	PanoDetector::ImgData&		_imgData;
};

// definition of a runnable class for writeKeyPoints
class WriteKeyPointsRunnable : public Runnable
{
public:
	WriteKeyPointsRunnable(PanoDetector::ImgData& iImageData, const PanoDetector& iPanoDetector) :
	  _imgData(iImageData), _panoDetector(iPanoDetector) {};

	  void run() 
	  {	
		  //if (!PanoDetector::AnalyzeImage(_imgData, _panoDetector)) return;
		  PanoDetector::FindKeyPointsInImage(_imgData, _panoDetector);
		  PanoDetector::FilterKeyPointsInImage(_imgData, _panoDetector);
		  PanoDetector::MakeKeyPointDescriptorsInImage(_imgData, _panoDetector);
		  PanoDetector::FreeMemoryInImage(_imgData, _panoDetector);
	  }
private:
	const PanoDetector&			_panoDetector;
	PanoDetector::ImgData&		_imgData;
};

// definition of a runnable class for keypoints data
class LoadKeypointsDataRunnable : public Runnable
{
	public:
	LoadKeypointsDataRunnable(PanoDetector::ImgData& iImageData, const PanoDetector& iPanoDetector) :
		_imgData(iImageData), _panoDetector(iPanoDetector) {};

	void run() 
	{	
		PanoDetector::LoadKeypoints(_imgData, _panoDetector);
		PanoDetector::BuildKDTreesInImage(_imgData, _panoDetector);
	}

	private:
		const PanoDetector&			_panoDetector;
		PanoDetector::ImgData&		_imgData;
};

// definition of a runnable class for MatchData
class MatchDataRunnable : public Runnable
{
public:
	MatchDataRunnable(PanoDetector::MatchData& iMatchData, const PanoDetector& iPanoDetector) :
	  _matchData(iMatchData), _panoDetector(iPanoDetector) {};

	  void run() 
	  {	
		  PanoDetector::FindMatchesInPair(_matchData, _panoDetector);
		  PanoDetector::RansacMatchesInPair(_matchData, _panoDetector);
		  PanoDetector::FilterMatchesInPair(_matchData, _panoDetector);
	  }
private:
	const PanoDetector&			_panoDetector;
	PanoDetector::MatchData&	_matchData;
};


void PanoDetector::run()
{
	// init the random time generator
	srandom((unsigned int)time(NULL));
	PoolExecutor aExecutor(_cores);

	// Load the input project file
	if(!loadProject()) return;

	// 1. Load images from the project if no keypoint file is given
	if (_keyPointsIdx.size() == 0 && !_writeAllKeyPoints)
		loadImages();

	// 2. run analysis of images or keypoints
	try 
	{
		if (_keyPointsIdx.size() != 0) {
			TRACE_INFO(endl<< "--- Analyze Keypoints ---" << endl);
			for (unsigned int i = 0; i < _keyPointsIdx.size(); ++i)
				aExecutor.execute(new WriteKeyPointsRunnable(_filesData[_keyPointsIdx[i]], *this));

		} else if (_writeAllKeyPoints) {
			TRACE_INFO(endl<< "--- Analyze Keypoints---" << endl);
			for (ImgDataIt_t aB = _filesData.begin(); aB != _filesData.end(); ++aB)
				aExecutor.execute(new WriteKeyPointsRunnable(aB->second, *this));

		} else {
			TRACE_INFO(endl<< "--- Analyze Images ---" << endl);
			for (ImgDataIt_t aB = _filesData.begin(); aB != _filesData.end(); ++aB)
			{
				if (aB->second._hasakeyfile) {
					aExecutor.execute(new LoadKeypointsDataRunnable(aB->second, *this));

				} else {
					aExecutor.execute(new ImgDataRunnable(aB->second, *this));

				}
			}
		}
		aExecutor.wait();
	} 
	catch(Synchronization_Exception& e)
	{ 
		TRACE_ERROR(e.what() << endl);
		return;
	}

	// check if the load of images succeed.
   if (!checkLoadSuccess())
   {
       TRACE_INFO("One or more images failed to load. Exiting.");
       return;
   }
  
	// Detect matches if writeKeyPoints wasn't set  
	if(!_writeAllKeyPoints && _keyPointsIdx.size() == 0)
	{
		// 3. prepare matches
		prepareMatches();

		// 4. find matches
		TRACE_INFO(endl<< "--- Find matches ---" << endl);
		try 
		{
			BOOST_FOREACH(MatchData& aMD, _matchesData)
				aExecutor.execute(new MatchDataRunnable(aMD, *this));
			aExecutor.wait();
		} 
		catch(Synchronization_Exception& e)
		{ 
			TRACE_ERROR(e.what() << endl);
			return;
		}

		// Remap control points back to Rectilinear projection if necessary
		double scale;
		if(_stereoRemap)
		{
			TRACE_INFO(endl<< "--- Remap Back matches---" << endl << endl);
			remapBackMatches();
         scale=1.0;
		} else {
         scale = _downscale ? 2.0:1.0;
		};
		// Add detected matches to _panoramaInfo
		BOOST_FOREACH(MatchData& aM, _matchesData)
			BOOST_FOREACH(lfeat::PointMatchPtr& aPM, aM._matches)
				_panoramaInfo->addCtrlPoint(ControlPoint(aM._i1->_number, scale*aPM->_img1_x, scale*aPM->_img1_y,
										 							  aM._i2->_number, scale*aPM->_img2_x, scale*aPM->_img2_y));
	}
	
	// 5. write output
	if(_writeAllKeyPoints)
	{ //Write specified keyfiles
		TRACE_INFO(endl<< "--- Write Keyfiles output ---" << endl << endl);
			for (ImgDataIt_t aB = _filesData.begin(); aB != _filesData.end(); ++aB)
				writeKeyfile(aB->second);

	} else if (_keyPointsIdx.size() != 0) { //Write all keyfiles
		TRACE_INFO(endl<< "--- Write Keyfiles output ---" << endl << endl);
			for (unsigned int i = 0; i < _keyPointsIdx.size(); ++i)
				writeKeyfile(_filesData[_keyPointsIdx[i]]);

	} else { /// Write output project
		TRACE_INFO(endl<< "--- Write Project output ---" << endl << endl);
		writeOutput();
	}
}

bool PanoDetector::loadProject()
{
   ifstream ptoFile(_inputFile.c_str());
   if (ptoFile.bad()) { 
       cerr << "ERROR: could not open file: '" << _inputFile << "'!" << endl; 
       return false; 
   } 
	_panoramaInfo->setFilePrefix(hugin_utils::getPathPrefix(_inputFile));
	AppBase::DocumentData::ReadWriteError err = _panoramaInfo->readData(ptoFile);
	if (err != AppBase::DocumentData::SUCCESSFUL) {
		  cerr << "ERROR: couldn't parse panos tool script: '" << _inputFile << "'!" << endl;
		  return false;
	}

	// Create a copy of panoramaInfo that will be used to define
	// image options
	_panoramaInfoCopy=_panoramaInfo->duplicate();

	// Add images found in the project file to _filesData
	int nImg = _panoramaInfo->getNrOfImages();
	for (unsigned int imgNr = 0; imgNr < nImg; ++imgNr)
	{
		// Change image position in the copy
		SrcPanoImage img=_panoramaInfoCopy.getSrcImage(imgNr);
   	img.setYaw(0);
   	img.setRoll(0);
   	img.setPitch(0);
   	img.setX(0);
   	img.setY(0);
   	img.setZ(0);
   	img.setActive(true);
		_panoramaInfoCopy.setImage(imgNr,img);

		// insert the image in the map
		_filesData.insert(make_pair(imgNr, ImgData()));
		
		// get the data
		ImgData& aImgData = _filesData[imgNr];

		// set the name
		aImgData._name = img.getFilename();

		// Number pointing to image info in _panoramaInfo
		aImgData._number = imgNr;

		// Specify if the image has an associated keypoint file
		std::string keyfilename = aImgData._name;
		keyfilename.append(".key");
		ifstream keyfile(keyfilename.c_str());
		aImgData._hasakeyfile = keyfile.good();
	}

	return true;
}

void PanoDetector::loadImages()
{	
	// Define Progress
	AppBase::StreamMultiProgressDisplay progress(cout);

	// All images are remapped to stereographic projection only if
	// one image or more has an hfov >=65
	int nImg = _panoramaInfo->getNrOfImages();
	for (unsigned int imgNr = 0; imgNr < nImg; ++imgNr)
	{
		SrcPanoImage img=_panoramaInfoCopy.getSrcImage(imgNr);
		if(img.getHFOV()>=0)// TODO: >=65
		{
			_stereoRemap = true;
			break;
		}
		_stereoRemap = false;
	}

	// Load and format images
	FileRemapper<vigra::BRGBImage, vigra::BImage> remapper;	//TODO : Grayscale images case
	progress.pushTask(ProgressTask("Loading Images", "", 1.0/(nImg)));
	for (unsigned int imgNr = 0; imgNr < nImg; ++imgNr)
	{
		SrcPanoImage img=_panoramaInfoCopy.getSrcImage(imgNr);

		_filesData[imgNr]._detectWidth = img.getSize().width();
		_filesData[imgNr]._detectHeight = img.getSize().height();

		if (_downscale)
	   {
		   _filesData[imgNr]._detectWidth >>= 1;
		   _filesData[imgNr]._detectHeight >>= 1;
	   }
	   vigra::DImage final_img(_filesData[imgNr]._detectWidth, _filesData[imgNr]._detectHeight);

		// Remap image to stereographic if needed
		if(_stereoRemap)
		{
			// Defines remapped image options
			PanoramaOptions opts = _panoramaInfoCopy.getOptions();
		   opts.setHFOV(img.getHFOV());
		   opts.setWidth(_filesData[imgNr]._detectWidth);
		   opts.setHeight(_filesData[imgNr]._detectHeight);
			opts.setProjection(PanoramaOptions::STEREOGRAPHIC);

			// Get the image remapped
			RemappedPanoImage<vigra::BRGBImage, vigra::BImage> * remapped = 
				remapper.getRemapped(_panoramaInfoCopy, opts, imgNr, 
											vigra::Rect2D(0,0,opts.getWidth(),opts.getHeight()), progress);
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

		   vigra::ImageImportInfo aImageInfo(_filesData[imgNr]._name.c_str());
			vigra::BRGBImage RGBimg(aImageInfo.width(), aImageInfo.height());
		   vigra::importImage(aImageInfo, destImage(RGBimg));

			if (_downscale)
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
      _filesData[imgNr]._ii.init(final_img.begin(), _filesData[imgNr]._detectWidth,
																	 _filesData[imgNr]._detectHeight);
		_filesData[imgNr]._loadFail = false;

		// DEBUG: export remapped image
      // std::ostringstream filename;
		// filename << _filesData[imgNr]._name << "STEREO.JPG"; // opts.getOutputExtension()
		// vigra::ImageExportInfo exinfo(filename.str().c_str());
      // vigra::exportImage(srcImageRange(final_img), exinfo);
	}
	progress.popTask();
}

void PanoDetector::remapBackMatches()
{
		// Remap control points back to rectilinear coordinates
    	HuginBase::PTools::Transform trafo;
		int nImg = _panoramaInfo->getNrOfImages();
		for (unsigned int imgNr = 0; imgNr < nImg; ++imgNr)
		{
			SrcPanoImage img=_panoramaInfoCopy.getSrcImage(imgNr);
			PanoramaOptions opts = _panoramaInfoCopy.getOptions();
		   opts.setHFOV(img.getHFOV());
		   opts.setWidth(_filesData[imgNr]._detectWidth);
         opts.setHeight(_filesData[imgNr]._detectHeight);
			opts.setProjection(PanoramaOptions::STEREOGRAPHIC);
			trafo.createTransform(img,opts);
 
    		double xout,yout;
			BOOST_FOREACH(MatchData& aM, _matchesData)
			{
				BOOST_FOREACH(lfeat::PointMatchPtr& aPM, aM._matches)
				{
					if(aM._i1->_number == imgNr)
					{
						//cout << imgNr << ": B-" << aPM->_img1_x << " " << aPM->_img1_y << endl;
						if(trafo.transformImgCoord(xout, yout, aPM->_img1_x, aPM->_img1_y))
               	{
                     aPM->_img1_x=xout;
	                  aPM->_img1_y=yout;
                  };
						//cout << imgNr << ": A-" << aPM->_img1_x << " " << aPM->_img1_y << endl;
					}
					if(aM._i2->_number == imgNr)
					{
						//cout << imgNr << ": B-" << aPM->_img2_x << " " << aPM->_img2_y << endl;
						if(trafo.transformImgCoord(xout, yout, aPM->_img2_x, aPM->_img2_y))
                  {
	                  aPM->_img2_x=xout;
                     aPM->_img2_y=yout;
                  };
						//cout << imgNr << ": A-" << aPM->_img2_x << " " << aPM->_img2_y << endl;
					}
				}
			}
		}
}

bool PanoDetector::checkLoadSuccess()
{
    for (unsigned int aFileN = 0; aFileN < _filesData.size(); ++aFileN)
    {
        ImgData& aID = _filesData[aFileN];
        if (aID._loadFail) 
            return false;
    }
    return true;
}

void PanoDetector::prepareMatches()
{
	int aLen = _filesData.size();
	if (_linearMatch)
		aLen = _linearMatchLen;

	if (aLen >= _filesData.size())
		aLen = _filesData.size() - 1;

	for (unsigned int i1 = 0; i1 < _filesData.size(); ++i1)
	{
		int aEnd = i1 + 1 + aLen;
		if (_filesData.size() < aEnd)
			aEnd = _filesData.size();

		for (unsigned int i2 = (i1+1); i2 < aEnd; ++i2) 
		{
			// create a new entry in the matches map
			_matchesData.push_back(MatchData());

			MatchData& aM = _matchesData.back();
			aM._i1 = &(_filesData[i1]);
			aM._i2 = &(_filesData[i2]);
		}
	}
}

