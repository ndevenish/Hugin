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
* <http://www.gnu.org/licenses/>.
*/

#ifndef __lfeat_keypointdetector_h
#define __lfeat_keypointdetector_h

#include "Image.h"
#include "KeyPoint.h"

namespace lfeat
{

class KeyPointInsertor
{
public:
    virtual void operator()(const KeyPoint& k) = 0;
};

class LFIMPEX KeyPointDetector
{
public:
    // default constructor
    KeyPointDetector();

    // setters for various parameters
    inline void setMaxScales(unsigned int iMaxScales)
    {
        _maxScales = iMaxScales;
    }
    inline void setMaxOctaves(unsigned int iMaxOctaves)
    {
        _maxOctaves = iMaxOctaves;
    }
    inline void setScoreThreshold(double iThreshold)
    {
        _scoreThreshold = iThreshold;
    }

    // detect keypoints and put them in the insertor
    void detectKeypoints(Image& iImage, KeyPointInsertor& iInsertor);

private:

    // internal values of the keypoint detector

    // number of scales
    unsigned int					_maxScales;

    // number of octaves
    unsigned int					_maxOctaves;

    // detection score threshold
    double							_scoreThreshold;

    // initial box filter size
    unsigned int					_initialBoxFilterSize;

    // scale overlapping : how many filter sizes to overlap
    // with default value 3 : [3,5,7,9,11][7,11,15,19,23][...
    unsigned int					_scaleOverlap;

    // some default values.
    const static double kBaseSigma;

    bool fineTuneExtrema(double** * iSH, unsigned int iX, unsigned int iY, unsigned int iS,
                         double& oX, double& oY, double& oS, double& oScore,
                         unsigned int iOctaveWidth, unsigned int iOctaveHeight, unsigned int iBorder);

    bool calcTrace(Image& iImage, double iX, double iY, double iScale, int& oTrace);

    unsigned int		getFilterSize(unsigned int iOctave, unsigned int iScale);
    unsigned int		getBorderSize(unsigned int iOctave, unsigned int iScale);

};

}

#endif //__lfeat_keypointdetector_h
