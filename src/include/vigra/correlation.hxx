// -*- c-basic-offset: 4 -*-

/** @file TemplateMatching.cpp
 *
 *  @brief implementation of TemplateMatching Class
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
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

//#include "PT/Correlation.h"
#include <cmath>
#include <vigra/imageiterator.hxx>
#include <vigra/stdimage.hxx>
#include <vigra/transformimage.hxx>
#include "vigra/bordertreatment.hxx"
#include "vigra/inspectimage.hxx"
#include "vigra/accessor.hxx"

#include "common/utils.h"

namespace vigra {

    
struct CorrelationResult
{
    CorrelationResult()
        : max(-1), xMax(-1), yMax(-1)
        { }
    double max;
    double xMax;
    double yMax;
};


/** correlate a template with an image.
 *
 *  most code is taken from vigra::convoluteImage.
 *  See its documentation for further information
 */
template <class SrcIterator, class SrcAccessor,
          class DestIterator, class DestAccessor,
          class KernelIterator, class KernelAccessor>
CorrelationResult correlateImage(SrcIterator sul, SrcIterator slr, SrcAccessor as,
                    DestIterator dul, DestAccessor ad,
                    KernelIterator ki, KernelAccessor ak,
                    vigra::Diff2D kul, vigra::Diff2D klr)
{
    vigra_precondition(kul.x <= 0 && kul.y <= 0,
                 "convolveImage(): coordinates of "
                 "kernel's upper left must be <= 0.");
    vigra_precondition(klr.x >= 0 && klr.y >= 0,
                 "convolveImage(): coordinates of "
                 "kernel's lower right must be >= 0.");

    // use traits to determine SumType as to prevent possible overflow
    typedef typename
        NumericTraits<typename SrcAccessor::value_type>::RealPromote SumType;
    typedef typename
        NumericTraits<typename KernelAccessor::value_type>::RealPromote KSumType;
    typedef
        NumericTraits<typename DestAccessor::value_type> DestTraits;

    // calculate width and height of the image
    int w = slr.x - sul.x;
    int h = slr.y - sul.y;
    int wk = klr.x - kul.x;
    int hk = klr.y - kul.y;

    vigra_precondition(w >= wk && h >= hk,
                 "convolveImage(): kernel larger than image.");

    int x,y;
    int ystart = -kul.y;
    int yend   = h-klr.y;
    int xstart = -kul.x;
    int xend   = w-klr.x;

    // calculate template mean
    vigra::FindAverage<vigra::BImage::PixelType> average;
//    vigra::inspectImage(ki + kul, ki + klr,
//                        vigra::StandardValueAccessor<unsigned char>,
//                        average);
    vigra::inspectImage(ki + kul, ki + klr,
                        ak,
                        average);
    KSumType kmean = average();
    
    CorrelationResult res;
    
    // create y iterators
    DestIterator yd = dul + Diff2D(xstart, ystart);
    SrcIterator ys = sul + Diff2D(xstart, ystart);

    
    for(y=ystart; y < yend; ++y, ++ys.y, ++yd.y)
    {
        std::cerr << y << std::endl;
        // create x iterators
        DestIterator xd(yd);
        SrcIterator xs(ys);

        for(x=xstart; x < xend; ++x, ++xs.x, ++xd.x)
        {
            int x0, y0, x1, y1;

            y0 = kul.y;
            y1 = klr.y;
            x0 = kul.x;
            x1 = klr.x;;

            // init the sum
            SumType numerator = 0;
            SumType div1 = 0;
            SumType div2 = 0;
            SumType spixel = 0;
            KSumType kpixel = 0;

            // create inner y iterators
            SrcIterator yys = xs + kul;
            KernelIterator yk  = ki + kul;

            vigra::FindAverage<vigra::BImage::PixelType> average;
            vigra::inspectImage(yys, yys + Diff2D(wk, hk), as, average);
            double mean = average();

            int xx, yy;
            for(yy=0; yy<hk; ++yy, ++yys.y, ++yk.y)
            {
                // create inner x iterators
                SrcIterator xxs = yys;
                KernelIterator xk = yk;

                for(xx=0; xx<wk; ++xx, ++xxs.x, ++xk.x)
                {
                    spixel = as(xxs) - mean;
                    kpixel = as(xk) - kmean;
                    numerator += kpixel * spixel;
                    div1 += kpixel * kpixel;
                    div2 += spixel * spixel;
                }
            }
            numerator = (numerator/sqrt(div1 * div2)) * 127 + 127;
            if (numerator > res.max) {
                res.max = numerator;
                res.xMax = x;
                res.yMax = y;
            }
            // store correlation in destination pixel
            ad.set(DestTraits::fromRealPromote(numerator), xd);
        }
    }
    return res;
}

}  // namespace
