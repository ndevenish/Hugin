// -*- c-basic-offset: 4 -*-
/** @file ImageProcessing.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  Misc image processing functions. Needed for control point search
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

#ifndef _IMAGEPROCESSING_H
#define _IMAGEPROCESSING_H

#include "hugin/ImageCache.h"

class wxImage;

#if 0

/** correlate a template with an image.
 *
 *  most code is taken from vigra::convoluteImage.
 *  See its documentation for further information.
 *
 *  Correlation result already contains the maximum position
 *  and its correlation value.
 *  it should be possible to set a threshold here.
 */
template <class SrcIterator, class SrcAccessor,
          class DestIterator, class DestAccessor,
          class KernelIterator, class KernelAccessor>
CorrelationResult correlateImage_new(SrcIterator sul, SrcIterator slr, SrcAccessor as,
                                     DestIterator dul, DestAccessor ad,
                                     KernelIterator ki, KernelAccessor ak,
                                     vigra::Diff2D kul, vigra::Diff2D klr,
                                     double threshold = 0.7
                                    )
{
    vigra_precondition(kul.x <= 0 && kul.y <= 0,
                 "convolveImage(): coordinates of "
                 "kernel's upper left must be <= 0.");
    vigra_precondition(klr.x >= 0 && klr.y >= 0,
                 "convolveImage(): coordinates of "
                 "kernel's lower right must be >= 0.");

    // use traits to determine SumType as to prevent possible overflow
    typedef typename
        vigra::NumericTraits<typename SrcAccessor::value_type>::RealPromote SumType;
    typedef typename
        vigra::NumericTraits<typename KernelAccessor::value_type>::RealPromote KSumType;
    typedef
        vigra::NumericTraits<typename DestAccessor::value_type> DestTraits;

    // calculate width and height of the source and kernel
    int w = slr.x - sul.x;
    int h = slr.y - sul.y;
    int wk = klr.x - kul.x +1;
    int hk = klr.y - kul.y +1;

    DEBUG_DEBUG("correlate Image srcSize " << (slr - sul).x << "," << (slr - sul).y
                << " tmpl size: " << wk << "," << hk)

    vigra_precondition(w >= wk && h >= hk,
                 "convolveImage(): kernel larger than image.");

    int ystart = -kul.y;
    int yend   = h-klr.y;
    int xstart = -kul.x;
    int xend   = w-klr.x;

    // calculate template mean
    vigra::FindAverage<typename KernelAccessor::value_type> average;
    vigra::inspectImage(ki + kul, ki + klr,
                        ak,
                        average);
    KSumType kmean = average();

    CorrelationResult res;


    // create the correlation center iterators
    DestIterator centerDest = dul + vigra::Diff2D(xstart, ystart);
    SrcIterator centerSrc = sul + vigra::Diff2D(xstart, ystart);


    DEBUG_DEBUG("size: " << w << "," <<  h << " ystart: " << ystart <<", yend: " << yend);
    for(centerDest.y = DestIterator::MoveY(0),
         centerSrc.y = SrcIterator::MoveY(0);
        centerDest.y < yend;
        ++centerDest.y, ++centerSrc.y)
    {
        std::cerr << centerDest.y << " " << std::flush;

        for(centerDest.x = DestIterator::MoveX(0),
             centerSrc.x = SrcIterator::MoveX(0);
            centerDest.x < xend;
            ++centerDest.x, ++centerSrc.x)
        {
            // inner loop, calculate correlation
            SumType numerator = 0;
            SumType div1 = 0;
            SumType div2 = 0;
            SumType spixel = 0;
            KSumType kpixel = 0;

            // create inner iterators
            // access to the source image
            SrcIterator src(centerSrc - kul);
            int sxstart = src.x;
            int systart = src.y;
            int sxend = sxstart + wk;
            int syend = systart + wk;

            // access to the kernel image
            KernelIterator kernel(ki - kul);
            int kxstart = kernel.x;
            int kystart = kernel.y;

            for(src.y = systart, kernel.y = kystart;
                src.y < syend;
                src.y++, kernel.y++)
            {
                for (src.x = sxstart, kernel.x = kxstart;
                     src.x < sxend;
                     src.x++, kernel.x++)
                {
                    spixel = as(src) - mean;
                    kpixel = ak(kernel) - kmean;
                    numerator += kpixel * spixel;
                    div1 += kpixel * kpixel;
                    div2 += spixel * spixel;
                }
            }
            numerator = (numerator/sqrt(div1 * div2));
            if (numerator > res.max) {
                res.max = numerator;
                res.pos.x = x;
                res.pos.y = y;
            }
            numerator = numerator;
            // store correlation in destination pixel
            ad.set(DestTraits::fromRealPromote(numerator), centerDest);
        }
    }
    return res;
}

#endif

/** fit a polynom of second order though the 3x3 neighbourhood.
 *
 *  x = a_1*s^2 + b_1*s + c_1
 *  y = a_2*t^2 + b_2*t + c_2
 *
 *
 *  y1 = ax1^2 + bx1 + c;
 *  y2 = ax2^2 + bx2 + c;
 *  y3 = ax3^2 + bx3 + c;
 *
 */


#if 0

// a subpixel correlation result
struct SubPixelCorrelationResult
{
    SubPixelCorrelationResult()
        : maxi(-1),pos(-1,-1)
        { }
    double maxi;
    FDiff2D pos;
};


    return r;
}

#endif



#endif // _IMAGEPROCESSING_H
