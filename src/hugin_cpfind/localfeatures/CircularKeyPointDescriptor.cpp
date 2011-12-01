/* *-* tab-width: 4; c-basic-offset: 4; -*- */
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

#include <iostream>

#ifdef _MSC_VER
#define _USE_MATH_DEFINES
#endif
#include <math.h>
#include <vector>
#include <map>
#include <fstream>

#include <string.h>

#include "KeyPoint.h"
#include "CircularKeyPointDescriptor.h"
#include "MathStuff.h"
#include "WaveFilter.h"

//#define DEBUG_DESC
//#define DEBUG_ROT

using namespace lfeat;
using namespace std;

LUT<0, 83> Exp1_2(exp, 0.5, -0.08);

CircularKeyPointDescriptor::CircularKeyPointDescriptor(Image& iImage,
        std::vector<int> rings, std::vector<double>ring_radius,
        std::vector<double>ring_gradient_width,
        int ori_nbins, double ori_sample_scale, int ori_gridsize) :
    _image(iImage), _ori_nbins(ori_nbins), _ori_sample_scale(ori_sample_scale),
    _ori_gridsize(ori_gridsize)
{
    // default parameters
    if (rings.size() == 0)
    {
        rings.push_back(1);
        ring_radius.push_back(0);
        ring_gradient_width.push_back(2);
        rings.push_back(6);
        ring_radius.push_back(4.2);
        ring_gradient_width.push_back(1.9);
        /*
        rings.push_back(6);
        ring_radius.push_back(5);
        ring_gradient_width.push_back(3);
        */
        rings.push_back(6);
        ring_radius.push_back(8);
        ring_gradient_width.push_back(3.2);
    }

    assert(rings.size() == ring_radius.size());
    assert(rings.size() == ring_gradient_width.size());

    /*
    // radius of the rings (ring 0 is the center point)
    double ring_radius[3];
    ring_radius[0] =  0;
    ring_radius[1] =  2;
    ring_radius[2] =  5;

    // width of the boxfilter used during the computation (ring 0 is the center point)
    double ring_gradwidth[3];
    ring_gradwidth[0] = 0.5;
    ring_gradwidth[1] = 1.5;
    ring_gradwidth[2] = 2.5;

    // number of entries for each ring
    int nrings[3];
    nrings[0] = 1;
    nrings[1] = 6;
    nrings[2] = 6;
    */
    // compute number of sampling points
    _subRegions = 0;
    for (unsigned int i=0; i < rings.size(); i++)
    {
        _subRegions += rings[i];
    }

    // create a list of sample parameters
    _samples = new SampleSpec[_subRegions];

    // precompute positions of the sampling points
    int j=0;
    for (unsigned int i=0; i < rings.size(); i++)
    {
        // alternate the phi offset of the rings,
        // so that the circles don't overlap too much with the
        // next ring
        double phioffset = i % 2== 0 ? 0 : M_PI/rings[i];
        for (int ri=0; ri < rings[i]; ri++)
        {
            double phi = ri*2*M_PI /rings[i] + phioffset;
            _samples[j].x = ring_radius[i]*cos(phi);
            _samples[j].y = ring_radius[i]*sin(phi);
            _samples[j].size = ring_gradient_width[i];
            j++;
        }
    }

    // compute 4 gradient entries (pos(dx), pos(-dx), pos(dy), pos(-dy))
    // maybe use something else here, for example, just the gradient entries...
    // also use LBP as a 5th feature
    _vecLen = 3;
    _descrLen = _vecLen * _subRegions - 1;

    _ori_hist = new double [_ori_nbins + 2];

}

CircularKeyPointDescriptor::~CircularKeyPointDescriptor()
{
    delete[] _ori_hist;
    delete[] _samples;
}

void CircularKeyPointDescriptor::makeDescriptor(KeyPoint& ioKeyPoint) const
{
    // create a descriptor context
    //KeyPointDescriptorContext aCtx(_subRegions, _vecLen, ioKeyPoint._ori);

    // create the storage in the keypoint
    if (!ioKeyPoint._vec)
    {
        ioKeyPoint.allocVector(getDescriptorLength());
    }

    // create a vector
    createDescriptor(ioKeyPoint);

    // normalize
    Math::Normalize(ioKeyPoint._vec, getDescriptorLength());
}

