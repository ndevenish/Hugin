// -*- c-basic-offset: 4 -*-
/** @file ScaleSpace.cpp
 *
 *  @author Alexandre Jenny <alexandre.jenny@le-geo.com>
 *
 *  $Id$
 *
 *  This program is licenced under the SIFT feature detection licence,
 *  see LICENCE_SIFT document
 *
 */

#include "keypoints/ScaleSpace.h"

// ====================================================
// Init this Scale Space
//   Create blur and dogs pictures
//
void ScaleSpace::InitScaleSpace( float currentscale, const FImage & image )
{
    // Reset
    Reset();

    // init local variables
    dimX = image.width();
    dimY = image.height();
    octSize = currentscale;

    int s;
    blur = new FImage[ SCALES + 3 ];
    dogs = new FImage[ SCALES + 2 ];

    float k = pow( 2.0f, 1.0f / SCALES );
    blur[0].resize( dimX, dimY );
    copyImage( srcImageRange(image), destImage(blur[0]) );

    float sigmaW = SIGMA;

	// computes blur maps
    for (s=1; s<SCALES+3; s++)
    {
        blur[s].resize( dimX, dimY );
        copyImage( srcImageRange(blur[s-1]), destImage(blur[s]) );
        float sigmaCalc = sigmaW * sqrt( k*k - 1.0 );
        GaussianBlur( sigmaCalc, blur[s] );
        sigmaW *= k;
    }

    // computes difference of gauss
    for ( s=0; s<SCALES+2; s++ )
    {
        dogs[s].resize( dimX, dimY );
        combineTwoImages( srcImageRange(blur[0]), srcImage(blur[s+1]), destImage(dogs[s]), std::minus<float>() );
        //ImageAbsDiff( srcImageRange(blur[0]), srcImage(blur[s+1]), destImage(dogs[s]) );
    }

    // prepare analysed image
    analysed.resize( dimX, dimY );
    for (int x=0; x<dimX; x++)
        for (int y=0; y<dimY; y++)
            analysed(x, y) = 0;
}

// ====================================================
// Main entry for keypoints retrieval
//   look for keypoints
void ScaleSpace::FindKeypoints( Keypoints & keys )
{
    for (int sc=1; sc<=SCALES; sc++)
    {
        // precalculate for this scale every orientation and gradiant
        GradiantOrientationImages( blur[sc], grad, orie );

        // for every pixels in extension
        for ( int x=EXT; x<dimX-EXT; x++ )
        {
            for ( int y=EXT; y<dimY-EXT; y++ )
            {
				// check for maximum or minimum
                if ( !CheckForMinMax( x, y, dogs[sc-1], dogs[sc], dogs[sc+1] ) )
                    continue;

				// accurate localization and prunning
                AccurateLocalizationAndPruning( x, y, sc, keys );
            }
        }
    }
}

// ***********************************************************************
// AccurateLocalizationAndPruning
//   This is really the heart of the SIFT algorithm
//   It can be decomposed into 3 steps :
//     - 3D interpolation to find the real position of point in (x, y, s) space
//     - handling of already done position
//     - feature vector calculation
//
void ScaleSpace::AccurateLocalizationAndPruning( int x, int y, int s, Keypoints & keys )
{
    Vector3 dDog, dpos;
    float new_heigth, dx2, dy2, dxdy;
    int MaxIter = 6;
    bool moved;
    int new_x = x;
    int new_y = y;

    // =====
    // ===== 1. Interactive 3D interpolation (x direction, y direction, and scale direction)
    // =====
    // Iterate to find good extrenum position until stabilization or too many moves
    while ( (MaxIter--) >= 0 )
    {
        // 3d fit of this extrenum
        dpos = Fit3D( new_x, new_y, s, dDog, dx2, dy2, dxdy );
	
		// calculate new peak height
        new_heigth = dogs[s](new_x, new_y) + 0.5 * dpos.Dot( dDog );

        // pruning small peak
        if ( fabs(new_heigth)< PEAK_THRESH )
            return;

		// pruning big edge response
        float egdetest = sqr(dx2+dy2) / fabs(dx2*dy2-dxdy*dxdy);
        if ( egdetest > sqr(R_EDGE +1)/R_EDGE )
            return;
		
		// some other cases -> col
        if ( fabs(dpos.x) > 1.5f )
            return;
        if ( fabs(dpos.y) > 1.5f )
            return;
        if ( fabs(dpos.z) > 1.5f )
            return;

		// make the moves
        moved = false;
        if ( dpos.x > 0.6 )
        {
            if ( new_x < dimX - EXT )
            {
                new_x++;
                moved = true;
            }
        }
        if ( dpos.x < -0.6 )
        {
            if (new_x > EXT)
            {
                new_x--;
                moved = true;
            }
        }
        if ( dpos.y > 0.6 )
        {	
            if ( new_y < dimY - EXT )
            {
                new_y++;
                moved = true;
            }
        }
        if ( dpos.y < -0.6 )
        {
            if (new_y > EXT )
            {
                new_y--;
                moved = true;
            }
        }

        // if stable, leave
        if (!moved)
            break;
    }

    // =====
    // ===== 2. was already done ? quit
    // =====
    if (analysed(new_x, new_y) != 0.0 )
        return;
    analysed(new_x, new_y) = 1.0;

    // =====
    // ===== 3. new accurate positions in 3d
    // =====
    float new_s = SIGMA * pow (2.0, (s + dpos.z) / SCALES);
    CreateFeatureVector( new_x + dpos.x, new_y + dpos.y, new_s, keys );
}

