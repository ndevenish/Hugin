// -*- c-basic-offset: 4 -*-
/** @file ImageTransforms.h
 *
 *  Contains functions to transform whole images.
 *  Can use PTools::Transform or PT::SpaceTransform for the calculations
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
 *
 *  This is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _IMAGETRANSFORMS_H
#define _IMAGETRANSFORMS_H

#include <fstream>

#include <vigra/basicimage.hxx>
#include <vigra_ext/ROIImage.h>
#include <vigra_ext/Interpolators.h>
#include <vigra/impex.hxx>
#include <vigra/impexalpha.hxx>

#include <common/math.h>


namespace vigra_ext
{


/** Transform an image into the panorama
 *
 *  It can be used for partial transformations as well, if the bounding
 *  box of a remapped image is known.
 *
 *  Usage: create an output image @dest that should contain the remapped
 *         @p src image. if @p dest doesn't cover the whole output panorama,
 *         use @p destUL to specify the offset of @p dest from the output
 *         panorama.
 *
 *  @param src    source image
 *  @param dest   (partial) panorama image. the image size needed to
 *                hold the complete remapped image can be calculated using
 *                calcBorderPoints().
 *  @param destUL upper left point of @p dest in final panorama. set to (0,0)
 *                if @p dest has the same size as the complete panorama.
 *  @param transform function used to remap the picture.
 *  @param centerDist image, with the same size as dest, that will contain the
 *                distance of the corrosponding pixel from the center of @p
 *                src. This is useful to calculate nice seams. Use a null
 *                image if this information is not needed.
 *  @param interp Interpolator class (calculates weights for interpolation)
 *
 */
template <class SrcImageIterator, class SrcAccessor,
          class DestImageIterator, class DestAccessor,
          class TRANSFORM,
          class AlphaImageIterator, class AlphaAccessor,
          class Interpolator>
void transformImageIntern(vigra::triple<SrcImageIterator, SrcImageIterator, SrcAccessor> src,
                         vigra::triple<DestImageIterator, DestImageIterator, DestAccessor> dest,
                         std::pair<AlphaImageIterator, AlphaAccessor> alpha,
                         TRANSFORM & transform,
                         vigra::Diff2D destUL,
                         Interpolator interp,
                         utils::MultiProgressDisplay & prog)
{
    vigra::Diff2D destSize = dest.second - dest.first;

    int xstart = destUL.x;
    int xend   = destUL.x + destSize.x;
    int ystart = destUL.y;
    int yend   = destUL.y + destSize.y;

    prog.pushTask(utils::ProgressTask("Remapping", "", 1.0/(yend-ystart)));

    vigra::Diff2D srcSize = src.second - src.first;
    // FIXME: use d & e here.
//    vigra::Diff2D srcMiddle = srcSize / 2;

//    vigra::BilinearInterpolatingAccessor<SrcAccessor, typename SrcAccessor::value_type> interpol(src.third);

    //InterpolatingAccessor(src.third, interp);
    vigra_ext::InterpolatingAccessor<SrcAccessor,
                            typename SrcAccessor::value_type,
                            Interpolator> interpol(src.third, interp);


//    vigra::BilinearInterpolatingAccessor<SrcAccessor, typename SrcAccessor::value_type> interpol(src.third);

//    vigra::BilinearInterpolatingAccessor interpol(src.third);

    // create dest y iterator
    DestImageIterator yd(dest.first);
    // create dist y iterator
    AlphaImageIterator ydist(alpha.first);
    // loop over the image and transform
    for(int y=ystart; y < yend; ++y, ++yd.y, ++ydist.y)
    {
        // create x iterators
        DestImageIterator xd(yd);
        AlphaImageIterator xdist(ydist);
        for(int x=xstart; x < xend; ++x, ++xd.x, ++xdist.x)
        {
            double sx,sy;
            transform.transformImgCoord(sx,sy,x,y);
            // make sure that the interpolator doesn't
            // access pixels outside.. Should we introduce
            // some sort of border treatment?
            if (sx < interp.size/2 -1
                || sx > srcSize.x-interp.size/2 - 1
                || sy < interp.size/2 - 1
                || sy > srcSize.y-interp.size/2 - 1)
            {
                *xdist = 0;
                // nothing..
            } else {
//                cout << x << "," << y << " -> " << sx << "," << sy << " " << std::endl;

//                nearest neighbour
//                *xd = src.third(src.first, vigra::Diff2D((int)round(sx), (int)round(sy)));
                // use given interpolator function.
                *xd = interpol(src.first, sx, sy);
                *xdist = 255;
            }
        }
        if ((y-ystart)%100 == 0) {
            prog.setProgress(((double)y-ystart)/(yend-ystart));
        }
    }
    prog.popTask();
}

