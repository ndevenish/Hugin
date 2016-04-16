// -*- c-basic-offset: 4 -*-
/** @file ImageTransforms.h
 *
 *  Contains functions to transform whole images.
 *  Can use PTools::Transform or PanoCommand::SpaceTransform for the calculations
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
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _VIGRA_EXT_IMAGETRANSFORMS_H
#define _VIGRA_EXT_IMAGETRANSFORMS_H

#include <fstream>

#include <vigra/basicimage.hxx>
#include <vigra_ext/ROIImage.h>
#include <vigra_ext/Interpolators.h>

#include <hugin_math/hugin_math.h>
#include <hugin_utils/utils.h>
#include <appbase/ProgressDisplay.h>

namespace vigra_ext
{

/** Set negative elements of a pixel to zero */
template <class T>
T zeroNegative(T p)
{
    if (p < 0) {
	return vigra::NumericTraits<T>::zero();
    } else {
	return p;
    }
}

/** Set negative elements of a pixel to zero */
template <class T>
vigra::RGBValue<T> zeroNegative(vigra::RGBValue<T> p)
{
    if (p.red() < 0) p.setRed(vigra::NumericTraits<T>::zero());
    if (p.green() < 0) p.setGreen(vigra::NumericTraits<T>::zero());
    if (p.blue() < 0) p.setBlue(vigra::NumericTraits<T>::zero());
    return p;
}


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
          class PixelTransform,
          class AlphaImageIterator, class AlphaAccessor,
          class Interpolator>
void transformImageIntern(vigra::triple<SrcImageIterator, SrcImageIterator, SrcAccessor> src,
                          vigra::triple<DestImageIterator, DestImageIterator, DestAccessor> dest,
                          std::pair<AlphaImageIterator, AlphaAccessor> alpha,
                          TRANSFORM & transform,
                          PixelTransform & pixelTransform,
                          vigra::Diff2D destUL,
                          Interpolator interp,
                          bool warparound,
                          AppBase::ProgressDisplay* progress,
                          bool singleThreaded)
{
    const vigra::Diff2D destSize = dest.second - dest.first;

    const int xstart = destUL.x;
    const int xend = destUL.x + destSize.x;
    const int ystart = destUL.y;
    const int yend = destUL.y + destSize.y;

    vigra_ext::ImageInterpolator<SrcImageIterator, SrcAccessor, Interpolator>
        interpol(src, interp, warparound);

    // loop over the image and transform
#pragma omp parallel for if(!singleThreaded) schedule(dynamic) 
    for (int y = ystart; y < yend; ++y)
    {
        // create x iterators
        DestImageIterator xd(dest.first);
        xd.y += y - ystart;
        AlphaImageIterator xdm(alpha.first);
        xdm.y += y - ystart;
        typename SrcAccessor::value_type tempval;
        for (int x = xstart; x < xend; ++x, ++xd.x, ++xdm.x)
        {
            double sx, sy;
            if (transform.transformImgCoord(sx, sy, x, y)) {
                if (interpol.operator()(sx, sy, tempval)){
                    // apply pixel transform and write to output
                    dest.third.set(zeroNegative(pixelTransform(tempval, hugin_utils::FDiff2D(sx, sy))), xd);
                    alpha.second.set(pixelTransform.hdrWeight(tempval, vigra::UInt8(255)), xdm);
                }
                else {
                    alpha.second.set(0, xdm);
                }
            }
            else {
                alpha.second.set(0, xdm);
            }
        }
    }
}

/** transform input images with alpha channel */
template <class SrcImageIterator, class SrcAccessor,
          class SrcAlphaIterator, class SrcAlphaAccessor,
          class DestImageIterator, class DestAccessor,
          class TRANSFORM,
          class PixelTransform,
          class AlphaImageIterator, class AlphaAccessor,
          class Interpolator>