// ***********************************************************************
// CreateFeatureVector
//   Create the feacture vector by using many technics
//     1. Calculate the orientation vector
//     2. For each peak, create a feature vector
//
void ScaleSpace::CreateFeatureVector( float x, float y, float s, Keypoints & keys )
{
    // int position
    int xi = (int) floor(x + 0.5f);
    int yi = (int) floor(y + 0.5f);
    int si = (int) floor(s + 0.5f);
	
	// array of orientation
    OrientationVector OV;

    // radius calculus
    float sig = 1.5 * s;
    int radius = (int) (3.0 * sig);

	// =====
	// ===== 1. Calculate orientation vector
	// =====
    for (int ind_x = xi - radius; ind_x < xi + radius; ind_x++ )
    {
        for (int ind_y = yi - radius; ind_y < yi + radius; ind_y++ )
        {
            // check limits
            if (( ind_x < 0 ) || ( ind_y < 0) || (ind_x >= dimX) || (ind_y >= dimY) )
                continue;

            // gradiant
            float gradVal = grad( ind_x, ind_y );
            float distSq  = sqr( ind_x - x ) + sqr( ind_y - y );
			
            // outside the circle
            if ( distSq >= radius * radius )
                continue;

            float gaussianWeight = exp ( -distSq / (2.0 * sig * sig) );

            // add this orientation in the vector
            OV.AddOrientation( orie( ind_x, ind_y), gaussianWeight * gradVal );
        }
    }

    // 3 passes smoothing;
    OV.Smooth();
    OV.Smooth();
    OV.Smooth();

    // find the max
    float maxPeakValue = OV.FindMax();
	
    // =====
    // ===== 2. Create feature vector of with peaks orientations
    // =====
    for (int bin=0; bin<ORIENTARRAY; bin++ )
    {
        int left = ORIENTARRAY - 1;
        if (bin > 0)
            left = bin - 1;

        int right = 0;
        if (bin < (ORIENTARRAY - 1) )
            right = bin + 1;

        // only consider local peaks
        if ( OV.Orient[left] >= OV.Orient[bin] )
            continue;
        if ( OV.Orient[right] >= OV.Orient[bin] )
            continue;
        if ( OV.Orient[bin] < (0.8 * maxPeakValue) )
            continue;

        // interpolates the values
        //  with f(x) = ax² + bx + c
        //   f(-1) = x0
        //   f( 0) = x1
        //   f(+1) = x2
        //  => a = (x0+x2)/2 - x1
        //     b = (x2-x0)/2
        //     c = x1
        // f'(x) = 0 => x = -b/2a
        float x0 = OV.Orient[left];
        float x1 = OV.Orient[bin];
        float x2 = OV.Orient[right];
        float a  = 0.5f*(x0+x2) - x1;
        float b  = 0.5f*(x2-x0);
        float realangle = bin-b/(2*a);

        // [0:36[ to [0:2*pi[
        realangle *= 2 * PI / ORIENTARRAY;
        // [0:2pi[ to [-pi:pi[
        realangle -= PI;

        // Create keypoint
        Keypoint newkey;

        newkey.col_x		= x * octSize;
        newkey.row_y		= y * octSize;
        newkey.scale		= s * octSize;
        newkey.orientation	= realangle;

        ComputeFeature( x, y, s, newkey );
		
        keys.push_back( newkey );
    }
}

