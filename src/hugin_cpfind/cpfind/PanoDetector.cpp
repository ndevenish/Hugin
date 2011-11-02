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

//for multi row strategy
#include <panodata/StandardImageVariableGroups.h>
#include <PT/Panorama.h>
#include <PT/ImageGraph.h>
#include <algorithms/optimizer/PTOptimizer.h>
#include <algorithms/basic/CalculateOverlap.h>

#include "ImageImport.h"

#ifdef _WINDOWS
#include <windows.h>
#include <direct.h>
#else
#include <unistd.h>
#endif
#ifdef __APPLE__
#include <hugin_config.h>
#include <mach-o/dyld.h>	/* _NSGetExecutablePath */
#include <limits.h>		/* PATH_MAX */
#include <libgen.h>		/* dirname */
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


#define TRACE_IMG(X) {if (_panoDetector.getVerbose() == 1) {TRACE_INFO("i" << _imgData._number << " : " << X << endl);}}
#define TRACE_PAIR(X) {if (_panoDetector.getVerbose() == 1){ TRACE_INFO("i" << _matchData._i1->_number << " <> " \
																		 "i" << _matchData._i2->_number << " : " << X << endl);}}

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

PanoDetector::PanoDetector() :	
	_writeAllKeyPoints(false), _verbose(1),
	_sieve1Width(10), _sieve1Height(10), _sieve1Size(100), 
	_kdTreeSearchSteps(200), _kdTreeSecondDistance(0.25), _ransacIters(1000), _ransacDistanceThres(50),
	_sieve2Width(5), _sieve2Height(5),_sieve2Size(1), _test(false), _cores(utils::getCPUCount()),
	_minimumMatches(6), _linearMatch(false), _linearMatchLen(1), _downscale(true), _cache(false), _cleanup(false),
    _keypath(""), _outputFile("default.pto"), _outputGiven(false), _celeste(false), _celesteThreshold(0.5), _celesteRadius(20), _multirow(false)
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

    if(_linearMatch && _multirow)
    {
        std::cout << "Linear match and multi row matching strategy does not work together." << std::endl;
        return false;
    };

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
    if(_celeste)
    {
        cout << "Celeste options" << endl;
        cout << "  Threshold : " << _celesteThreshold << endl;
        cout << "  Radius : " << _celesteRadius << endl;
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
    if(_linearMatch)
    {
        cout << "  Mode : Linear match with length of " << _linearMatch << " image" << endl;
    }
    else
    {
        if(_multirow)
        {
            cout << "  Mode : Multi row" << endl;
        }
        else
        {
            cout << "  Mode : All pairs" << endl;
        };
    };
	cout << "  Distance threshold : " << _ransacDistanceThres << endl;
	cout << "RANSAC Options" << endl;
	cout << "  Mode : ";
	switch (_ransacMode) {
	case RANSACOptimizer::AUTO:
		cout << "auto" << endl;
		break;
	case RANSACOptimizer::HOMOGRAPHY:
		cout << "homography" << endl;
		break;
	case RANSACOptimizer::RPY:
		cout << "roll, pitch, yaw" << endl;
		break;
	case RANSACOptimizer::RPYV:
		cout << "roll, pitch, yaw, fov" << endl;
		break;
	case RANSACOptimizer::RPYVB:
		cout << "roll, pitch, yaw, fov, distortion" << endl;
		break;
	}
	cout << "  Iterations : " << _ransacIters << endl;
	cout << "  Distance threshold : " << _ransacDistanceThres << endl;
	cout << "Sieve 2 Options" << endl;
	cout << "  Width : " << _sieve2Width << endl;
	cout << "  Height : " << _sieve2Height << endl;
	cout << "  Size : " << _sieve2Size << endl;
	cout << "  ==> Maximum matches per image pair : " << _sieve2Size * _sieve2Height * _sieve2Width << endl;
}