void transformImageAlphaIntern(vigra::triple<SrcImageIterator, SrcImageIterator, SrcAccessor> src,
                               std::pair<SrcAlphaIterator, SrcAlphaAccessor> srcAlpha,
                               vigra::triple<DestImageIterator, DestImageIterator, DestAccessor> dest,
                               std::pair<AlphaImageIterator, AlphaAccessor> alpha,
                               TRANSFORM & transform,
                               PixelTransform & pixelTransform,
                               vigra::Diff2D destUL,
                               Interpolator interp,
                               bool warparound,
                               AppBase::ProgressDisplay* progress,
                               bool singleThreaded)
{
    const vigra::Diff2D destSize = dest.second - dest.first;

    const int xstart = destUL.x;
    const int xend   = destUL.x + destSize.x;
    const int ystart = destUL.y;
    const int yend   = destUL.y + destSize.y;

    vigra_ext::ImageMaskInterpolator<SrcImageIterator, SrcAccessor, SrcAlphaIterator,
                                     SrcAlphaAccessor, Interpolator>
                                    interpol (src, srcAlpha, interp, warparound);

    // loop over the image and transform
#pragma omp parallel for if(!singleThreaded) schedule(dynamic)
    for(int y=ystart; y < yend; ++y)
    {
        // create x iterators
        DestImageIterator xd(dest.first);
        xd.y += y - ystart;
        AlphaImageIterator xdist(alpha.first);
        xdist.y += y - ystart;
        typename SrcAccessor::value_type tempval;
        typename SrcAlphaAccessor::value_type alphaval;
        for (int x = xstart; x < xend; ++x, ++xd.x, ++xdist.x)
        {
            double sx,sy;
            if (transform.transformImgCoord(sx,sy,x,y)) {
                // try to interpolate.
                if (interpol(sx, sy, tempval, alphaval)) {
                    dest.third.set(zeroNegative(pixelTransform(tempval, hugin_utils::FDiff2D(sx, sy))), xd);
                    alpha.second.set(pixelTransform.hdrWeight(tempval, alphaval), xdist);
                } else {
                    // point outside of image or mask
                    alpha.second.set(0, xdist);
                }
            } else {
                alpha.second.set(0, xdist);
            }
        }
    }
};

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
          class TRANSFORM,
          class PixelTransform>
void transformImage(vigra::triple<SrcImageIterator, SrcImageIterator, SrcAccessor> src,
                    vigra::triple<DestImageIterator, DestImageIterator, DestAccessor> dest,
                    std::pair<AlphaImageIterator, AlphaAccessor> alpha,
                    vigra::Diff2D destUL,
                    TRANSFORM & transform,
                    PixelTransform & pixelTransform,
                    bool warparound,
                    Interpolator interpol,
                    AppBase::ProgressDisplay* progress, bool singleThreaded = false)
{
    switch (interpol) {
    case INTERP_CUBIC:
	DEBUG_DEBUG("using cubic interpolator");
    transformImageIntern(src, dest, alpha, transform, pixelTransform, destUL,
                                 vigra_ext::interp_cubic(), warparound,
                                 progress, singleThreaded);
	break;
    case INTERP_SPLINE_16:
	DEBUG_DEBUG("interpolator: spline16");
    transformImageIntern(src, dest, alpha, transform, pixelTransform, destUL,
                                 vigra_ext::interp_spline16(), warparound,
                                 progress, singleThreaded);
	break;
    case INTERP_SPLINE_36:
	DEBUG_DEBUG("interpolator: spline36");
    transformImageIntern(src, dest, alpha, transform, pixelTransform, destUL,
                                 vigra_ext::interp_spline36(), warparound,
                                 progress, singleThreaded);
	break;
    case INTERP_SPLINE_64:
	DEBUG_DEBUG("interpolator: spline64");
    transformImageIntern(src, dest, alpha, transform, pixelTransform, destUL,
                                 vigra_ext::interp_spline64(), warparound,
                                 progress, singleThreaded);
	break;
    case INTERP_SINC_256:
	DEBUG_DEBUG("interpolator: sinc 256");
    transformImageIntern(src, dest, alpha, transform, pixelTransform, destUL,
                                 vigra_ext::interp_sinc<8>(), warparound,
                                 progress, singleThreaded);
	break;
    case INTERP_BILINEAR:
        transformImageIntern(src, dest, alpha, transform, pixelTransform, destUL,
                                 vigra_ext::interp_bilin(), warparound,
                                 progress, singleThreaded);
	break;
    case INTERP_NEAREST_NEIGHBOUR:
        transformImageIntern(src, dest, alpha, transform, pixelTransform, destUL,
                                 vigra_ext::interp_nearest(), warparound,
                                 progress, singleThreaded);
	break;
    case INTERP_SINC_1024:
        transformImageIntern(src, dest, alpha, transform, pixelTransform, destUL,
                                 vigra_ext::interp_sinc<32>(), warparound,
                                 progress, singleThreaded);
	break;
    }
}