/** transform input images with alpha channel */
template <class SrcImageIterator, class SrcAccessor,
          class SrcAlphaIterator, class SrcAlphaAccessor,
          class DestImageIterator, class DestAccessor,
          class TRANSFORM,
          class AlphaImageIterator, class AlphaAccessor,
          class Interpolator>
void transformImageAlphaIntern(vigra::triple<SrcImageIterator, SrcImageIterator, SrcAccessor> src,
                               std::pair<SrcAlphaIterator, SrcAlphaAccessor> srcAlpha,
                               vigra::triple<DestImageIterator, DestImageIterator, DestAccessor> dest,
                               std::pair<AlphaImageIterator, AlphaAccessor> alpha,
                               TRANSFORM & transform,
                               vigra::Diff2D destUL,
                               Interpolator interp,
                               utils::MultiProgressDisplay & prog)
{
    vigra::Diff2D destSize = dest.second - dest.first;

    int xstart = destUL.x;
    int xend   = destUL.x + destSize.x;
    int ystart = destUL.y;
    int yend   = destUL.y + destSize.y;

    prog.pushTask(utils::ProgressTask("Remapping", "", 1.0/(yend-ystart)));

    vigra::Diff2D srcSize = src.second - src.first;
    // FIXME: use d & e here.
//    vigra::Diff2D srcMiddle = srcSize / 2;

//    vigra::BilinearInterpolatingAccessor<SrcAccessor, typename SrcAccessor::value_type> interpol(src.third);

    //InterpolatingAccessor(src.third, interp);
    vigra_ext::InterpolatingAccessor<SrcAccessor,
                                     typename SrcAccessor::value_type,
                                     Interpolator> interpol(src.third, interp);

//    vigra::BilinearInterpolatingAccessor<SrcAccessor, typename SrcAccessor::value_type> interpol(src.third);

//    vigra::BilinearInterpolatingAccessor interpol(src.third);

    // create dest y iterator
    DestImageIterator yd(dest.first);
    // create dist y iterator
    AlphaImageIterator ydist(alpha.first);

    typename SrcAccessor::value_type tempval;

    // loop over the image and transform
    for(int y=ystart; y < yend; ++y, ++yd.y, ++ydist.y)
    {
        // create x iterators
        DestImageIterator xd(yd);
        AlphaImageIterator xdist(ydist);
        for(int x=xstart; x < xend; ++x, ++xd.x, ++xdist.x)
        {
            double sx,sy;
            transform.transformImgCoord(sx,sy,x,y);
            // make sure that the interpolator doesn't
            // access pixels outside.. Should we introduce
            // some sort of border treatment?
            if (sx < interp.size/2 -1
                || sx > srcSize.x-interp.size/2 - 1
                || sy < interp.size/2 - 1
                || sy > srcSize.y-interp.size/2 - 1)
            {
                *xdist = 0;
                // nothing..
            } else if (interpol(src.first, srcAlpha, sx, sy, tempval)) {
                *xd = tempval;
                *xdist = 255;
            } else {
                *xdist = 0;
            }
        }
        if ((y-ystart)%100 == 0) {
            prog.setProgress(((double)y-ystart)/(yend-ystart));
        }
    }
    prog.popTask();
}


/** Transform an image into the panorama
 *
 *  It can be used for partial transformations as well, if the boundig
 *  box of a remapped image is known.
 *
 *  Usage: create an output image @dest that should contain the remapped
 *         @p src image. if @p dest doesn't cover the whole output panorama,
 *         use @p destUL to specify the offset of @p dest from the output
 *         panorama.
 *
 *  @param src    source image
 *  @param dest   (partial) panorama image. the image size needed to
 *                hold the complete remapped image can be calculated using
 *                calcBorderPoints().
 *  @param destUL upper left point of @p dest in final panorama. set to (0,0)
 *                if @p dest has the same size as the complete panorama.
 *  @param transform function used to remap the picture.
 *  @param centerDist image, with the same size as dest, that will contain the
 *                distance of the corrosponding pixel from the center of @p
 *                src. This is useful to calculate nice seams. Use a null
 *                image if this information is not needed.
 *  @param interp Interpolator class (calculates weights for interpolation)
 *
 */
template <class SrcImageIterator, class SrcAccessor,
          class DestImageIterator, class DestAccessor,
          class TRANSFORM,
          class DistImageIterator, class DistAccessor,
          class Interpolator>
