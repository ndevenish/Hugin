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

#ifdef _WINDOWS
#include <direct.h>
#else
#include <unistd.h>
#endif

#ifndef srandom
#define srandom srand
#endif	

using namespace std;
using namespace ZThread;
using namespace HuginBase;
using namespace AppBase;
using namespace HuginBase::Nona;
using namespace hugin_utils;

std::string includeTrailingPathSep(std::string path)
{
    std::string pathWithSep(path);
#ifdef _WINDOWS
    if(pathWithSep[pathWithSep.length()-1]!='\\' || pathWithSep[pathWithSep.length()-1]!='/')
        pathWithSep.append("\\");
#else
    if(pathWithSep[pathWithSep.length()-1]!='/')
        pathWithSep.append("/");
#endif
    return pathWithSep;
};

std::string getKeyfilenameFor(std::string keyfilesPath, std::string filename)
{
    std::string newfilename;
    if(keyfilesPath.empty())
    {
        //if no path for keyfiles is given we are saving into the same directory as image file
        newfilename=stripExtension(filename);
    }
    else
    {
        newfilename=includeTrailingPathSep(keyfilesPath);
        newfilename.append(stripPath(stripExtension(filename)));
    };
    newfilename.append(".key");
    return newfilename;
};

PanoDetector::PanoDetector() :	_outputFile("default.pto"),
	_writeAllKeyPoints(false),
	_sieve1Width(10), _sieve1Height(10), _sieve1Size(30), 
	_kdTreeSearchSteps(40), _kdTreeSecondDistance(0.15), _sieve2Width(5), _sieve2Height(5),
	_sieve2Size(2), _test(false), _cores(utils::getCPUCount()), _ransacIters(1000), _ransacDistanceThres(25),
    _minimumMatches(4), _linearMatch(false), _linearMatchLen(1), _downscale(true), _cache(false), _cleanup(false),
    _keypath("")
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
    if (_keyPointsIdx.size() != 0)
    {
        cout << "Output file(s)      :  keyfile(s) for images";
        for (unsigned int i = 0; i < _keyPointsIdx.size(); ++i)
        cout << " " << _keyPointsIdx[i] << endl;
    }
    else
    {
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
		  if (!PanoDetector::AnalyzeImage(_imgData, _panoDetector)) return;
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
		  if (!PanoDetector::AnalyzeImage(_imgData, _panoDetector)) return;
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
		  PanoDetector::RemapBackMatches(_matchData, _panoDetector);
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
    if(!loadProject())
    {
        return;
    };
    if(_writeAllKeyPoints)
    {
        for(unsigned int i=0;i<_panoramaInfo->getNrOfImages();i++)
        {
            _keyPointsIdx.push_back(i);
        };
    };

    if(_cleanup)
    {
        CleanupKeyfiles();
        return;
    };

    // 2. run analysis of images or keypoints
    try 
    {
        if (_keyPointsIdx.size() != 0)
        {
            TRACE_INFO(endl<< "--- Analyze Images ---" << endl);
            for (unsigned int i = 0; i < _keyPointsIdx.size(); ++i)
            {
                aExecutor.execute(new WriteKeyPointsRunnable(_filesData[_keyPointsIdx[i]], *this));
            };
        }
        else
        {
            TRACE_INFO(endl<< "--- Analyze Images ---" << endl);
            for (ImgDataIt_t aB = _filesData.begin(); aB != _filesData.end(); ++aB)
            {
                if (aB->second._hasakeyfile)
                {
                    aExecutor.execute(new LoadKeypointsDataRunnable(aB->second, *this));
                }
                else
                {
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
  
    if(_cache)
    {
        TRACE_INFO(endl << "--- Cache keyfiles to disc ---" << endl);
        for (ImgDataIt_t aB = _filesData.begin(); aB != _filesData.end(); ++aB)
        {
            if (!aB->second._hasakeyfile)
            {
                TRACE_INFO("i" << aB->second._number << " : Caching keypoints..." << endl);
                writeKeyfile(aB->second);
            };
        };
    };

    // Detect matches if writeKeyPoints wasn't set  
    if(_keyPointsIdx.size() == 0)
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

		// Add detected matches to _panoramaInfo
		BOOST_FOREACH(MatchData& aM, _matchesData)
			BOOST_FOREACH(lfeat::PointMatchPtr& aPM, aM._matches)
				_panoramaInfo->addCtrlPoint(ControlPoint(aM._i1->_number, aPM->_img1_x, aPM->_img1_y,
										 							  aM._i2->_number, aPM->_img2_x, aPM->_img2_y));
	}
	
	// 5. write output
    if (_keyPointsIdx.size() != 0)
    {
        //Write all keyfiles
        TRACE_INFO(endl<< "--- Write Keyfiles output ---" << endl << endl);
        for (unsigned int i = 0; i < _keyPointsIdx.size(); ++i)
        {
            writeKeyfile(_filesData[_keyPointsIdx[i]]);
        };
    }
    else
    {
        /// Write output project
        TRACE_INFO(endl<< "--- Write Project output ---" << endl << endl);
        writeOutput();
    };
}

bool PanoDetector::loadProject()
{
   ifstream ptoFile(_inputFile.c_str());
   if (ptoFile.bad()) { 
       cerr << "ERROR: could not open file: '" << _inputFile << "'!" << endl; 
       return false; 
   } 
    std::string prefix=hugin_utils::getPathPrefix(_inputFile);
    if(prefix.empty())
    {
        // Get the current working directory: 
        char* buffer;
#ifdef _WINDOWS
#define getcwd _getcwd
#endif
        if((buffer=getcwd(NULL,0))!=NULL)
        {
            prefix.append(buffer);
            free(buffer);
            prefix=includeTrailingPathSep(prefix);
       }
    };
	_panoramaInfo->setFilePrefix(prefix);
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
		// insert the image in the map
		_filesData.insert(make_pair(imgNr, ImgData()));
		
		// get the data
		ImgData& aImgData = _filesData[imgNr];

		// get a copy of image info
		SrcPanoImage img = _panoramaInfoCopy.getSrcImage(imgNr);

		// set the name
		aImgData._name = img.getFilename();

		// modify image position in the copy
		img.setYaw(0);
		img.setRoll(0);
		img.setPitch(0);
		img.setX(0);
		img.setY(0);
		img.setZ(0);
		img.setActive(true);
		_panoramaInfoCopy.setImage(imgNr,img);

		// Number pointing to image info in _panoramaInfo
		aImgData._number = imgNr;

        aImgData._needsremap=(img.getHFOV()>=65);
		// set image detection size
         if(aImgData._needsremap)
        {
            _filesData[imgNr]._detectWidth = max(img.getSize().width(),img.getSize().height());
            _filesData[imgNr]._detectHeight = max(img.getSize().width(),img.getSize().height());
        }
        else
        {
            _filesData[imgNr]._detectWidth = img.getSize().width();
            _filesData[imgNr]._detectHeight = img.getSize().height();
        };

        if (_downscale)
        {
           _filesData[imgNr]._detectWidth >>= 1;
           _filesData[imgNr]._detectHeight >>= 1;
        }

        // set image remapping options
        if(aImgData._needsremap)
        {
            aImgData._projOpts = _panoramaInfoCopy.getOptions();
            aImgData._projOpts.setHFOV(img.getHFOV());
            aImgData._projOpts.setWidth(_filesData[imgNr]._detectWidth);
            aImgData._projOpts.setHeight(_filesData[imgNr]._detectHeight);
            aImgData._projOpts.setProjection(PanoramaOptions::STEREOGRAPHIC);
        }

        // Specify if the image has an associated keypoint file

        aImgData._keyfilename = getKeyfilenameFor(_keypath,aImgData._name);
        ifstream keyfile(aImgData._keyfilename.c_str());
        aImgData._hasakeyfile = keyfile.good();
    }

    return true;
}

bool PanoDetector::checkLoadSuccess()
{
    if(_keyPointsIdx.size()!=0)
    {
        for (unsigned int i = 0; i < _keyPointsIdx.size(); ++i)
        {
            if(_filesData[_keyPointsIdx[i]]._loadFail)
            {
                return false;
            };
        };
    }
    else
    {
        for (unsigned int aFileN = 0; aFileN < _filesData.size(); ++aFileN)
        {
            if(_filesData[aFileN]._loadFail)
            {
                return false;
            };
        };
    };
    return true;
}

void PanoDetector::CleanupKeyfiles()
{
    for (ImgDataIt_t aB = _filesData.begin(); aB != _filesData.end(); ++aB)
    {
        if (aB->second._hasakeyfile)
        {
            remove(aB->second._keyfilename.c_str());
        };
    };
};

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