/** Transform image, and respect a possible alpha channel */
template <class SrcImageIterator, class SrcAccessor,
          class SrcAlphaIterator, class SrcAlphaAccessor,
          class DestImageIterator, class DestAccessor,
          class AlphaImageIterator, class AlphaAccessor,
          class TRANSFORM, class PixelTransform>
void transformImageAlpha(vigra::triple<SrcImageIterator, SrcImageIterator, SrcAccessor> src,
                         std::pair<SrcAlphaIterator, SrcAlphaAccessor> srcAlpha,
                         vigra::triple<DestImageIterator, DestImageIterator, DestAccessor> dest,
                         std::pair<AlphaImageIterator, AlphaAccessor> alpha,
                         vigra::Diff2D destUL,
                         TRANSFORM & transform,
                         PixelTransform & pixelTransform,
                         bool warparound,
                         Interpolator interpol,
                         AppBase::ProgressDisplay* progress, bool singleThreaded = false)
{
    switch (interpol) {
    case INTERP_CUBIC:
	DEBUG_DEBUG("using cubic interpolator");
	transformImageAlphaIntern(src,srcAlpha, dest, alpha, transform, pixelTransform, destUL,
				              vigra_ext::interp_cubic(), warparound,
                              progress, singleThreaded);
	break;
    case INTERP_SPLINE_16:
	DEBUG_DEBUG("interpolator: spline16");
    transformImageAlphaIntern(src,srcAlpha, dest, alpha, transform, pixelTransform, destUL,
                              vigra_ext::interp_spline16(), warparound,
                              progress, singleThreaded);
	break;
    case INTERP_SPLINE_36:
	DEBUG_DEBUG("interpolator: spline36");
    transformImageAlphaIntern(src,srcAlpha, dest, alpha, transform, pixelTransform, destUL,
                              vigra_ext::interp_spline36(),  warparound,
                              progress, singleThreaded);
	break;
    case INTERP_SPLINE_64:
	DEBUG_DEBUG("interpolator: spline64");
    transformImageAlphaIntern(src,srcAlpha, dest, alpha, transform, pixelTransform, destUL,
                              vigra_ext::interp_spline64(),  warparound,
                              progress, singleThreaded);
	break;
    case INTERP_SINC_256:
	DEBUG_DEBUG("interpolator: sinc 256");
    transformImageAlphaIntern(src,srcAlpha, dest, alpha, transform, pixelTransform, destUL,
                              vigra_ext::interp_sinc<8>(), warparound,
                              progress, singleThreaded);
	break;
    case INTERP_BILINEAR:
        transformImageAlphaIntern(src,srcAlpha, dest, alpha, transform, pixelTransform, destUL,
                              vigra_ext::interp_bilin(), warparound,
                              progress, singleThreaded);
	break;
    case INTERP_NEAREST_NEIGHBOUR:
        transformImageAlphaIntern(src,srcAlpha, dest, alpha, transform, pixelTransform, destUL,
                              vigra_ext::interp_nearest(), warparound,
                              progress, singleThreaded);
	break;
    case INTERP_SINC_1024:
        transformImageAlphaIntern(src,srcAlpha, dest, alpha, transform, pixelTransform, destUL,
                              vigra_ext::interp_sinc<32>(), warparound,
                              progress, singleThreaded);
	break;
    }
}

}; // namespace

#endif // _VIGRA_EXT_IMAGETRANSFORMS_H