void PanoDetector::printFilenames()
{
    cout << endl << "Project contains the following images:" << endl;
    for(unsigned int i=0;i<_panoramaInfo->getNrOfImages();i++)
    {
        std::string name(_panoramaInfo->getImage(i).getFilename());
        if(name.compare(0,_prefix.length(),_prefix)==0)
            name=name.substr(_prefix.length(),name.length()-_prefix.length());
        cout << "Image " << i << endl << "  Imagefile: " << name << endl;
        bool writeKeyfileForImage=false;
        if(_keyPointsIdx.size()>0)
        {
            for(unsigned j=0;j<_keyPointsIdx.size() && !writeKeyfileForImage;j++)
            {
                writeKeyfileForImage=_keyPointsIdx[j]==i;
            };
        };
        if(_cache || _filesData[i]._hasakeyfile || writeKeyfileForImage)
        {
            name=_filesData[i]._keyfilename;
            if(name.compare(0,_prefix.length(),_prefix)==0)
                name=name.substr(_prefix.length(),name.length()-_prefix.length());
            cout << "  Keyfile  : " << name;
            if(writeKeyfileForImage)
            {
                cout << " (will be generated)" << endl;
            }
            else
            {
                cout << (_filesData[i]._hasakeyfile?" (will be loaded)":" (will be generated)") << endl;
            };
        };
        cout << "  Remapped : " << (_filesData[i]._needsremap?"yes":"no") << endl;
    };
};

// definition of a runnable class for image data
class ImgDataRunnable : public Runnable
{
public:
	ImgDataRunnable(PanoDetector::ImgData& iImageData, const PanoDetector& iPanoDetector) :
	  _imgData(iImageData), _panoDetector(iPanoDetector) {};

	  void run() 
	  {	
		  TRACE_IMG("Analyzing image...");
		  if (!PanoDetector::AnalyzeImage(_imgData, _panoDetector)) return;
		  PanoDetector::FindKeyPointsInImage(_imgData, _panoDetector);
		  PanoDetector::FilterKeyPointsInImage(_imgData, _panoDetector);
		  PanoDetector::MakeKeyPointDescriptorsInImage(_imgData, _panoDetector);
		  PanoDetector::RemapBackKeypoints(_imgData, _panoDetector);
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
		  TRACE_IMG("Analyzing image...");
		  if (!PanoDetector::AnalyzeImage(_imgData, _panoDetector)) return;
		  PanoDetector::FindKeyPointsInImage(_imgData, _panoDetector);
		  PanoDetector::FilterKeyPointsInImage(_imgData, _panoDetector);
		  PanoDetector::MakeKeyPointDescriptorsInImage(_imgData, _panoDetector);
		  PanoDetector::RemapBackKeypoints(_imgData, _panoDetector);
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
		TRACE_IMG("Loading keypoints...");
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
		  //TRACE_PAIR("Matching...");
		  PanoDetector::FindMatchesInPair(_matchData, _panoDetector);
		  PanoDetector::RansacMatchesInPair(_matchData, _panoDetector);
		  PanoDetector::FilterMatchesInPair(_matchData, _panoDetector);		  
		  TRACE_PAIR("Found " << _matchData._matches.size() << " matches");
	  }
private:
	const PanoDetector&			_panoDetector;
	PanoDetector::MatchData&	_matchData;
};