void transformImageDist(vigra::triple<SrcImageIterator, SrcImageIterator, SrcAccessor> src,
                        vigra::triple<DestImageIterator, DestImageIterator, DestAccessor> dest,
                        vigra::Diff2D destUL,
                        TRANSFORM & transform,
                        vigra::triple<DistImageIterator, DistImageIterator, DistAccessor> centerDist,
                        Interpolator & interp,
                        utils::MultiProgressDisplay & prog)
{
    vigra::Diff2D destSize = dest.second - dest.first;
    vigra::Diff2D distSize = centerDist.second - centerDist.first;

    bool calcDist=true;
    if (distSize.x == 0 && distSize.y == 0) {
        calcDist=false;
    }

    if (calcDist) {
        DEBUG_ASSERT(distSize == destSize);
    }
    int xstart = destUL.x;
    int xend   = destUL.x + destSize.x;
    int ystart = destUL.y;
    int yend   = destUL.y + destSize.y;

    prog.pushTask(utils::ProgressTask("Remapping", "", 1.0/(yend-ystart)));

    vigra::Diff2D srcSize = src.second - src.first;
    // FIXME: use d & e here.
    vigra::Diff2D srcMiddle = srcSize / 2;

//    vigra::BilinearInterpolatingAccessor<SrcAccessor, typename SrcAccessor::value_type> interpol(src.third);

    //InterpolatingAccessor(src.third, interp);
    vigra_ext::InterpolatingAccessor<SrcAccessor,
                          typename SrcAccessor::value_type,
                          Interpolator> interpol(src.third, interp);


//    vigra::BilinearInterpolatingAccessor<SrcAccessor, typename SrcAccessor::value_type> interpol(src.third);

//    vigra::BilinearInterpolatingAccessor interpol(src.third);

    // create dest y iterator
    DestImageIterator yd(dest.first);
    // create dist y iterator
    DistImageIterator ydist(centerDist.first);
    // loop over the image and transform
    for(int y=ystart; y < yend; ++y, ++yd.y, ++ydist.y)
    {
        // create x iterators
        DestImageIterator xd(yd);
        DistImageIterator xdist(ydist);
        for(int x=xstart; x < xend; ++x, ++xd.x, ++xdist.x)
        {
            double sx,sy;
            transform.transformImgCoord(sx,sy,x,y);
            // make sure that the interpolator doesn't
            // access pixels outside.. Should we introduce
            // some sort of border treatment?
            if (sx < interp.size/2 -1
                || sx > srcSize.x-interp.size/2 - 1
                || sy < interp.size/2 - 1
                || sy > srcSize.y-interp.size/2 - 1)
            {
                if (calcDist) {
                    // save an invalid distance
                    *xdist = FLT_MAX;
                }
                // nothing..
            } else {
//                cout << x << "," << y << " -> " << sx << "," << sy << " " << std::endl;

//                nearest neighbour
//                *xd = src.third(src.first, vigra::Diff2D((int)round(sx), (int)round(sy)));
                // use given interpolator function.
                *xd = interpol(src.first, sx, sy);
                if (calcDist) {
                    double mx = sx - srcMiddle.x;
                    double my = sy - srcMiddle.y;
                    *xdist = sqrt(mx*mx + my*my);
                }
            }
        }
        if ((y-ystart)%100 == 0) {
            prog.setProgress(((double)y-ystart)/(yend-ystart));
        }
    }
    prog.popTask();
}


/** Transform an image into the panorama
 *
 *  It can be used for partial transformations as well, if the boundig
 *  box of a remapped image is known.
 *
 *  Usage: create an output image @dest that should contain the remapped
 *         @p src image. if @p dest doesn't cover the whole output panorama,
 *         use @p destUL to specify the offset of @p dest from the output
 *         panorama.
 *
 *  @param src    source image
 *  @param dest   (partial) panorama image. the image size needed to
 *                hold the complete remapped image can be calculated using
 *                calcBorderPoints().
 *  @param destUL upper left point of @p dest in final panorama. set to (0,0)
 *                if @p dest has the same size as the complete panorama.
 *  @param transform function used to remap the picture.
 *  @param centerDist image, with the same size as dest, that will contain the
 *                distance of the corrosponding pixel from the center of @p
 *                src. This is useful to calculate nice seams. Use a null
 *                image if this information is not needed.
 *  @param interpol Interpolation algorithm that should be used.
 *
 */
template <class SrcImageIterator, class SrcAccessor,
          class DestImageIterator, class DestAccessor,
          class AlphaImageIterator, class AlphaAccessor,
          class TRANSFORM>
