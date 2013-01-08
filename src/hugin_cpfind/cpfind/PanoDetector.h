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

#ifndef __detectpano_panodetector_h
#define __detectpano_panodetector_h

#include <hugin_config.h>
#include <hugin_version.h>

#include "PanoDetectorDefs.h"
#include <string>
#include <map>
#include <localfeatures/Image.h>
#include <localfeatures/PointMatch.h>
#include "TestCode.h"
#include <zthread/Runnable.h>
#include <zthread/PoolExecutor.h>

#include <localfeatures/KeyPoint.h>
#include <localfeatures/KeyPointDetector.h>

#include <flann/flann.hpp>

#include <vigra_ext/ROIImage.h>

#include <panodata/Panorama.h>
#include <algorithms/optimizer/PTOptimizer.h>
#include <celeste/Celeste.h>

using namespace HuginBase;

class PanoDetector
{
public:
    typedef std::vector<std::string>						FileNameList_t;
    typedef std::vector<std::string>::iterator				FileNameListIt_t;
    typedef KDTreeSpace::KDTree<KDElemKeyPoint, double>		KPKDTree;
    typedef boost::shared_ptr<KPKDTree >					KPKDTreePtr;

    typedef lfeat::KeyPointDetector KeyPointDetector;

    /** for selecting matching strategy */
    enum MatchingStrategy
    {
        ALLPAIRS=0,
        LINEAR,
        MULTIROW,
        PREALIGNED
    };

    PanoDetector();

    bool checkData();
    void printDetails();
    void printFilenames();
    void printHelp();
    void run();
    bool match(ZThread::PoolExecutor& aExecutor, std::vector<HuginBase::UIntSet> &checkedPairs);
    bool matchMultiRow(ZThread::PoolExecutor& aExecutor);
    /** does only matches image pairs which overlaps and don't have control points
        @param aExecutor executor for threading
        @param pano pano, which should be used for determing of overlap, can contain also less images than _panoramaInfo
        @param connectedImages contains a list of already connected or tested image pairs, which should be skipped
        @param imgMap map of image nr in partial pano and full panorama 
        @param exactOverlap if true, only really overlapping image pairs are matched, if false it increases the hfov
                 to take also narrow overlaps better into account
        @return true, if detection was successful
    */
    bool matchPrealigned(ZThread::PoolExecutor& aExecutor, Panorama* pano, std::vector<HuginBase::UIntSet> &connectedImages, std::vector<size_t> imgMap, bool exactOverlap=true);


    // accessors

    inline Panorama* getPanoramaInfo() const
    {
        return _panoramaInfo;
    }

    inline void setKeyPointsIdx(std::vector<int> keyPointsIdx)
    {
        _keyPointsIdx = keyPointsIdx;
    }
    inline std::vector<int> getKeyPointsIdx() const
    {
        return _keyPointsIdx;
    }
    inline void setWriteAllKeyPoints(bool writeAllKeyPoints=true)
    {
        _writeAllKeyPoints = writeAllKeyPoints;
    }
    inline bool getWriteAllKeyPoints() const
    {
        return _writeAllKeyPoints;
    }

    inline void setVerbose(int level)
    {
        _verbose = level;
    }
    inline int  getVerbose() const
    {
        return _verbose;
    }

    inline void setSieve1Width(int iWidth)
    {
        _sieve1Width = iWidth;
    }
    inline void setSieve1Height(int iHeight)
    {
        _sieve1Height = iHeight;
    }
    inline void setSieve1Size(int iSize)
    {
        _sieve1Size = iSize;
    }
    inline int  getSieve1Width() const
    {
        return _sieve1Width;
    }
    inline int  getSieve1Height() const
    {
        return _sieve1Height;
    }
    inline int  getSieve1Size() const
    {
        return _sieve1Size;
    }

    inline void setKDTreeSearchSteps(int iSteps)
    {
        _kdTreeSearchSteps = iSteps;
    }
    inline void setKDTreeSecondDistance(double iDist)
    {
        _kdTreeSecondDistance = iDist;
    }
    inline int  getKDTreeSearchSteps() const
    {
        return _kdTreeSearchSteps;
    }
    inline double  getKDTreeSecondDistance() const
    {
        return _kdTreeSecondDistance;
    }