bool PanoDetector::LoadSVMModel()
{
    string model_file = ("celeste.model");
    ifstream test(model_file.c_str());
    if (!test.good())
    {
#if _WINDOWS
        char buffer[MAX_PATH];//always use MAX_PATH for filepaths
        GetModuleFileNameA(NULL,buffer,sizeof(buffer));
        string working_path=(buffer);
        string install_path_model="";
        //remove filename
        std::string::size_type pos=working_path.rfind("\\");
        if(pos!=std::string::npos)
        {
            working_path.erase(pos);
            //remove last dir: should be bin
            pos=working_path.rfind("\\");
            if(pos!=std::string::npos)
            {
                working_path.erase(pos);
                //append path delimiter and path
                working_path.append("\\share\\hugin\\data\\");
                install_path_model=working_path;
            }
        }
#elif defined MAC_SELF_CONTAINED_BUNDLE
        //string install_path_model = ("./xrc/");
		char path[PATH_MAX + 1];
		uint32_t size = sizeof(path);
		string install_path_model("");
		if (_NSGetExecutablePath(path, &size) == 0)
		{
			//install_path_model=path;
			install_path_model=dirname(path);
			install_path_model.append("/xrc/");
			cout << "Detected path " << install_path_model << endl << endl;
		}
#else
        string install_path_model = (INSTALL_DATA_DIR);
#endif
		install_path_model.append(model_file);
        ifstream test2(install_path_model.c_str());
		if (!test2.good())
        {
            cout << endl << "Couldn't open SVM model file " << model_file << endl;
            cout << "Also tried " << install_path_model << endl << endl; 
            return false;
        };
        model_file = install_path_model;
  	}
    if(!celeste::loadSVMmodel(svmModel,model_file))
    {
        svmModel=NULL;
        return false;
    };
    return true;
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

    svmModel=NULL;
    if(_celeste)
    {
        TRACE_INFO("\nLoading Celeste model file...\n");
        if(!LoadSVMModel())
        {
            setCeleste(false);
        };
    };

    //print some more information about the images
	if (_verbose > 0) {
		printFilenames();
	}

    // 2. run analysis of images or keypoints
#if _WINDOWS
    //multi threading of image loading results sometime in a race condition
    //try to prevent this by initialisation of codecManager before
    //running multi threading part
    std::string s=vigra::impexListExtensions();
#endif
    try 
    {
        if (_keyPointsIdx.size() != 0)
        {
			if (_verbose > 0)
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
        if(svmModel!=NULL)
        {
            celeste::destroySVMmodel(svmModel);
        };
		return;
	}

    if(svmModel!=NULL)
    {
        celeste::destroySVMmodel(svmModel);
    };

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
        if(_multirow)
        {
            if(!matchMultiRow(aExecutor))
            {
                return;
            };
        }
        else
        {
            if(!match(aExecutor))
            {
                return;
            };
        };
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
        if(_outputGiven)
        {
            cout << endl << "Warning: You have given the --output switch." << endl
                         << "This switch is not compatible with the --writekeyfile or --kall switch." << endl
                         << "If you want to generate the keyfiles and" << endl
                         << "do the matching in the same run use the --cache switch instead." << endl << endl;
        };
    }
    else
    {
        /// Write output project
        TRACE_INFO(endl<< "--- Write Project output ---" << endl);
        writeOutput();
        TRACE_INFO("Written output to " << _outputFile << endl << endl);
    };
}

bool PanoDetector::match(PoolExecutor& aExecutor)
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
        return false;
    }

    // Add detected matches to _panoramaInfo
    BOOST_FOREACH(MatchData& aM, _matchesData)
        BOOST_FOREACH(lfeat::PointMatchPtr& aPM, aM._matches)
            _panoramaInfo->addCtrlPoint(ControlPoint(aM._i1->_number, aPM->_img1_x, aPM->_img1_y,
                                                     aM._i2->_number, aPM->_img2_x, aPM->_img2_y));
    return true;
};

