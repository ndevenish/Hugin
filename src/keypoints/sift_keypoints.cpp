// -*- c-basic-offset: 4 -*-
/** @file sift_keypoints.cpp
 *
 *  @author Alexandre Jenny <alexandre.jenny@le-geo.com>
 *
 *  $Id$
 *
 *  This program is licenced under the SIFT feature detection licence,
 *  see LICENCE_SIFT document
 *
 *  A few code snippets have also been taken from the vigra examples
 *
 * "This software is provided for non-commercial use only. The University of
 * British Columbia has applied for a patent on the SIFT algorithm in the
 * United States. Commercial applications of this software may require a
 * license from the University of British Columbia."
 * For more information, see the LICENSE file supplied with the distribution.
 */

#include <config.h>

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

#include "vigra/impex.hxx"

#include "keypoints/ScaleSpace.h"

using namespace vigra;

Matrix3 Matrix3::Identity;

// ***************************************************
// sift constants : could be changed
const int DODOUBLESIZE = 0;	// does a doubling of picture at beginning

using namespace vigra;

// ***********************************************************************
// PrintKeys
//
void PrintKeys( Keypoints k )
{
    std::cout << k.size() << std::endl;
	
    Keypoints::iterator iIt;
    for (iIt = k.begin(); iIt != k.end(); iIt++)
    {
        std::cout << iIt->col_x << " " << iIt->row_y << " " << iIt->scale << " " << iIt->orientation << " ";
        //for (int i=0; i<128; i++)
        //	std::cout << iIt->feature[i] << " ";
        std::cout << std::endl;
    }
}

void WriteKeyFile( Keypoints k, char* filename )
{
    FILE *stream;

    /* Open for write */
    if( (stream = fopen( filename, "w+" )) != NULL )
    {
        fprintf(stream, "%d 128\r\n", k.size());

        Keypoints::iterator iIt;
        for (iIt = k.begin(); iIt != k.end(); iIt++)
        {
            fprintf(stream, "%f %f %f %f ", iIt->col_x, iIt->row_y, iIt->scale, iIt->orientation);
            for (int i=0; i<128; i++)
                fprintf(stream, "%3d ", iIt->FVBytes[i]);
            fprintf(stream, "\r\n");
        }
    }
    fclose( stream );
}

// ***********************************************************************
// Main
//
int main(int argc, char ** argv)
{
    if(argc != 3)
    {
        std::cout << "Usage: " << argv[0] << " image_file key_file " << std::endl;
        std::cout << "(supported formats: " << impexListFormats() << ")" << std::endl;
        return 1;
    }

    // read image given as first argument
    // file type is determined automatically
    vigra::ImageImportInfo info(argv[1]);

    //=====
    //===== 0. Prepare the image, convert to grayscale and float conversion
    //=====

    // the float grayscale version of the picture
    FImage fin(info.width(), info.height());

    std::cout << "Converting to grayscale" << std::endl;
    if(info.isGrayscale())
    {
        // create a gray scale image of appropriate size
        vigra::BImage in(info.width(), info.height());
        importImage(info, destImage(in));
        // convert to float
        copyImage(srcImageRange(in), destImage(fin));
    }
    else
    {
        // create a RGB image of appropriate size
        BRGBImage in(info.width(), info.height());
        FRGBImage Frgb(info.width(), info.height());
        importImage(info, destImage(in));
        // convert to float RGB
        copyImage(srcImageRange(in), destImage(Frgb));
        // convert from float RGB to float
        copyImage(srcImageRange(Frgb, RGBToGrayAccessor<RGBValue<float> >()), destImage(fin));
    }

    // =====
    // ===== 1. Prepare grayscale picture for sift detection
    // =====
	
    //exportImage( srcImageRange(fin), vigra::ImageExportInfo("fin.jpg"));

    std::cout << "Preparing Picture" << std::endl;
	
    float currentScale = 1.0;
	
    if ( DODOUBLESIZE )
    {
        FImage tmp;
        tmp.resize( fin.width(), fin.height() );
        ResizeDoubleLinear( fin, tmp );
        fin.resize( tmp.width(), tmp.height() );
        copyImage( srcImageRange(tmp), destImage(fin) );
        currentScale = 0.5;
    }
	
    // =====
    // ===== 2. Loop over Scale Spaces
    // =====
    Keypoints keys;
    FImage femp;
    ScaleSpace ss;

    // as long as minimum image size requirements are met:
    // get keypoints, scale down, and over again
    while ( fin.width() > 24 && fin.height() > 24)
    {
        // debug
        std::cout << "Octave Size " << fin.width() << " x " << fin.height() << std::endl;

        // init scale space
        std::cout << "  Computing Dogs" << std::endl;
        ss.InitScaleSpace( currentScale, fin );
        std::cout << "  Retrieving keys" << std::endl;
        ss.FindKeypoints( keys );
        //ss.DebugExport();
		
        // resize and goes next step
        ResizeHalf( fin, femp );
        fin.resize( femp.width(), femp.height() );
        copyImage( srcImageRange(femp), destImage(fin) );
        currentScale *= 2.0;

        // loop
        std::cout << "Number of keys " << keys.size() << std::endl;
    }

    //=====
    //===== 3. Outputs keys
    //=====
    WriteKeyFile( keys, argv[2] );
    return 0;
}
