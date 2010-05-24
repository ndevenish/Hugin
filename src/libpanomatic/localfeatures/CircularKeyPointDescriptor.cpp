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

#include <string.h>

#include "KeyPoint.h"
#include "CircularKeyPointDescriptor.h"
#include "MathStuff.h"
#include "WaveFilter.h"

using namespace lfeat;
using namespace std;

LUT<0, 83> Exp1_2(exp, 0.5, -0.08);

CircularKeyPointDescriptor::CircularKeyPointDescriptor(Image& iImage,
		std::vector<int> rings, std::vector<double>ring_radius, 
	    std::vector<double>ring_gradient_width) : 
	_image(iImage)
{
  
	// default parameters
	if (rings.size() == 0) {
		rings.push_back(1);
		ring_radius.push_back(0);
		ring_gradient_width.push_back(1);
		rings.push_back(6);
		ring_radius.push_back(1.6);
		ring_gradient_width.push_back(1.5);
		rings.push_back(6);
		ring_radius.push_back(6);
		ring_gradient_width.push_back(6);
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
	for (unsigned int i=0; i < rings.size(); i++) _subRegions += rings[i];

	// create a list of sample parameters
	_samples = new SampleSpec[_subRegions];

	// precompute positions of the sampling points
	int j=0;
	for (unsigned int i=0; i < rings.size(); i++) {
		// alternate the phi offset of the rings,
		// so that the circles don't overlap too much with the
		// next ring
		double phioffset = i % 2== 0 ? 0 : M_PI/rings[i];
		for (int ri=0; ri < rings[i]; ri++) {
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
	_descrLen = _vecLen * _subRegions;
}

CircularKeyPointDescriptor::~CircularKeyPointDescriptor()
{
	delete[] _samples;
}

void CircularKeyPointDescriptor::makeDescriptor(KeyPoint& ioKeyPoint) const
{
	// create a descriptor context
	//KeyPointDescriptorContext aCtx(_subRegions, _vecLen, ioKeyPoint._ori);

	// create the storage in the keypoint
	if (!ioKeyPoint._vec) {
		ioKeyPoint.allocVector(getDescriptorLength());
	}

	// create a vector
	createDescriptor(ioKeyPoint);

	// normalize
	Math::Normalize(ioKeyPoint._vec, getDescriptorLength());
}

void CircularKeyPointDescriptor::assignOrientation(KeyPoint& ioKeyPoint) const
{
	double hist_real[12];
    double *hist = hist_real+1;
	unsigned int aRX = Math::Round(ioKeyPoint._x);
	unsigned int aRY = Math::Round(ioKeyPoint._y);
	int aStep = (int)(ioKeyPoint._scale + 0.8);
	
	WaveFilter aWaveFilter(2.5 * ioKeyPoint._scale + 1.5, _image);

	memset(hist_real, 0, sizeof(hist_real));
	// compute haar wavelet responses in a circular neighborhood of 6s
	for (int aYIt = -9; aYIt <= 9; aYIt++)
	{ 
		int aSY = aRY + aYIt * aStep;
		for (int aXIt = -9; aXIt <= 9; aXIt++)
		{
			int aSX = aRX + aXIt * aStep;

			// keep points in a circular region of diameter 6s
			unsigned int aSqDist = aXIt * aXIt + aYIt * aYIt;
			if (aSqDist <= 81 && aWaveFilter.checkBounds(aSX, aSY))
			{
				double aWavX = aWaveFilter.getWx(aSX, aSY);
				double aWavY = aWaveFilter.getWy(aSX, aSY);
				double aWavResp = sqrt(aWavX * aWavX + aWavY * aWavY);
				if (aWavResp > 0)
				{
					int bin = ((atan2(aWavY, aWavX) + PI) / (2*PI)*10);
					// deal with possible rounding problems.
					bin = (bin+10)%10;
					// center of bin 0 equals -PI + 16°deg, etc.
					hist[bin] += aWavResp * Exp1_2(aSqDist);
				}
			}
		}
	}

	// find bin with the maximum response.
	double aMax = hist[0];
	int iMax = 0;
	for (int i=1; i < 10; i++) {
		if (hist[i] > aMax) {
			aMax = hist[i];
			iMax = i;
		}
	}

	// avoid boundary problems, wrap around histogram
	hist[-1] = hist[9];
	hist[10] = hist[0];
	// perform subpixel estimation.
	double a=-(2.0*hist[iMax]-hist[iMax-1]-hist[iMax+1]);
	double b = -(hist[iMax+1] - hist[iMax-1]);
	double dsub = b/a/2;
	if (fabs(dsub) > 1) {
		//cerr << "iMax: " << iMax << " dsub: "<<  dsub << " data: " << hist[iMax-1] << " " << hist[iMax] << " " << hist[iMax+1] << endl;
		dsub = 0;
	}

	ioKeyPoint._ori = (iMax+0.5+dsub) / 10.0 * 2*PI - PI;
}

void CircularKeyPointDescriptor::createDescriptor(KeyPoint& ioKeyPoint) const
{
	// create the vector of features by analyzing a square patch around the point.
	// for this the current patch (x,y) will be translated in rotated coordinates (u,v)

	double aX = ioKeyPoint._x;
	double aY = ioKeyPoint._y;
	int aS = ioKeyPoint._scale;

	// get the sin/cos of the orientation
	double ori_sin = sin(ioKeyPoint._ori);
	double ori_cos = cos(ioKeyPoint._ori);

	if (aS < 1) aS = 1;

	// compute the gradients in x and y for all regions of interest

	// we override the wave filter size later
	WaveFilter aWaveFilter(10, _image);

	// compute features at each position and store in feature vector
	int j=0;
	double middleMean = 0;
	for (int i=0; i < _subRegions; i++) {
		// scale radius with aS.
		double xS = _samples[i].x * aS;
		double yS = _samples[i].y * aS;
		// rotate sample point with the orientation
		double aXSample = aX + xS * ori_cos + yS * ori_sin; 
		double aYSample = aY - xS * ori_sin + yS * ori_cos; 
		// make integer values from double ones
		int aIntXSample = Math::Round(aXSample);
		int aIntYSample = Math::Round(aYSample);
		int aIntSampleSize = Math::Round(_samples[i].size* aS);
		int sampleArea = aIntSampleSize * aIntSampleSize;
		
		if (!aWaveFilter.checkBounds(aIntXSample, aIntYSample, aIntSampleSize)) {
			ioKeyPoint._vec[j++] = 0;
			ioKeyPoint._vec[j++] = 0;
			//ioKeyPoint._vec[j++] = 0;
			//ioKeyPoint._vec[j++] = 0;
			if (i > 0) {
				ioKeyPoint._vec[j++] = 0;
			}
			continue;
		}

		double aWavX = aWaveFilter.getWx(aIntXSample, aIntYSample, aIntSampleSize) / sampleArea;
		double aWavY = aWaveFilter.getWy(aIntXSample, aIntYSample, aIntSampleSize) / sampleArea; 

		double meanGray = aWaveFilter.getSum(aIntXSample, aIntYSample, aIntSampleSize) / sampleArea;
		if (j == 0) {
			middleMean = meanGray;
		}

		// rotate extracted gradients
		double aWavXR = aWavX * ori_cos + aWavY * ori_sin;
		double aWavYR = -aWavX * ori_sin + aWavY * ori_cos;
		
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
		if (j != 0) {
			ioKeyPoint._vec[j++] = meanGray < middleMean ? 0 : 1;
		}
	}
}