bool PanoDetector::loadProject()
{
   ifstream ptoFile(_inputFile.c_str());
   if (ptoFile.bad()) { 
       cerr << "ERROR: could not open file: '" << _inputFile << "'!" << endl; 
       return false; 
   } 
    _prefix=hugin_utils::getPathPrefix(_inputFile);
    if(_prefix.empty())
    {
        // Get the current working directory: 
        char* buffer;
#ifdef _WINDOWS
#define getcwd _getcwd
#endif
        if((buffer=getcwd(NULL,0))!=NULL)
        {
            _prefix.append(buffer);
            free(buffer);
            _prefix=includeTrailingPathSep(_prefix);
       }
    };
	_panoramaInfo->setFilePrefix(_prefix);
	AppBase::DocumentData::ReadWriteError err = _panoramaInfo->readData(ptoFile);
	if (err != AppBase::DocumentData::SUCCESSFUL) {
		  cerr << "ERROR: couldn't parse panos tool script: '" << _inputFile << "'!" << endl;
		  return false;
	}

	// Create a copy of panoramaInfo that will be used to define
	// image options
	_panoramaInfoCopy=_panoramaInfo->duplicate();

	// Add images found in the project file to _filesData
	unsigned int nImg = _panoramaInfo->getNrOfImages();
    unsigned int imgWithKeyfile=0;
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
		img.setResponseType(SrcPanoImage::RESPONSE_LINEAR);
		img.setExposureValue(0);
		_panoramaInfoCopy.setImage(imgNr,img);

		// Number pointing to image info in _panoramaInfo
		aImgData._number = imgNr;

        aImgData._needsremap=(img.getHFOV()>=65 && img.getProjection() != SrcPanoImage::FISHEYE_STEREOGRAPHIC);
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
			aImgData._projOpts.setProjection(PanoramaOptions::STEREOGRAPHIC);
			aImgData._projOpts.setHFOV(250);
			aImgData._projOpts.setVFOV(250);
			aImgData._projOpts.setWidth(250);
			aImgData._projOpts.setHeight(250);

			// determine size of output image.
			// The old code did not work with images with images with a FOV
			// approaching 180 degrees
            vigra::Rect2D roi=estimateOutputROI(_panoramaInfoCopy,aImgData._projOpts,imgNr);
			double scalefactor = max((double)_filesData[imgNr]._detectWidth / roi.width(),
									 (double)_filesData[imgNr]._detectHeight / roi.height() );
			
			// resize output canvas
			vigra::Size2D canvasSize((int)aImgData._projOpts.getWidth() * scalefactor,
									 (int)aImgData._projOpts.getHeight() * scalefactor);
			aImgData._projOpts.setWidth(canvasSize.width(), false);
			aImgData._projOpts.setHeight(canvasSize.height());

			// set roi to cover the remapped input image
			roi = roi * scalefactor;
            _filesData[imgNr]._detectWidth = roi.width();
			_filesData[imgNr]._detectHeight = roi.height();
			aImgData._projOpts.setROI(roi);
        }

        // Specify if the image has an associated keypoint file

        aImgData._keyfilename = getKeyfilenameFor(_keypath,aImgData._name);
        ifstream keyfile(aImgData._keyfilename.c_str());
        aImgData._hasakeyfile = keyfile.good();
        if(aImgData._hasakeyfile)
        {
            imgWithKeyfile++;
        };
    }
    //update masks, convert positive masks into negative masks
    //because positive masks works only if the images are on the final positions
    _panoramaInfoCopy.updateMasks(true);

    //if all images has keyfile, we don't need to load celeste model file
    if(nImg==imgWithKeyfile)
    {
        _celeste=false;
    };
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
	unsigned int aLen = _filesData.size();
	if (_linearMatch)
		aLen = _linearMatchLen;

	if (aLen >= _filesData.size())
		aLen = _filesData.size() - 1;

	for (unsigned int i1 = 0; i1 < _filesData.size(); ++i1)
	{
		unsigned int aEnd = i1 + 1 + aLen;
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

struct img_ev
{
    unsigned int img_nr;
    double ev;
};
struct stack_img
{
    unsigned int layer_nr;
    std::vector<img_ev> images;
};
bool sort_img_ev (img_ev i1, img_ev i2) { return (i1.ev<i2.ev); };

bool PanoDetector::matchMultiRow(PoolExecutor& aExecutor)
{
    //step 1
    std::vector<stack_img> stack_images;
    HuginBase::StandardImageVariableGroups* variable_groups = new HuginBase::StandardImageVariableGroups(*_panoramaInfo);
    for(unsigned int i=0; i<_panoramaInfo->getNrOfImages(); i++)
    {
        unsigned int stack_nr=variable_groups->getStacks().getPartNumber(i);
        //check, if this stack is already in list
        bool found=false;
        unsigned int index=0;
        for(index=0;index<stack_images.size();index++)
        {
            found=(stack_images[index].layer_nr==stack_nr);
            if(found)
            {
                break;
            };
        };
        if(!found)
        {
            //new stack
            stack_images.resize(stack_images.size()+1);
            index=stack_images.size()-1;
            //add new stack
            stack_images[index].layer_nr=stack_nr;
        };
        //add new image
        unsigned int new_image_index=stack_images[index].images.size();
        stack_images[index].images.resize(new_image_index+1);
        stack_images[index].images[new_image_index].img_nr=i;
        stack_images[index].images[new_image_index].ev=_panoramaInfo->getImage(i).getExposure();
    };
    delete variable_groups;
    //get image with median exposure for search with cp generator
    vector<unsigned int> images_layer;
    UIntSet images_layer_set;
    for(unsigned int i=0;i<stack_images.size();i++)
    {
        std::sort(stack_images[i].images.begin(),stack_images[i].images.end(),sort_img_ev);
        unsigned int index=0;
        if(stack_images[i].images[0].ev!=stack_images[i].images[stack_images[i].images.size()-1].ev)
        {
            index=stack_images[i].images.size() / 2;
        };
        images_layer.push_back(stack_images[i].images[index].img_nr);
        images_layer_set.insert(stack_images[i].images[index].img_nr);
        if(stack_images[i].images.size()>1)
        {
            //build match list for stacks
            for(unsigned int j=0;j<stack_images[i].images.size()-1;j++)
            {
                _matchesData.push_back(MatchData());
                MatchData& aM=_matchesData.back();
                aM._i1=&(_filesData[stack_images[i].images[j].img_nr]);
                aM._i2=&(_filesData[stack_images[i].images[j+1].img_nr]);
            };
        };
    };
    //build match data list for image pairs
    if(images_layer.size()>1)
    {
        for(unsigned int i=0;i<images_layer.size()-1;i++)
        {
            _matchesData.push_back(MatchData());
            MatchData& aM = _matchesData.back();
            aM._i1 = &(_filesData[images_layer[i]]);
            aM._i2 = &(_filesData[images_layer[i+1]]);
        };
    };
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
        return false;
    }

    // Add detected matches to _panoramaInfo
    BOOST_FOREACH(MatchData& aM, _matchesData)
        BOOST_FOREACH(lfeat::PointMatchPtr& aPM, aM._matches)
            _panoramaInfo->addCtrlPoint(ControlPoint(aM._i1->_number, aPM->_img1_x, aPM->_img1_y,
                                                     aM._i2->_number, aPM->_img2_x, aPM->_img2_y));
    
    // step 2: connect all image groups
    _matchesData.clear();
    CPGraph graph;
    createCPGraph(*_panoramaInfo, graph);
    CPComponents comps;
    unsigned int n = findCPComponents(graph, comps);
    if(n>1)
    {
        vector<unsigned int> ImagesGroups;
        for(unsigned int i=0;i<n;i++)
        {
            ImagesGroups.push_back(*(comps[i].begin()));
            if(comps[i].size()>1)
                ImagesGroups.push_back(*(comps[i].rbegin()));
        };
        for(unsigned int i=0;i<ImagesGroups.size()-1;i++)
        {
            for(unsigned int j=i+1;j<ImagesGroups.size();j++)
            {
                _matchesData.push_back(MatchData());
                MatchData& aM = _matchesData.back();
                aM._i1 = &(_filesData[ImagesGroups[i]]);
                aM._i2 = &(_filesData[ImagesGroups[j]]);
            };
        };
        TRACE_INFO(endl<< "--- Find matches in images groups ---" << endl);
        try 
        {
            BOOST_FOREACH(MatchData& aMD, _matchesData)
                aExecutor.execute(new MatchDataRunnable(aMD, *this));
            aExecutor.wait();
        } 
        catch(Synchronization_Exception& e)
        { 
            TRACE_ERROR(e.what() << endl);
            return false;
        }
        BOOST_FOREACH(MatchData& aM, _matchesData)
            BOOST_FOREACH(lfeat::PointMatchPtr& aPM, aM._matches)
                _panoramaInfo->addCtrlPoint(ControlPoint(aM._i1->_number, aPM->_img1_x, aPM->_img1_y,
                                                         aM._i2->_number, aPM->_img2_x, aPM->_img2_y));
    };
    // step 3: now connect all overlapping images
    _matchesData.clear();
    createCPGraph(*_panoramaInfo,graph);
    if(findCPComponents(graph, comps)==1 && images_layer.size()>2)
    {
        PT::Panorama optPano=_panoramaInfo->getSubset(images_layer_set);
        //next steps happens only when all images are connected;
        //now optimize panorama
        PanoramaOptions opts = _panoramaInfo->getOptions();
        opts.setProjection(PanoramaOptions::EQUIRECTANGULAR);
        opts.optimizeReferenceImage=0;
        // calculate proper scaling, 1:1 resolution.
        // Otherwise optimizer distances are meaningless.
        opts.setWidth(30000, false);
        opts.setHeight(15000);

        optPano.setOptions(opts);
        int w = optPano.calcOptimalWidth();
        opts.setWidth(w);
        opts.setHeight(w/2);
        optPano.setOptions(opts);

        //generate optimize vector, optimize only yaw and pitch
        OptimizeVector optvars;
        const SrcPanoImage & anchorImage = optPano.getImage(opts.optimizeReferenceImage);
        for (unsigned i=0; i < optPano.getNrOfImages(); i++) 
        {
            std::set<std::string> imgopt;
            if(i==opts.optimizeReferenceImage)
            {
                //optimize only anchors pitch, not yaw
                imgopt.insert("p");
            }
            else
            {
                imgopt.insert("p");
                imgopt.insert("y");
            };
            optvars.push_back(imgopt);
        }
        optPano.setOptimizeVector(optvars);

        // remove vertical and horizontal control points
        CPVector cps = optPano.getCtrlPoints();
        CPVector newCP;
        for (CPVector::const_iterator it = cps.begin(); it != cps.end(); it++) {
            if (it->mode == ControlPoint::X_Y)
            {
                newCP.push_back(*it);
            }
        }
        optPano.setCtrlPoints(newCP);

        if (getVerbose() < 2) 
        {
            PT_setProgressFcn(ptProgress);
            PT_setInfoDlgFcn(ptinfoDlg);
        };
        //in a first step do a pairwise optimisation
        HuginBase::AutoOptimise::autoOptimise(optPano, false);
        //now the final optimisation
        HuginBase::PTools::optimize(optPano);
        if (getVerbose() < 2) 
        {
            PT_setProgressFcn(NULL);
            PT_setInfoDlgFcn(NULL);
        };

        HuginBase::CalculateImageOverlap overlap(&optPano);
        overlap.calculate(10);
        for(unsigned int i=0;i<images_layer.size()-2;i++)
        {
            for(unsigned int j=i+2;j<images_layer.size();j++)
            {
                if(overlap.getOverlap(i,j)>0)
                {
                    _matchesData.push_back(MatchData());
                    MatchData& aM = _matchesData.back();
                    aM._i1 = &(_filesData[images_layer[i]]);
                    aM._i2 = &(_filesData[images_layer[j]]);
                };
            };
        };
        TRACE_INFO(endl<< "--- Find matches for overlapping images ---" << endl);
        try 
        {
            BOOST_FOREACH(MatchData& aMD, _matchesData)
                aExecutor.execute(new MatchDataRunnable(aMD, *this));
            aExecutor.wait();
        }
        catch(Synchronization_Exception& e)
        {
            TRACE_ERROR(e.what() << endl);
            return false;
        }

        // Add detected matches to _panoramaInfo
        BOOST_FOREACH(MatchData& aM, _matchesData)
            BOOST_FOREACH(lfeat::PointMatchPtr& aPM, aM._matches)
                _panoramaInfo->addCtrlPoint(ControlPoint(aM._i1->_number, aPM->_img1_x, aPM->_img1_y,
                                                         aM._i2->_number, aPM->_img2_x, aPM->_img2_y));
    };
    return true;
};