int CircularKeyPointDescriptor::assignOrientation(KeyPoint& ioKeyPoint, double angles[4]) const
{
    double* hist = _ori_hist+1;
    unsigned int aRX = Math::Round(ioKeyPoint._x);
    unsigned int aRY = Math::Round(ioKeyPoint._y);
    int aStep = (int)(ioKeyPoint._scale + 0.8);

    WaveFilter aWaveFilter(_ori_sample_scale * ioKeyPoint._scale + 1.5, _image);

#ifdef DEBUG_ROT_2
    std::cerr << "ori_scale = " << 2.5 * ioKeyPoint._scale + 1.5 << std::endl;
    std::cerr << "ori= [ ";
#endif

#ifdef _MSC_VER
#pragma message("use LUT after parameter tuning")
#else
#warning use LUT after parameter tuning
#endif
    double coeffadd = 0.5;
    double coeffmul = (0.5 + 6 ) / - (_ori_nbins*_ori_nbins);

    memset(_ori_hist, 0, sizeof(double)*(_ori_nbins + 2));
    // compute haar wavelet responses in a circular neighborhood of _ori_gridsize s
    for (int aYIt = -_ori_gridsize; aYIt <= _ori_gridsize; aYIt++)
    {
        int aSY = aRY + aYIt * aStep;
        for (int aXIt = -_ori_gridsize; aXIt <= _ori_gridsize; aXIt++)
        {
            int aSX = aRX + aXIt * aStep;
            // keep points in a circular region of diameter 6s
            unsigned int aSqDist = aXIt * aXIt + aYIt * aYIt;
            if (aSqDist <= _ori_nbins*_ori_nbins && aWaveFilter.checkBounds(aSX, aSY))
            {
                double aWavX = aWaveFilter.getWx(aSX, aSY);
                // y axis for derivate seems is from bottom to top (typical math coordinate system),
                // not from top to bottom (as in the typical image coordinate system).
                double aWavY = - aWaveFilter.getWy(aSX, aSY);
                double aWavResp = sqrt(aWavX * aWavX + aWavY * aWavY);
                if (aWavResp > 0)
                {
                    // add PI ->  0 .. 2*PI interval
                    double angle = atan2(aWavY, aWavX)+PI;
                    int bin =  angle/(2*PI) * _ori_nbins;
                    // deal with possible rounding problems.
                    bin = (bin+_ori_nbins)%_ori_nbins;
                    // center of bin 0 equals -PI + 16°deg, etc.
                    double weight = exp(coeffmul * (aSqDist+coeffadd));
                    hist[bin] += aWavResp * weight;
                    //hist[bin] += aWavResp * Exp1_2(aSqDist);
#ifdef DEBUG_ROT_2
                    std::cerr << "[ " << aSX << ", " << aSY << ", "
                              << aWavX << ", " << aWavY << ", "
                              << aWavResp << ", " << angle << ", " << bin << ", "
                              << aSqDist << ", " << aWavResp* Exp1_2(aSqDist) << "], " << std::endl;
#endif
                }
            }
        }
    }
#ifdef DEBUG_ROT_2
    std::cerr << "]" << endl;
#endif

#if 0
    // smoothing doesn't seem to work very well with the default grid + scale values for orientation histogram building
    // smooth histogram
    for (int it=0; it < 1; it++)
    {
        double prev = hist[_ori_nbins-1];
        double first = hist[0];
        for (int i=0; i < _ori_nbins-2; i++)
        {
            double hs = (prev + hist[i] + hist[i+1]) / 3.0;
            prev = hist[i];
            hist[i] = hs;
        }
        hist[_ori_nbins-1] = (prev + hist[_ori_nbins-1] + first) / 3.0;
    }
#endif

    // avoid boundary problems, wrap around histogram
    hist[-1] = hist[_ori_nbins-1];
    hist[_ori_nbins] = hist[0];

    // find bin with the maximum response.
    double aMax = hist[0];
    int iMax = 0;
#ifdef DEBUG_ROT
    std::cerr << "rot_hist: [ " << aMax;
#endif
    for (int i=1; i < _ori_nbins; i++)
    {
#ifdef DEBUG_ROT
        std::cerr << ", " << hist[i];
#endif
        if (hist[i] > aMax)
        {
            aMax = hist[i];
            iMax = i;
        }
    }
#ifdef DEBUG_ROT
    std::cerr << " ] " << std::endl;
#endif

    double prev = hist[iMax-1];
    double curr = hist[iMax];
    double next = hist[iMax+1];
    double dsub = -0.5*(next-prev)/(prev+next-2*curr);
    ioKeyPoint._ori = (iMax+0.5+dsub) / _ori_nbins * 2*PI - PI;

    // add more keypoints that are within 0.8 of the maximum strength.
    aMax *= 0.8;
    int nNewOri = 0;
    for (int i=0; i < _ori_nbins; i++)
    {
        double prev = hist[i-1];
        double curr = hist[i];
        double next = hist[i+1];
        if (curr > aMax && prev < curr && next < curr && i != iMax)
        {
            dsub = -0.5*(next-prev)/(prev+next-2*curr);
            angles[nNewOri] = (i+0.5+dsub) / _ori_nbins * 2*PI - PI;
            nNewOri++;
            if (nNewOri == 4)
            {
                break;
            }
        }
    }
    return nNewOri;
}

