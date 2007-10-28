// -*- c-basic-offset: 4 -*-

/** @file jbu.h
 *
 *  @author Jing Jin <jingidy@gmail.com>
 *
 *  $Id$
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
 

#include <vigra/error.hxx>
#include <hugin_utils/utils.h>
#include <vigra_ext/impexalpha.hxx>

#include <boost/shared_ptr.hpp>

// define the types of images we will use.
// use float for RGB
typedef vigra::FRGBImage ImageType;
// use byte for original grey value in source image
typedef vigra::BImage WeightImageType;

// smart pointers to the images.
typedef boost::shared_ptr<ImageType> ImagePtr;
typedef boost::shared_ptr<vigra::FImage> FImagePtr;
typedef boost::shared_ptr<vigra::BImage> BImagePtr;

/** upsamples multiple images given vector to store results in */
void jointBilateralUpsampling(std::vector<FImagePtr> *sources, 
								std::vector<FImagePtr> *refs,
								std::vector<FImagePtr> *destImgs, 
								int num_neighbors);

/** upsamples a single image and returns the FImagePtr to the result */
FImagePtr jbuImage(FImagePtr source, FImagePtr refIm, int num_neighbors);

/** given 2 floating point pixel locations, computes the distance */
float dist(float x1, float y1, float x2, float y2);

/** gaussian applied to a single variable */
float simpleGauss(float x, float sigma, float mu);