void transformImage(vigra::triple<SrcImageIterator, SrcImageIterator, SrcAccessor> src,
                    vigra::triple<DestImageIterator, DestImageIterator, DestAccessor> dest,
                    std::pair<AlphaImageIterator, AlphaAccessor> alpha,
		    vigra::Diff2D destUL,
                    TRANSFORM & transform,
                    Interpolator interpol,
                    utils::MultiProgressDisplay & progress)
{
    switch (interpol) {
    case INTERP_CUBIC:
	DEBUG_DEBUG("using cubic interpolator");
	transformImageIntern(src, dest, alpha, transform, destUL,
                                 vigra_ext::interp_cubic(),
                                 progress);
	break;
    case INTERP_SPLINE_16:
	DEBUG_DEBUG("interpolator: spline16");
	transformImageIntern(src, dest, alpha, transform, destUL,
                                 vigra_ext::interp_spline16(),
                                 progress);
	break;
    case INTERP_SPLINE_36:
	DEBUG_DEBUG("interpolator: spline36");
	transformImageIntern(src, dest, alpha, transform, destUL,
                                 vigra_ext::interp_spline36(),
                                 progress);
	break;
    case INTERP_SPLINE_64:
	DEBUG_DEBUG("interpolator: spline64");
	transformImageIntern(src, dest, alpha, transform, destUL,
                                 vigra_ext::interp_spline64(),
                                 progress);
	break;
    case INTERP_SINC_256:
	DEBUG_DEBUG("interpolator: sinc 256");
	transformImageIntern(src, dest, alpha, transform, destUL,
                                 vigra_ext::interp_sinc<8>(),
                                 progress);
	break;
    case INTERP_BILINEAR:
	transformImageIntern(src, dest, alpha, transform, destUL,
                                 vigra_ext::interp_bilin(),
                                 progress);
	break;
    case INTERP_NEAREST_NEIGHBOUR:
	transformImageIntern(src, dest, alpha, transform, destUL,
                                 vigra_ext::interp_nearest(),
                                 progress);
	break;
    case INTERP_SINC_1024:
	transformImageIntern(src, dest, alpha, transform, destUL,
                                 vigra_ext::interp_sinc<32>(),
                                 progress);
	break;
    }
}

/** Transform image, and respect a possible alpha channel */
template <class SrcImageIterator, class SrcAccessor,
          class SrcAlphaIterator, class SrcAlphaAccessor,
          class DestImageIterator, class DestAccessor,
          class AlphaImageIterator, class AlphaAccessor,
          class TRANSFORM>
void transformImageAlpha(vigra::triple<SrcImageIterator, SrcImageIterator, SrcAccessor> src,
                         std::pair<SrcAlphaIterator, SrcAlphaAccessor> srcAlpha,
                         vigra::triple<DestImageIterator, DestImageIterator, DestAccessor> dest,
                         std::pair<AlphaImageIterator, AlphaAccessor> alpha,
                         vigra::Diff2D destUL,
                         TRANSFORM & transform,
                         Interpolator interpol,
                         utils::MultiProgressDisplay & progress)
{
    switch (interpol) {
    case INTERP_CUBIC:
	DEBUG_DEBUG("using cubic interpolator");
	transformImageAlphaIntern(src,srcAlpha, dest, alpha, transform, destUL,
				      vigra_ext::interp_cubic(),
				      progress);
	break;
    case INTERP_SPLINE_16:
	DEBUG_DEBUG("interpolator: spline16");
	transformImageAlphaIntern(src,srcAlpha, dest, alpha, transform, destUL,
				      vigra_ext::interp_spline16(),
				      progress);
	break;
    case INTERP_SPLINE_36:
	DEBUG_DEBUG("interpolator: spline36");
	transformImageAlphaIntern(src,srcAlpha, dest, alpha, transform, destUL,
				      vigra_ext::interp_spline36(),
				      progress);
	break;
    case INTERP_SPLINE_64:
	DEBUG_DEBUG("interpolator: spline64");
	transformImageAlphaIntern(src,srcAlpha, dest, alpha, transform, destUL,
				      vigra_ext::interp_spline64(),
				      progress);
	break;
    case INTERP_SINC_256:
	DEBUG_DEBUG("interpolator: sinc 256");
	transformImageAlphaIntern(src,srcAlpha, dest, alpha, transform, destUL,
				      vigra_ext::interp_sinc<8>(),
				      progress);
	break;
    case INTERP_BILINEAR:
	transformImageAlphaIntern(src,srcAlpha, dest, alpha, transform, destUL,
				      vigra_ext::interp_bilin(),
				      progress);
	break;
    case INTERP_NEAREST_NEIGHBOUR:
	transformImageAlphaIntern(src,srcAlpha, dest, alpha, transform, destUL,
				      vigra_ext::interp_nearest(),
				      progress);
	break;
    case INTERP_SINC_1024:
	transformImageAlphaIntern(src,srcAlpha, dest, alpha, transform, destUL,
				      vigra_ext::interp_sinc<32>(),
				      progress);
	break;
    }
}

template <class T, int nr>
void fillVector(T vec[3], T &val, int len)
{
    for (int i=0; i<len; i++) vec[i] = val;
}

}; // namespace

#endif // _IMAGETRANSFORMS_H
