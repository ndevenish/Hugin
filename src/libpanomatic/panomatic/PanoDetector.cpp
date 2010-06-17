#include <hugin_config.h>
#include <hugin_version.h>

#include "PanoDetector.h"
#include <iostream>
#include <fstream>
#include <boost/foreach.hpp>

#include <time.h>

#include "zthread/Runnable.h"
#include "zthread/PoolExecutor.h"
#include "Utils.h"
#include "Tracer.h"


#include <hugin_basic.h>
#include <hugin_utils/platform.h>

#ifndef srandom
#define srandom srand
#endif	

using namespace std;
using namespace ZThread;
using namespace HuginBase;
using namespace hugin_utils;


PanoDetector::PanoDetector() :	_loadKeypoints(false), _outputFile("default0.oto"),
	_loadProject(false),
	_gradDescriptor(false),
	_sieve1Width(10), _sieve1Height(10), _sieve1Size(10), 
	_kdTreeSearchSteps(40), _kdTreeSecondDistance(0.15), _sieve2Width(5), _sieve2Height(5),
	_sieve2Size(1), _test(false), _cores(utils::getCPUCount()), _ransacIters(1000), _ransacDistanceThres(25),
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
		if (_files.size() != 2)
		{
			std::cout << "In test mode you must provide exactly 2 images." << std::endl;
			return false;
		}
	}

	return true;
}


void PanoDetector::printDetails()
{
	//cout << "\tNumber of keys    : " << _numKeys << endl;
	cout << "Output file       : " << _outputFile << endl;
	cout << "Number of CPU     : " << _cores << endl << endl;
	if (_loadKeypoints) {
		cout << "loading keypoints from file";
	} else {
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
	}
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

	if(!_loadProject && !_loadKeypoints)
	{
		cout << "Input Images :" << endl;
		BOOST_FOREACH(string& aF, _files)
			cout << "  - " << aF << endl;
	}
	if (_loadProject)
		cout << "Input Project File : " << _inputProjectFile << endl;

}

// definition of a runnable class for ImgData
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

// definition of a runnable class for loadproject
class LoadProjectDataRunnable : public Runnable
{
public:
	LoadProjectDataRunnable(PanoDetector::ImgData& iImageData, const PanoDetector& iPanoDetector) :
	  _imgData(iImageData), _panoDetector(iPanoDetector) {};

	  void run() 
	  {
		  if (!PanoDetector::AnalyzeImage(_imgData, _panoDetector)) return;
				// TODO: implement another function making use of SrcPanoImage methods
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

// definition of a runnable class for loadkeys
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

	// 1. prepare images
	// if a pto file was given as input, we add its images to _files.
	if(_loadProject)
	{
		if(!loadProject()) return;
	} else {
		prepareImages();
	}

	// 2. run analysis of images
	TRACE_INFO(endl<< "--- Analyze Images ---" << endl);
	try 
	{
		for (ImgDataIt_t aB = _filesData.begin(); aB != _filesData.end(); ++aB)
			if (_loadKeypoints) {
				aExecutor.execute(new LoadKeypointsDataRunnable(aB->second, *this));
			} else if (_loadProject) {
				aExecutor.execute(new LoadProjectDataRunnable(aB->second, *this));
			} else {
				aExecutor.execute(new ImgDataRunnable(aB->second, *this));
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
	
	// 5. write output
	TRACE_INFO(endl<< "--- Write output ---" << endl);
	writeOutput();

}

void PanoDetector::prepareImages()
{	
	for (unsigned int aFileN = 0; aFileN < _files.size(); ++aFileN)
	{
		// insert the image in the map
		_filesData.insert(make_pair(_files[aFileN], ImgData()));
		
		// get the data
		ImgData& aImgData = _filesData[_files[aFileN]];

		// set the name
		aImgData._name = _files[aFileN];
	}	
}

bool PanoDetector::loadProject()
{
   	ifstream ptoFile(_inputProjectFile.c_str());
    if (ptoFile.bad()) { 
        cerr << "ERROR: could not open file: '" << _inputProjectFile << "'!" << endl; 
        return false; 
    } 
		_panoramaInfo->setFilePrefix(hugin_utils::getPathPrefix(_inputProjectFile));
		AppBase::DocumentData::ReadWriteError err = _panoramaInfo->readData(ptoFile);
		if (err != AppBase::DocumentData::SUCCESSFUL) {
			  cerr << "ERROR: couldn't parse panos tool script: '" << _inputProjectFile << "'!" << endl;
			  return false;
		}

		// Add images found in the project file to _files
		int nImg = _panoramaInfo->getNrOfImages();
		for (unsigned int imgNr = 0; imgNr < nImg; ++imgNr)
		{
			// get image info
			SrcPanoImage img_info = _panoramaInfo->getImage(imgNr);

			// insert the image in the map
			_filesData.insert(make_pair(img_info.getFilename(), ImgData()));

			// get the data
			ImgData& aImgData = _filesData[img_info.getFilename()];

			// set the name
			aImgData._name = img_info.getFilename();

			// Number pointing to image info in _panoramaInfo
			aImgData._number = imgNr;
		}

		return true;
}

bool PanoDetector::checkLoadSuccess()
{
    for (unsigned int aFileN = 0; aFileN < _files.size(); ++aFileN)
    {
        ImgData& aID = _filesData[_files[aFileN]];
        if (aID._loadFail) 
            return false;
    }
    return true;
}


void PanoDetector::prepareMatches()
{
	int aLen = _files.size();
	if (_linearMatch)
		aLen = _linearMatchLen;

	if (aLen >= _files.size())
		aLen = _files.size() - 1;
	
	for (unsigned int i1 = 0; i1 < _files.size(); ++i1)
	{
		int aEnd = i1 + 1 + aLen;
		if (_files.size() < aEnd)
			aEnd = _files.size();

		for (unsigned int i2 = (i1+1); i2 < aEnd; ++i2) 
		{
			// create a new entry in the matches map
			_matchesData.push_back(MatchData());

			MatchData& aM = _matchesData.back();
			aM._i1_name = _files[i1];
			aM._i2_name = _files[i2];
			aM._i1 = &(_filesData[aM._i1_name]);
			aM._i2 = &(_filesData[aM._i2_name]);
		}
	}
}