// gradient and intensity difference
void CircularKeyPointDescriptor::createDescriptor(KeyPoint& ioKeyPoint) const
{
#ifdef DEBUG_DESC
    std::ofstream dlog("descriptor_details.txt", std::ios_base::app);
#endif

    // create the vector of features by analyzing a square patch around the point.
    // for this the current patch (x,y) will be translated in rotated coordinates (u,v)

    double aX = ioKeyPoint._x;
    double aY = ioKeyPoint._y;
    int aS = (int)ioKeyPoint._scale;

    // get the sin/cos of the orientation
    double ori_sin = sin(ioKeyPoint._ori);
    double ori_cos = cos(ioKeyPoint._ori);

    if (aS < 1)
    {
        aS = 1;
    }

    // compute the gradients in x and y for all regions of interest

    // we override the wave filter size later
    WaveFilter aWaveFilter(10, _image);

    // compute features at each position and store in feature vector
    int j=0;
    double middleMean = 0;

    for (int i=0; i < _subRegions; i++)
    {
        // scale radius with aS.
        double xS = _samples[i].x * aS;
        double yS = _samples[i].y * aS;
        // rotate sample point with the orientation
        double aXSample = aX + xS * ori_cos - yS * ori_sin;
        double aYSample = aY + xS * ori_sin + yS * ori_cos;
        // make integer values from double ones
        int aIntXSample = Math::Round(aXSample);
        int aIntYSample = Math::Round(aYSample);
        int aIntSampleSize = Math::Round(_samples[i].size* aS);
        int sampleArea = aIntSampleSize * aIntSampleSize;

        if (!aWaveFilter.checkBounds(aIntXSample, aIntYSample, aIntSampleSize))
        {
            ioKeyPoint._vec[j++] = 0;
            ioKeyPoint._vec[j++] = 0;
            //ioKeyPoint._vec[j++] = 0;
            //ioKeyPoint._vec[j++] = 0;
            if (i > 0)
            {
                ioKeyPoint._vec[j++] = 0;
            }
#ifdef DEBUG_DESC
            dlog << xS << " " << yS << " "
                 << aIntXSample << " " << aIntYSample << " " << aIntSampleSize << " "
                 << 0 << " " << 0 << " " << 0 << " " << 0 << " " << 0 << std::endl;
#endif
            continue;
        }

        double aWavX = aWaveFilter.getWx(aIntXSample, aIntYSample, aIntSampleSize) / sampleArea;
        double aWavY = - aWaveFilter.getWy(aIntXSample, aIntYSample, aIntSampleSize) / sampleArea;

        double meanGray = aWaveFilter.getSum(aIntXSample, aIntYSample, aIntSampleSize) / sampleArea;

        if (i == 0)
        {
            middleMean = meanGray;
        }

        // rotate extracted gradients

        //  need to rotate in the other direction?

        double aWavXR = aWavX * ori_cos + aWavY * ori_sin;
        double aWavYR = -aWavX * ori_sin + aWavY * ori_cos;
        /*
        double aWavXR = aWavX * ori_cos - aWavY * ori_sin;
        double aWavYR = aWavX * ori_sin + aWavY * ori_cos;
        */

#ifdef DEBUG_DESC
        dlog << xS << " " << yS << " "
             << aIntXSample << " " << aIntYSample << " " << aIntSampleSize << " "
             << aWavX << " " << aWavY << " " << meanGray << " " << aWavXR << " " << aWavYR << std::endl;
#endif

        // store descriptor
        ioKeyPoint._vec[j++] = aWavXR;
        ioKeyPoint._vec[j++] = aWavYR;
        /*
        if (aWavXR > 0) {
        	ioKeyPoint._vec[j++] = aWavXR;
        	ioKeyPoint._vec[j++] = 0;
        } else {
        	ioKeyPoint._vec[j++] = 0;
        	ioKeyPoint._vec[j++] = -aWavXR;
        }
        if (aWavYR > 0) {
        	ioKeyPoint._vec[j++] = aWavYR;
        	ioKeyPoint._vec[j++] = 0;
        } else {
        	ioKeyPoint._vec[j++] = 0;
        	ioKeyPoint._vec[j++] = -aWavYR;
        }
        */
        if (i != 0)
        {
            ioKeyPoint._vec[j++] = meanGray - middleMean;
        }
    }
#ifdef DEBUG_DESC
    dlog.close();
#endif

}