// ***********************************************************************
// ComputeFeature
// Does the calculation of the 128 float feature vector
//
//
//
void ScaleSpace::ComputeFeature( float x, float y, float s, Keypoint & key )
{
    FeatureVector fv;

    //
    int xi = (int) (x + 0.5f);
    int yi = (int) (y + 0.5f);

    float sinAngle = sin( key.orientation );
    float cosAngle = cos( key.orientation );

    // check this scale
    float scaleCompute = 3.0 * s;
    int radius = 8;
    for (int rx=-radius; rx<radius; rx++ )
    {
        float xf = (rx+radius)/4.0f;
        for (int ry=-radius; ry<radius; ry++ )
        {
            float yf = (ry+radius)/4.0f;

            // ouside circle
            if ( sqr(rx) + sqr(ry) >= radius*radius )
                continue;
            if ( ( xi+rx<0) || ( xi+rx>= dimX ) || (yi+ry<0) || ( yi+ry >= dimY ) )
                continue;

            // rotate the points
            float newX = rx * cosAngle - ry * sinAngle;
            float newY = rx * sinAngle + ry * cosAngle;
			
            float xRel = newX + xi - x;
            float yRel = newY + yi - y;
            float gaussFactor = exp ( -(xRel * xRel + yRel * yRel) / ( scaleCompute * scaleCompute ) );

            float weightedMagnitude = gaussFactor * grad( xi+rx, yi+ry );

            //if (weightedMagnitude < 0.0001)
            //	continue;

            float gradAng = orie( xi+rx, yi+ry ) - key.orientation;

            if (gradAng>PI)
                gradAng -= 2* PI;
            if (gradAng<-PI)
                gradAng += 2* PI;

            float orient = (gradAng+PI) * 8 / (2 * PI);

            // will be placed in 2 x 2 x 2 cases :
            //  [ xi xi+1 ] x [ yi yi+1 ] x [ angle, angle+1 ]
			
            fv.AddValue( xf, yf, orient, weightedMagnitude );
        }
    }

    fv.Normalize();
    fv.Threshold( 0.2f );
	
    for (int n = 0 ; n < FVSIZE ; n++)
    {
        int intval = (int) ( fv.FVFloat[n] * 512.0);
        //assert (intval >= 0);
        if (intval > 255)
            intval = 255;
        key.FVBytes[n] = (unsigned char) intval;
    }
}

