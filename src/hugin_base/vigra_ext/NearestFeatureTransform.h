// -*- c-basic-offset: 4 -*-
/*
 * Copyright (C) 2004 Andrew Mihal
 *
 * This file is part of Enblend.
 *
 * Adapted to vigra & panoramic stitcher by Pablo d'Angelo
 *
 * Enblend is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Enblend is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Enblend; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef NEARESTFEATURETRANSFORM_H
#define NEARESTFEATURETRANSFORM_H

#include <iostream>
#include <list>
#include <math.h>
#include <vigra/basicimage.hxx>
#include <vigra/functorexpression.hxx>
#include <vigra/combineimages.hxx>

#include <hugin_utils/utils.h>
#include <appbase/ProgressDisplayOld.h>

// HACK, for uint typedefs...
#include <tiff.h>
#include <tiffio.h>

//#include "enblend.h"



/*
extern int Verbose;
extern bool Wraparound;
extern uint32 OutputWidth;
extern uint32 OutputHeight;

// Region of interest for this operation.
extern uint32 UBBFirstX;
extern uint32 UBBLastX;
extern uint32 UBBFirstY;
extern uint32 UBBLastY;
*/

namespace vigra_ext {

#define EUCLIDEAN_METRIC

#ifdef EUCLIDEAN_METRIC
//    typedef uint32 dist_t;
//    #define DIST_MAX ((dist_t)-1)
    typedef float dist_t;
    #define DIST_MAX (1e20)
    #define DIST_MIN 0
    inline dist_t distance(uint32 deltaX, dist_t *deltaY) {
        return (*deltaY == DIST_MAX) ? DIST_MAX : *deltaY + deltaX * deltaX;
    }
    inline dist_t distance(uint32 deltaY) {
        return deltaY * deltaY;
    }
#else
#ifdef CHESSBOARD_METRIC
    typedef uint32 dist_t;
    #define DIST_MAX ((dist_t)-1)
    #define DIST_MIN 0
    inline dist_t distance(uint32 deltaX, dist_t *deltaY) {
        return max(deltaX, *deltaY);
    }
    inline dist_t distance(uint32 deltaY) {
        return deltaY;
    }
#else
#ifdef MANHATTAN_METRIC
    typedef uint32 dist_t;
    #define DIST_MAX ((dist_t)-1)
    #define DIST_MIN 0
    inline dist_t distance(uint32 deltaX, dist_t *deltaY) {
        return (*deltaY == DIST_MAX) ? DIST_MAX : deltaX + *deltaY;
    }
    inline dist_t distance(uint32 deltaY) {
        return deltaY;
    }
#endif
#endif
#endif


#ifdef DEBUG_NEAREST
static inline void saveDist(dist_t *ptr, uint32 w, uint32 h, const char * filename)
{

    TIFF *outputTIFF = TIFFOpen(filename, "w");
    if (outputTIFF == NULL) {
        std::cerr << "enblend: error opening TIFF file \""
             << filename << "\"" << std::endl;
        exit(1);
    }

    TIFFSetField(outputTIFF, TIFFTAG_ORIENTATION, 1);
    TIFFSetField(outputTIFF, TIFFTAG_SAMPLEFORMAT, 3);
    TIFFSetField(outputTIFF, TIFFTAG_SAMPLESPERPIXEL, 1);
    TIFFSetField(outputTIFF, TIFFTAG_BITSPERSAMPLE, 32);
    TIFFSetField(outputTIFF, TIFFTAG_IMAGEWIDTH, w);
    TIFFSetField(outputTIFF, TIFFTAG_IMAGELENGTH, h);
    TIFFSetField(outputTIFF, TIFFTAG_PHOTOMETRIC, 1);
    TIFFSetField(outputTIFF, TIFFTAG_PLANARCONFIG, 1);

    for (uint32 scan = 0; scan < h; scan++) {
        TIFFWriteScanline(outputTIFF,
                          &(ptr[scan * w]),
                          scan,
                          1);
    }
    TIFFClose(outputTIFF);
}

static void saveDistRaw(void * ptr, uint32 w, uint32 h, const char * file)
{
    FILE * f = fopen(file,"wb+");
    if (f == NULL) {
        std::cerr << "enblend: error creating temporary file." << std::endl;
        exit(1);
    }
    size_t size = w*h;
    size_t itemsWritten = fwrite(ptr, sizeof(dist_t), size, f);
    if (itemsWritten < size) {
        std::cerr << "enblend: error writing to temporary file." << std::endl;
        perror("reason");
        exit(1);
    }
}
#endif

/** Perform a nearest feature transform on the input image within the union
 *  bounding box. For each thinnable pixel, determine if the pixel is closer
 *  to a green pixel or a blue pixel. Make the thinnable pixel the same color
 *  as the closest green or blue pixel.
 *
 *  note about the color channels: b = whiteimage = mask1
 *  note about the color channels: g = blackimage = mask2
 */
template <typename Feat1Iter, typename Feat1Accessor,
          typename Feat2Iter, typename Feat2Accessor,
          typename Mask1Iter, typename Mask1Accessor,
          typename Mask2Iter, typename Mask2Accessor>
void nearestFeatureTransform(vigra::triple<Feat1Iter, Feat1Iter, Feat1Accessor> feat1,
                             std::pair<Feat2Iter, Feat2Accessor> feat2,
                             std::pair<Mask1Iter, Mask1Accessor> mask1,
			     std::pair<Mask2Iter, Mask2Accessor> mask2,
			     AppBase::MultiProgressDisplay & progress)
{
    typedef typename Mask1Accessor::value_type MaskType;
    typedef vigra::BasicImage<MaskType>       FeatImage;
    typedef typename FeatImage::Iterator      FeatIter;
    progress.pushTask(AppBase::ProgressTask("blend mask", "creating blend line",1/4));	

    vigra::Diff2D ubbSize = feat1.second - feat1.first;

    uint32 ubbWidth = ubbSize.x;
    uint32 ubbHeight = ubbSize.y;
    uint32 ubbPixels = ubbWidth * ubbHeight;

    // mask for the feature pixels ( a feature pixel is
    // feat1 xor feat2
    FeatImage feature(ubbSize);

    // For each pixel, store the distance to the nearest feature in the same
    // column.
    dist_t *dnfColumn = (dist_t*)malloc(ubbPixels * sizeof(dist_t));

    if (dnfColumn == NULL) {
        std::cerr << std::endl
             << "enblend: out of memory (in nearestFeatureTransform for dnfColumn)" << std::endl;
        exit(1);
    }

    // For each pixel, store the distance to the nearest feature in the same
    // column or any column to the left.
    dist_t *dnfLeft = (dist_t*)malloc(ubbPixels * sizeof(dist_t));
    if (dnfLeft == NULL) {
        std::cerr << "enblend: out of memory (in nearestFeatureTransform for dnfLeft)" << std::endl;
        exit(1);
    }

    // Initialize dnfColumn top-down. Store the distance to the nearest feature
    // in the same column and above us.
    /*
    if (Verbose > 0) {
        cout << "Creating blend mask: 1/4 ";
        cout.flush();
    }
    */

    //    MaskPixel *firstMaskP = mask;

    Mask1Iter m1x(mask1.first);
    Mask2Iter m2x(mask2.first);

    Feat1Iter f1x(feat1.first);
    Feat2Iter f2x(feat2.first);

    //FeatIter featx(feature.upperLeft());
    //    MaskIter mux(maskUnion);

    for (uint32 x = 0; x < ubbWidth; x++, ++m1x.x, ++m2x.x, ++f1x.x, ++f2x.x) {
        dist_t *dnfColumnP = &dnfColumn[x];
        //MaskPixel *maskP = firstMaskP + x;

        // Color of the last feature pixel.
        MaskType lastFeature2 = 0;
        MaskType lastFeature1 = 0;
        // Distance to the last feature pixel in pixels.
        uint32 lastFeatureDeltaY = 0;
        bool foundFirstFeature = false;

	Mask1Iter m1y(m1x);
	Mask2Iter m2y(m2x);
        Feat1Iter f1y(f1x);
        Feat2Iter f2y(f2x);
        for (uint32 y = 0; y < ubbHeight; y++, ++m1y.y, ++m2y.y, ++f1y.y, ++f2y.y) {
            // initialize masks and feature mask (pixel defined by only one image).
	    if ((*f2y) != (*f1y)) {
	    //if (maskP->r == 0) {
                // maskP is a feature pixel
                *dnfColumnP = DIST_MIN;
                lastFeatureDeltaY = 0;
                //lastFeatureG = maskP->g;
                //lastFeatureB = maskP->b;
		lastFeature2 = *f2y;
		lastFeature1 = *f1y;
                *m1y = lastFeature1;
                *m2y = lastFeature2;
                foundFirstFeature = true;
            } else if (foundFirstFeature) {
                // maskP is not a feature.
                *dnfColumnP = distance(lastFeatureDeltaY);
                //maskP->g = lastFeatureG;
                //maskP->b = lastFeatureB;
		*m1y = lastFeature1;
		*m2y = lastFeature2;
            } else {
                *dnfColumnP = DIST_MAX;
            }

            lastFeatureDeltaY++;

            // Move pointers down one row.
            dnfColumnP += ubbWidth;
	    //            maskP += ubbWidth;
        }
    }

#ifdef DEBUG_NEAREST
    vigra::exportImage(vigra::srcImageRange(feature), vigra::ImageExportInfo("nona_0_featuremask.tif"));
    vigra::exportImage(srcIterRange(mask1.first, mask1.first + ubbSize), vigra::ImageExportInfo("nona_1_mask1.tif"));
    vigra::exportImage(srcIterRange(mask2.first, mask2.first + ubbSize), vigra::ImageExportInfo("nona_1_mask2.tif"));
    saveDist(dnfColumn, ubbWidth, ubbHeight, "nona_1_dnfColumn.tif");
    saveDist(dnfLeft, ubbWidth, ubbHeight, "nona_1_dnfLeft.tif");
#endif

    // Initialize dnfColumn bottom-up. Caluclate the distance to the nearest
    // feature in the same column and below us.
    // If this is smaller than the value caluclated in the top-down pass,
    // overwrite that value.
    progress.increase();
    /*
    if (Verbose > 0) {
        cout << "2/4 ";
        cout.flush();
    }
    */

    //MaskPixel *lastMaskP = &mask[ubbPixels - 1];

    m1x = mask1.first + ubbSize - vigra::Diff2D(1,1);
    m2x = mask2.first + ubbSize - vigra::Diff2D(1,1);
    f1x = feat1.first + ubbSize - vigra::Diff2D(1,1);
    f2x = feat2.first + ubbSize - vigra::Diff2D(1,1);

    dist_t *lastDNFColumnP = &dnfColumn[ubbPixels - 1];
    for (uint32 x = 0; x < ubbWidth; x++, --m1x.x, --m2x.x, --f1x.x, --f2x.x) {
        dist_t *dnfColumnP = lastDNFColumnP - x;
        //MaskPixel *maskP = lastMaskP - x;

        // Color of the last feature pixel.
        MaskType lastFeatureG = 0;
        MaskType lastFeatureB = 0;
        // Distance to the last feature pixel in pixels.
        uint32 lastFeatureDeltaY = 0;
        bool foundFirstFeature = false;

	Mask1Iter m1y(m1x);
	Mask2Iter m2y(m2x);
        Feat1Iter f1y(f1x);
        Feat2Iter f2y(f2x);
        for (uint32 y = 0; y < ubbHeight; y++, --m1y.y, --m2y.y, --f1y.y, --f2y.y)
        {
            //if (maskP->r == 0) {
	    if ((*f2y) != (*f1y)) {
                // maskP is a feature pixel.
                //*dnfColumnP = DIST_MIN; don't need to do this again.
                lastFeatureDeltaY = 0;
                //lastFeatureG = maskP->g;
                //lastFeatureB = maskP->b;
		lastFeatureG = *m2y;
		lastFeatureB = *m1y;
                foundFirstFeature = true;
            } else if (foundFirstFeature) {
                // maskP is not a feature.
                dist_t distLastFeature = distance(lastFeatureDeltaY);
                // If last feature is closer than nearest feature above,
                // change distance and color to match last feature.
                if (distLastFeature < *dnfColumnP) {
                    *dnfColumnP = distLastFeature;
                    //maskP->g = lastFeatureG;
                    //maskP->b = lastFeatureB;
		    *m1y = lastFeatureB;
		    *m2y = lastFeatureG;
                }
            }

            lastFeatureDeltaY++;

            // Move pointers up one row.
            dnfColumnP -= ubbWidth;
            //maskP -= ubbWidth;
        }
    }

#ifdef DEBUG_NEAREST
    vigra::exportImage(srcIterRange(mask1.first, mask1.first + ubbSize), vigra::ImageExportInfo("nona_2_mask1.tif"));
    vigra::exportImage(srcIterRange(mask2.first, mask2.first + ubbSize), vigra::ImageExportInfo("nona_2_mask2.tif"));
    saveDist(dnfColumn, ubbWidth, ubbHeight, "nona_2_dnfColumn.tif");
    saveDist(dnfLeft, ubbWidth, ubbHeight, "nona_2_dnfLeft.tif");
#endif
    
    //size_t maxListSize = 0;

    // Calculate dnfLeft for each pixel.
    progress.increase();
    /*
    if (Verbose > 0) {
        cout << "3/4 ";
        cout.flush();
    }
    */

    Mask1Iter m1y(mask1.first);
    Mask2Iter m2y(mask2.first);
    for (uint32 y = 0; y < ubbHeight; y++, ++m1y.y, ++m2y.y) {
        dist_t *dnfLeftP = &dnfLeft[y * ubbWidth];
        dist_t *dnfColumnP = &dnfColumn[y * ubbWidth];
        //MaskPixel *maskP = firstMaskP + (y * ubbWidth);

        // List of dnfColumnP's on the left that might be the closest features
        // to the current dnfColumnP.
        std::list<dist_t*> potentialFeatureList;

        Mask1Iter m1x(m1y);
        Mask2Iter m2x(m2y);
        for (uint32 x = 0; x < ubbWidth; x++, ++m1x.x, ++m2x.x) {
            // First add ourself to the list.
            potentialFeatureList.push_back(dnfColumnP);

            // Iterate through the list starting at the right. For each
            // potential feature, all of the potential features to the left
            // in the list must be strictly closer. If not delete them from
            // the list.
            std::list<dist_t*>::iterator potentialFeature =
		    --(potentialFeatureList.end());
            // The last potential feature is dnfColumnP, just added above.
            // That is in the current column so the distance to that feature
            // is simply *dnfColumnP.
            dist_t distPotentialFeature = *dnfColumnP;
            while (potentialFeature != potentialFeatureList.begin()) {
                // Make an iterator that points to the predecessor.
                std::list<dist_t*>::iterator previousFeature = potentialFeature;
                previousFeature--;

                // previousFeature is this many columns to the left of (x,y).
                uint32 deltaX = dnfColumnP - *previousFeature;

                // previousFeature is this far from (x,y).
                dist_t distPreviousFeature = distance(deltaX, *previousFeature);

                if (distPreviousFeature >= distPotentialFeature) {
                    // previousFeature is not a candidate for dnfLeftP
                    // or anything further to the right.
                    potentialFeatureList.erase(previousFeature);
                } else {
                    // previousFeature is a candidate.
                    potentialFeature = previousFeature;
                    distPotentialFeature = distPreviousFeature;
                }
            }

            // The closest feature to (x,y) in columns <= x is the first
            // potential feature in the list.
            *dnfLeftP = distPotentialFeature;

            // Set color of maskP to be color of closest feature to the left.
            //MaskPixel *maskPLeft = maskP - (dnfColumnP - *potentialFeature);
            int dx = (dnfColumnP - *potentialFeature);
            Mask1Iter m1l(m1x);
            m1l.x -= dx;
            *m1x = *m1l;
            Mask1Iter m2l(m2x);
            m2l.x -= dx;
            *m2x = *m2l;

            // Move pointers right one column.
            dnfLeftP++;
            dnfColumnP++;
            //maskP++;

            //maxListSize = max(maxListSize, potentialFeatureList.size());
        }
    }
    //if (Verbose > 0) {
    //    cout << "max feature list size=" << maxListSize << endl;
    //}
    //maxListSize = 0;

#ifdef DEBUG_NEAREST    
    vigra::exportImage(vigra::srcIterRange(mask1.first, mask1.first + ubbSize), vigra::ImageExportInfo("nona_3_mask1.tif"));
    vigra::exportImage(vigra::srcIterRange(mask2.first, mask2.first + ubbSize), vigra::ImageExportInfo("nona_3_mask2.tif"));
    saveDist(dnfColumn, ubbWidth, ubbHeight, "nona_3_dnfColumn.tif");
    saveDist(dnfLeft, ubbWidth, ubbHeight, "nona_3_dnfLeft.tif");
#endif
    
    // Final pass: calculate the distance to the nearest feature in the same
    // column or any column to the right. If this is smaller than dnfLeftP,
    // Then recolor the pixel to the color of the nearest feature to the right.
    /*
    if (Verbose > 0) {
        cout << "4/4 ";
        cout.flush();
    }
    */
    dist_t *lastDNFLeftP = &dnfLeft[ubbPixels - 1];
    m1y = mask1.first + (ubbSize - vigra::Diff2D(1,1));
    m2y = mask2.first + (ubbSize - vigra::Diff2D(1,1));
    for (uint32 y = 0; y < ubbHeight; y++, --m1y.y, --m2y.y) {
        dist_t *dnfColumnP = lastDNFColumnP - (y * ubbWidth);
        dist_t *dnfLeftP = lastDNFLeftP - (y * ubbWidth);
        //MaskPixel *maskP = lastMaskP - (y * ubbWidth);

        // List of dnfColumnP's on the right that might be the closest features
        // to the current dnfColumnP.
        std::list<dist_t*> potentialFeatureList;

        Mask1Iter m1x(m1y);
        Mask2Iter m2x(m2y);
        for (uint32 x = 0; x < ubbWidth; x++, --m1x.x, --m2x.x) {
            // First add ourself to the list.
            potentialFeatureList.push_back(dnfColumnP);

            // Iterate through list and prune as before.
            std::list<dist_t*>::iterator potentialFeature =
                    --(potentialFeatureList.end());
            dist_t distPotentialFeature = *dnfColumnP;
            while (potentialFeature != potentialFeatureList.begin()) {
                // Iterator that points to predecessor.
                std::list<dist_t*>::iterator previousFeature = potentialFeature;
                previousFeature--;

                // previousFeature is this many columns to the right of (x,y);
                uint32 deltaX = *previousFeature - dnfColumnP;

                // previousFeature is this far from (x,y);
                dist_t distPreviousFeature = distance(deltaX, *previousFeature);

                if (distPreviousFeature >= distPotentialFeature) {
                    // previousFeature is not a candidate.
                    potentialFeatureList.erase(previousFeature);
                } else {
                    // previousFeature is a candidate.
                    potentialFeature = previousFeature;
                    distPotentialFeature = distPreviousFeature;
                }
            }

            // The closest feature on the right is potentialFeature.
            if (*dnfLeftP > distPotentialFeature) {
                // Recolor maskP.
                //MaskPixel *maskPRight = maskP + (*potentialFeature - dnfColumnP);
		Mask1Iter m1r(m1x);
                int dx = (*potentialFeature - dnfColumnP);
                m1r.x += dx;
                *m1x = *m1r;
                Mask2Iter m2r(m2x);
                m2r.x += dx;
                *m2x = *m2r;
            }

            // Move pointers left one column.
            dnfLeftP--;
            dnfColumnP--;
            //maskP--;

            //maxListSize = max(maxListSize, potentialFeatureList.size());
        }
    }
    //if (Verbose > 0) {
    //    cout << "max feature list size=" << maxListSize << endl;
    //}

    // ensure that only visible pixels are set, apply feat masks to calculated masks
    vigra::combineTwoImages(feat1, mask1, mask1,
                            vigra::functor::ifThenElse( vigra::functor::Arg1() > vigra::functor::Param(0),
                                                        vigra::functor::Arg2(),
                                                        vigra::functor::Param(0) ) );
    vigra::combineTwoImages(srcIterRange(feat2.first, feat2.first + ubbSize),
                            mask2, mask2,
                            vigra::functor::ifThenElse( vigra::functor::Arg1() > vigra::functor::Param(0),
                                                        vigra::functor::Arg2(),
                                                        vigra::functor::Param(0) ) );

#ifdef DEBUG_NEAREST
    vigra::exportImage(vigra::srcIterRange(mask1.first, mask1.first + ubbSize), vigra::ImageExportInfo("nona_4_mask1.tif"));
    vigra::exportImage(vigra::srcIterRange(mask2.first, mask2.first + ubbSize), vigra::ImageExportInfo("nona_4_mask2.tif"));
    saveDist(dnfColumn, ubbWidth, ubbHeight, "nona_4_dnfColumn.tif");
    saveDist(dnfLeft, ubbWidth, ubbHeight, "nona_4_dnfLeft.tif");
#endif
    
    progress.increase();

    free(dnfColumn);
    free(dnfLeft);

    progress.popTask();
    return;
}

} // namespace

#endif // NEARESTFEATURETRANSFORM_H