    inline void setMinimumMatches(int iMatches)
    {
        _minimumMatches = iMatches;
    }
    inline void setRansacIterations(int iIters)
    {
        _ransacIters = iIters;
    }
    inline void setRansacDistanceThreshold(int iDT)
    {
        _ransacDistanceThres = iDT;
    }
    inline void setRansacMode(RANSACOptimizer::Mode mode)
    {
        _ransacMode = mode;
    }
    inline int  getMinimumMatches() const
    {
        return _minimumMatches;
    }
    inline int  getRansacIterations() const
    {
        return _ransacIters;
    }
    inline int  getRansacDistanceThreshold() const
    {
        return _ransacDistanceThres;
    }
    inline RANSACOptimizer::Mode getRansacMode()
    {
        return _ransacMode;
    }

    inline void setSieve2Width(int iWidth)
    {
        _sieve2Width = iWidth;
    }
    inline void setSieve2Height(int iHeight)
    {
        _sieve2Height = iHeight;
    }
    inline void setSieve2Size(int iSize)
    {
        _sieve2Size = iSize;
    }
    inline int  getSieve2Width() const
    {
        return _sieve2Width;
    }
    inline int  getSieve2Height() const
    {
        return _sieve2Height;
    }
    inline int  getSieve2Size() const
    {
        return _sieve2Size;
    }

    inline void setLinearMatchLen(int iLen)
    {
        _linearMatchLen = iLen;
    }
    inline int  getLinearMatchLen() const
    {
        return _linearMatchLen;
    }
    inline void setMatchingStrategy(MatchingStrategy iMatchStrategy)
    {
        _matchingStrategy = iMatchStrategy;
    }
    inline MatchingStrategy getMatchingStrategy() const
    {
        return _matchingStrategy;
    }

    inline bool	getDownscale() const
    {
        return _downscale;
    }
    inline void setDownscale(bool iDown)
    {
        _downscale = iDown;
    }

    //	inline void setNumberOfKeys(int iNumKeys) { _numKeys = iNumKeys; }
    inline void setOutputFile(const std::string& outputFile)
    {
        _outputFile = outputFile;
        _outputGiven=true;
    }
    inline void setInputFile(const std::string& inputFile)
    {
        _inputFile = inputFile;
    }
    inline void setKeyfilesPath(const std::string& keypath)
    {
        _keypath = keypath;
    }
    inline bool getCached() const
    {
        return _cache;
    }
    inline void setCached(bool iCached)
    {
        _cache = iCached;
    }
    inline bool getCleanup() const
    {
        return _cleanup;
    }
    inline void setCleanup(bool iCleanup)
    {
        _cleanup = iCleanup;
    }
    inline bool getCeleste() const
    {
        return _celeste;
    };
    inline void setCeleste(bool iCeleste)
    {
        _celeste = iCeleste;
    };
    inline double getCelesteThreshold() const
    {
        return _celesteThreshold;
    };
    inline void setCelesteThreshold(double iCelesteThreshold)
    {
        _celesteThreshold = iCelesteThreshold;
    };
    inline int getCelesteRadius() const
    {
        return _celesteRadius;
    };
    inline void setCelesteRadius(int iCelesteRadius)
    {
        _celesteRadius = iCelesteRadius;
    };
    inline void setTest(bool iTest)
    {
        _test = iTest;
    }
    inline bool getTest() const
    {
        return _test;
    }
    inline void setCores(int iCores)
    {
        _cores = iCores;
    }

    // predeclaration
    struct ImgData;
    struct MatchData;

private:
    // options

    bool						_writeAllKeyPoints;
    std::vector<int>		_keyPointsIdx;

    int                     _verbose;

    int						_sieve1Width;
    int						_sieve1Height;
    int						_sieve1Size;

    int						_kdTreeSearchSteps;
    double					_kdTreeSecondDistance;