// ====================================================
// Just Check in the local 3x3 space, if it's a max or min
bool ScaleSpace::CheckForMinMax( int x, int y, const FImage & im0, const FImage & im1, const FImage & im2 )
{
    double val = im1(x, y);

    // peak threshold
    if ( fabs(val) < PEAK_THRESH )
        return false;

    // verify for max or min
    if (val<im1(x-1, y))
    {
        // check for minimal
        if (val>im1(x-1, y-1))  return false;
        if (val>im1(x-1, y+1))	return false;
        if (val>im1(x  , y-1))	return false;
        if (val>im1(x  , y+1))	return false;
        if (val>im1(x+1, y-1))	return false;
        if (val>im1(x+1, y  ))	return false;		
        if (val>im1(x+1, y+1))	return false;

        // check for minimal level -1
        if (val>im0(x-1, y-1))	return false;
        if (val>im0(x-1, y  ))	return false;
        if (val>im0(x-1, y+1))	return false;
        if (val>im0(x  , y-1))	return false;
        if (val>im0(x  , y  ))	return false;
        if (val>im0(x  , y+1))	return false;
        if (val>im0(x+1, y-1))	return false;
        if (val>im0(x+1, y  ))	return false;
        if (val>im0(x+1, y+1))	return false;

        // check for minimal level +1
        if (val>im2(x-1, y-1))	return false;
        if (val>im2(x-1, y  ))	return false;
        if (val>im2(x-1, y+1))	return false;
        if (val>im2(x  , y-1))	return false;
        if (val>im2(x  , y  ))	return false;
        if (val>im2(x  , y+1))	return false;
        if (val>im2(x+1, y-1))	return false;
        if (val>im2(x+1, y  ))	return false;
        if (val>im2(x+1, y+1))  return false;
        return true;
    }
    else
    {
        // check for maximum
        if (val<im1(x-1, y-1))	return false;
        if (val<im1(x-1, y+1))	return false;
        if (val<im1(x  , y-1))	return false;
        if (val<im1(x  , y+1))	return false;
        if (val<im1(x+1, y-1))	return false;
        if (val<im1(x+1, y  ))	return false;
        if (val<im1(x+1, y+1))	return false;

        // check for minimal level -1
        if (val<im0(x-1, y-1))	return false;
        if (val<im0(x-1, y  ))	return false;
        if (val<im0(x-1, y+1))	return false;
        if (val<im0(x  , y-1))	return false;
        if (val<im0(x  , y  ))	return false;
        if (val<im0(x  , y+1))	return false;
        if (val<im0(x+1, y-1))	return false;
        if (val<im0(x+1, y  ))	return false;
        if (val<im0(x+1, y+1))	return false;

        // check for minimal level +1
        if (val<im2(x-1, y-1))	return false;
        if (val<im2(x-1, y  ))	return false;
        if (val<im2(x-1, y+1))	return false;
        if (val<im2(x  , y-1))	return false;
        if (val<im2(x  , y  ))	return false;
        if (val<im2(x  , y+1))	return false;
        if (val<im2(x+1, y-1))	return false;
        if (val<im2(x+1, y  ))	return false;
        if (val<im2(x+1, y+1))	return false;
        return true;
    }
}

// ====================================================
// Fit in 3D
//   in the (X, Y, S) space, fit the peak, to find it's best extrenum
//   return solution and dDog as first derivative of Dog
Vector3 ScaleSpace::Fit3D( int x, int y, int s, Vector3 & dDog, float& dX2, float& dY2, float& dXdY)
{
    FImage& Below	= dogs[s - 1];
    FImage& Cur		= dogs[s    ];
    FImage& Above	= dogs[s + 1];

    // standard approximations of first derivatives d(DOG)/d(?)
    float dX = 0.5 * ( Cur(x+1, y) - Cur(x-1, y) );
    float dY = 0.5 * ( Cur(x, y+1) - Cur(x, y-1) );
    float dS = 0.5 * ( Above(x, y) - Below(x, y) );

    // standard approximations of seconds derivatives d²(DOG)/d(?²)
    /*float*/ dX2	= Cur(x-1, y) - 2.0 * Cur(x, y) + Cur(x+1, y);
    /*float*/ dY2	= Cur(x, y-1) - 2.0 * Cur(x, y) + Cur(x, y+1);
    float dS2	= Below(x, y) - 2.0 * Cur(x, y) + Above(x, y);
	
    // standard approximation of crossed derivatives d²(DOG)/d?d?
    /*float*/ dXdY  = 0.25 * ( Cur(x+1, y+1) - Cur(x-1, y+1)) - 0.5 * ( Cur(x+1, y-1) - Cur(x-1, y-1) );
    float dSdX	= 0.25 * ( Above(x+1, y) - Above(x-1, y)) - 0.5 * ( Below(x+1, y) - Below(x-1, y) );
    float dSdY	= 0.25 * ( Above(x, y+1) - Above(x, y-1)) - 0.5 * ( Below(x, y+1) - Below(x, y-1) );
		
    // matrix to solve is
    // dX         [ dX²  dXdY dXdS ]
    // dY = [ V ] [ dXdY dY²  dYdS ]
    // dS         [ dXdS dYdS dS²  ]
	
    Matrix3 mat;
    Vector3 solution;

    dDog.Set( dX, dY, dS );
    mat.m[0][0] = dX2;
    mat.m[1][1] = dY2;
    mat.m[2][2] = dS2;
    mat.m[1][0] = mat.m[0][1] = dXdY;
    mat.m[2][0] = mat.m[0][2] = dSdX;
    mat.m[2][1] = mat.m[1][2] = dSdY;

    solution = mat.Inverse().TransformVector( dDog );
    return solution;
}
