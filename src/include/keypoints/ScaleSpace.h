// -*- c-basic-offset: 4 -*-
/** @file ScaleSpace.h
 *
 *  @author Alexandre Jenny <alexandre.jenny@le-geo.com>
 *
 *  $Id$
 *
 *  This program is licenced under the SIFT feature detection licence,
 *  see LICENCE_SIFT document
 *
 */

#ifdef MSVC
#pragma once
#endif

#ifndef _SCALE_SPACE_H
#define _SCALE_SPACE_H


#include <math.h>
#include <iostream>
#include <queue>
#include <deque>
#include <vector>
#include <functional>
#include "vigra/stdimage.hxx"
#include "vigra/convolution.hxx"
#include "vigra/functorexpression.hxx"
#include "vigra/resizeimage.hxx"
#include "algo4sift.hxx"

#include "common/Vector3.h"
#include "common/Matrix3.h"

using namespace vigra;

// ===== CONST : should not be modified
const int   SCALES     	= 3;	    // number of stage in an octave
const float SIGMA	= 1.6f;     // sigma
const int   EXT		= 5;	    // edge width around picture where no points can be located
const int   ORIENTARRAY	= 36;	    // orientation array size
const int   FVSIZE	= 128;	    // Feature vector size
const int   R_EDGE      = 10;	    // edge response threshold
const float PEAK_THRESH = 0.01f;    // peak below that doesn't count
const float PI          = M_PI;	    // PI


// ===== Orientation Vector
// Stores an orientation vector
//   The array of orientation is what we can call a histogram of orientation around the
//   keypoint. It gives a great part of the caracterization of the keypoint
//
class OrientationVector
{
    public :
    float Orient[ ORIENTARRAY ];     // array of orientation
    OrientationVector()
	{
            memset( Orient, 0, sizeof(float) * ORIENTARRAY );
	}
    ~OrientationVector() { }

    //
    // Accumulates a value
    //
    void AddOrientation( float rad_angle, float value );
	
    //
    // smooth the vector
    //
    void Smooth();
	
    //
    // Retrieve the max from the vector
    //
    float FindMax();
};

// ===== Feature Vector
// stores temporary keypoints features
//  The final feature vector is formed with 'unsigned char'. This one is just float
class FeatureVector
{
    public :
    float FVFloat[ FVSIZE ];	// array of features
    FeatureVector()
	{
            memset( FVFloat, 0, sizeof(float) * FVSIZE );
	}
    ~FeatureVector() {}

    //
    // lineary add a value. Each couple (x,y) will influence 4 positions in the vector
    //
    void AddValue( float x, float y, float orientation, float value );
	
    //
    // normalize Vector
    //
    void Normalize();
	
    //
    // threshold Vector and renormalize
    //
    void Threshold(float limit);
};

// ===== Keypoint Class
// The sift keypoint feature descriptor
//    What is a keypoint ?
//      3D position (x, y, scale)
//      Mean orientation
//      Feature Vector
class Keypoint
{
    public :
    float			col_x;					// col, ie x coord
    float			row_y;					// row, ie y coord
    float			scale;					// scale
    float			orientation;			// orientation
    unsigned char	FVBytes[ 128 ];			// feature

    Keypoint() {}
    ~Keypoint() {}
};
// vector of Keypoint
typedef std::vector<Keypoint, std::allocator<Keypoint> > Keypoints;

// ===== ScaleSpace Class
// It handles a scale space with all the pictures associated.
// Implements what is needed for sift detection in one level of the full SpaceScale
//
class ScaleSpace
{
    private :
    int	    dimX, dimY;	// dimension of picture
    float   octSize;	// dimension of octave ( the current scale )
    FImage* blur;	// blur pictures for this scale space
    FImage* dogs;	// difference of gaussian picture for this scale space

    FImage  analysed;	// map which tells if a pixel was already analysed
    FImage  grad;	// gradiant maps
    FImage  orie;	// orientation maps
	
    //
    // CheckForMaxMin : return true, if it's a max or min in the local 3x3 matrix
    //
    bool CheckForMinMax( int x, int y, const FImage & im0, const FImage & im1, const FImage & im2 );

    //
    // Fit in 3D : in (X, Y, S) space, fits the paraboloide : 3D Hessian matrix inversion
    //
    Vector3 Fit3D( int x, int y, int s, Vector3 & dDog, float& dX2, float& dY2, float& dXdY);

    //
    // Accurate localization and pruning of keypoint
    //
    void AccurateLocalizationAndPruning( int x, int y, int s, Keypoints & keys );

    //
    // create the feature vector
    //
    void CreateFeatureVector( float x, float y, float s, Keypoints & keys );

    //
    // compute the feature vector
    //
    void ComputeFeature( float x, float y, float s, Keypoint & key );
	
public:
    // ctor, dtor
    ScaleSpace() { dogs = NULL; blur = NULL; }
    ~ScaleSpace(void) { Reset(); }

    // reset
    void Reset()
	{
            if (blur)
                delete [] blur;
            blur = NULL;
            if (dogs)
                delete [] dogs;
            dogs = NULL;
	}

    //
    // 1. Init this ScaleSpace. You provide the scale dimemsion and the picture
    //
    void InitScaleSpace( float currentscale, const FImage & image );
	
    //
    // 2. Find Keypoints : Find the keypoints in the previously provided picture
    //
    void FindKeypoints( Keypoints & keys );
};


#endif