    int						_minimumMatches;
    RANSACOptimizer::Mode	_ransacMode;
    int						_ransacIters;
    int						_ransacDistanceThres;

    int						_sieve2Width;
    int						_sieve2Height;
    int						_sieve2Size;

    MatchingStrategy _matchingStrategy;
    int						_linearMatchLen;

    bool						_test;
    int						_cores;
    bool                 _downscale;
    bool        _cache;
    bool        _cleanup;
    bool        _celeste;
    double      _celesteThreshold;
    int         _celesteRadius;
    std::string _keypath;
    std::string _prefix;

    //	bool						_stereoRemap;

    // list of files
    std::string				_outputFile;
    bool _outputGiven;
    std::string				_inputFile;

    // Store panorama information
    Panorama*			_panoramaInfo;
    Panorama				_panoramaInfoCopy;

    bool					loadProject();
    bool	      		checkLoadSuccess();
    void CleanupKeyfiles();

    void					writeOutput();
    void					writeKeyfile(ImgData& imgInfo);

    // internals
public:
    struct ImgData
    {
        std::string			_name;

        int					_number;
        int					_detectWidth;
        int					_detectHeight;

        lfeat::Image		_ii;
        vigra::BImage		_distancemap;

        bool				_needsremap;
        PanoramaOptions 	_projOpts;

        bool 					_hasakeyfile;
        std::string _keyfilename;

        lfeat::KeyPointVect_t	_kp;
        int					_descLength;
        bool          	   _loadFail;

        // kdtree
        flann::Matrix<double> _flann_descriptors;
        flann::Index<flann::L2<double> > * _flann_index;

    };

    typedef std::map<int, ImgData>					ImgData_t;
    typedef std::map<int, ImgData>::iterator		ImgDataIt_t;

    struct MatchData
    {
        ImgData*				_i1;
        ImgData*				_i2;
        lfeat::PointMatchVector_t		_matches;
    };

    typedef std::vector<MatchData>								MatchData_t;
    typedef std::vector<MatchData>::iterator					MatchDataIt_t;

    // actions
    static bool				LoadKeypoints(ImgData& ioImgInfo, const PanoDetector& iPanoDetector);

    static bool				AnalyzeImage(ImgData& ioImgInfo, const PanoDetector& iPanoDetector);
    static bool				FindKeyPointsInImage(ImgData& ioImgInfo, const PanoDetector& iPanoDetector);
    static bool				FilterKeyPointsInImage(ImgData& ioImgInfo, const PanoDetector& iPanoDetector);
    static bool				MakeKeyPointDescriptorsInImage(ImgData& ioImgInfo, const PanoDetector& iPanoDetector);
    static bool             RemapBackKeypoints(ImgData& ioImgInfo, const PanoDetector& iPanoDetector);
    static bool				BuildKDTreesInImage(ImgData& ioImgInfo, const PanoDetector& iPanoDetector);
    static bool				FreeMemoryInImage(ImgData& ioImgInfo, const PanoDetector& iPanoDetector);

    static bool				FindMatchesInPair(MatchData& ioMatchData, const PanoDetector& iPanoDetector);
    static bool				RansacMatchesInPair(MatchData& ioMatchData, const PanoDetector& iPanoDetector);
    static bool				RansacMatchesInPairCam(MatchData& ioMatchData, const PanoDetector& iPanoDetector);
    static bool				RansacMatchesInPairHomography(MatchData& ioMatchData, const PanoDetector& iPanoDetector);
    static bool				FilterMatchesInPair(MatchData& ioMatchData, const PanoDetector& iPanoDetector);

private:
    bool LoadSVMModel();
    ImgData_t				_filesData;
    MatchData_t				_matchesData;
    struct celeste::svm_model* svmModel;
};

/** returns the filename for the keyfile for a given image */
std::string getKeyfilenameFor(std::string keyfilesPath, std::string filename);

// dummy panotools progress functions
static int ptProgress( int command, char* argument )
{
    return 1;
}
static int ptinfoDlg( int command, char* argument )
{
    return 1;
}

#endif // __detectpano_panodetector_h
