// -*- c-basic-offset: 4 -*-
/** @file Correlation.h
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

#ifndef VIGRA_EXT_CORRELATION_H
#define VIGRA_EXT_CORRELATION_H

#include <vigra/stdimage.hxx>
#include <vigra/inspectimage.hxx>
#include <vigra/copyimage.hxx>
#include <vigra/resizeimage.hxx>

#include "common/utils.h"
#include "common/math.h"
#include "vigra_ext/Pyramid.h"

namespace vigra_ext{

/** Maximum of correlation, position and value */
struct CorrelationResult
{
    CorrelationResult()
        : maxi(-1),maxpos(0,0)
        { }
    double maxi;
    FDiff2D maxpos;
};


/** fine tune a point with normalized cross correlation
 *
 *  takes a patch of \p templSize by \p templSize from \p templImg
 *  images at \p tmplPos and searches it on the \p searchImg, at
 *  \p searchPos, in a neighbourhood of \p sWidth by \p sWidth.
 *
 *  The result in returned in @p tunedPos
 *
 *  @return correlation value
 */
template <class IMAGE>
CorrelationResult PointFineTune(const IMAGE & templImg,
                                vigra::Diff2D templPos,
                                int templSize,
                                const IMAGE & searchImg,
                                vigra::Diff2D searchPos,
                                int sWidth)
{
//    DEBUG_TRACE("templPos: " vigra::<< templPos << " searchPos: " vigra::<< searchPos);

    // extract patch from template

    // make template size user configurable as well?
    int templWidth = templSize/2;
    vigra::Diff2D tmplUL(-templWidth, -templWidth);
    vigra::Diff2D tmplLR(templWidth, templWidth);
    // clip template
    if (tmplUL.x + templPos.x < 0) tmplUL.x = -templPos.x;
    if (tmplUL.y + templPos.y < 0) tmplUL.y = -templPos.y;
    if (tmplLR.x + templPos.x> templImg.width())
        tmplLR.x = templImg.width() - templPos.x;
    if (tmplLR.y + templPos.y > templImg.height())
        tmplLR.y = templImg.height() - templPos.y;

    // extract patch from search region
    int swidth = sWidth/2;
//    DEBUG_DEBUG("search window half width/height: " << swidth << "x" << swidth);
//    Diff2D subjPoint(searchPos);
    if (searchPos.x < 0) searchPos.x = 0;
    if (searchPos.x > (int) searchImg.width()) searchPos.x = searchImg.width()-1;
    if (searchPos.y < 0) searchPos.y = 0;
    if (searchPos.y > (int) searchImg.height()) searchPos.x = searchImg.height()-1;

    vigra::Diff2D searchUL(searchPos.x - swidth, searchPos.y - swidth);
    vigra::Diff2D searchLR(searchPos.x + swidth, searchPos.y + swidth);
    // clip search window
    if (searchUL.x < 0) searchUL.x = 0;
    if (searchUL.x > searchImg.width()) searchUL.x = searchImg.width();
    if (searchUL.y < 0) searchUL.y = 0;
    if (searchUL.y > searchImg.height()) searchUL.y = searchImg.height();
    if (searchLR.x > searchImg.width()) searchLR.x = searchImg.width();
    if (searchLR.x < 0) searchLR.x = 0;
    if (searchLR.y > searchImg.height()) searchLR.y = searchImg.height();
    if (searchLR.y < 0) searchLR.y = 0;
//    DEBUG_DEBUG("search borders: " << searchLR.x << "," << searchLR.y);

    vigra::Diff2D searchSize = searchLR - searchUL;
    // create output image

    vigra::FImage dest(searchSize);
    dest.init(1);
    // we could use the multiresolution version as well.
    // but usually the region is quite small.
    CorrelationResult res;
    res = correlateImage(searchImg.upperLeft() + searchUL,
                         searchImg.upperLeft() + searchLR,
                         searchImg.accessor(),
                         dest.upperLeft(),
                         dest.accessor(),
                         templImg.upperLeft() + templPos,
                         templImg.accessor(),
                         tmplUL, tmplLR, -1);
    res.maxpos = res.maxpos + searchUL;
//    DEBUG_DEBUG("normal search finished, max:" << res.maxi
//                << " at " << res.maxpos);

    return res;
}

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
CorrelationResult correlateImage(SrcIterator sul, SrcIterator slr, SrcAccessor as,
                                 DestIterator dul, DestAccessor ad,
                                 KernelIterator ki, KernelAccessor ak,
                                 vigra::Diff2D kul, vigra::Diff2D klr,
                                 double threshold = 0.7 )
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

    // calculate width and height of the image
    int w = slr.x - sul.x;
    int h = slr.y - sul.y;
    int wk = klr.x - kul.x +1;
    int hk = klr.y - kul.y +1;

//    DEBUG_DEBUG("correlate Image srcSize " << (slr - sul).x << "," << (slr - sul).y
//                << " tmpl size: " << wk << "," << hk);

    vigra_precondition(w >= wk && h >= hk,
                 "convolveImage(): kernel larger than image.");

    int x,y;
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

    // create y iterators, they iterate over the rows.
    DestIterator yd = dul + vigra::Diff2D(xstart, ystart);
    SrcIterator ys = sul + vigra::Diff2D(xstart, ystart);


//    DEBUG_DEBUG("size: " << w << "," <<  h << " ystart: " << ystart <<", yend: " << yend);
    for(y=ystart; y < yend; ++y, ++ys.y, ++yd.y)
    {

        // create x iterators, they iterate the coorelation over
        // the columns
        DestIterator xd(yd);
        SrcIterator xs(ys);

        for(x=xstart; x < xend; ++x, ++xs.x, ++xd.x)
        {
            if (as(xd) < threshold) {
                continue;
            }
//            int x0, y0, x1, y1;

//            y0 = kul.y;
//            y1 = klr.y;
//            x0 = kul.x;
//            x1 = klr.x;;

            // init the sum
            SumType numerator = 0;
            SumType div1 = 0;
            SumType div2 = 0;
            SumType spixel = 0;
            KSumType kpixel = 0;

            // create inner y iterators
            // access to the source image
            SrcIterator yys = xs + kul;
            // access to the kernel image
            KernelIterator yk  = ki + kul;

            vigra::FindAverage<typename SrcAccessor::value_type> average;
            vigra::inspectImage(xs + kul, xs + klr, as, average);
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
                    kpixel = ak(xk) - kmean;
                    numerator += kpixel * spixel;
                    div1 += kpixel * kpixel;
                    div2 += spixel * spixel;
                }
            }
            numerator = (numerator/sqrt(div1 * div2));
            if (numerator > res.maxi) {
                res.maxi = numerator;
                res.maxpos.x = x;
                res.maxpos.y = y;
            }
            numerator = numerator;
            // store correlation in destination pixel
            ad.set(DestTraits::fromRealPromote(numerator), xd);
        }
    }
    return res;
}


#if 0
/** multi resolution template matching using cross correlatation.
 *
 */
template <class Image>
bool findTemplate(const Image & templ, const std::string & filename,
                  CorrelationResult & res)
{
//    DEBUG_TRACE("");
    // calculate pyramid level based on template size
    // the template should have be at least 1 pixel wide or high
    // (in the lowest resolution).
    int tw = templ.width();
    int th = templ.height();
    int ts = th<tw ? th : tw;

    // calculate the lowest level that will make sense
    int maxLevel = (int)floor(sqrt((double)ts/4.0));
    int levels = maxLevel+1;
    DEBUG_DEBUG("starting search on pyramid level " << maxLevel);

	vigra::BImage *templs = new vigra::BImage[levels];
    templs[0].resize(templ.width(), templ.height());
    vigra::copyImage(srcImageRange(templ), destImage(templs[0]));

    // create other levels
    for(int i=1; i<levels; i++) {
        vigra_ext::reduceToNextLevel(templs[i-1], templs[i]);
    }

    // this image will store the correlation coefficients
    // it will also be used by correlateImage as a mask image.
    // only the correlation for pixels with resImage(x,y) > theshold
    // will be calcuated. this avoids searching in completely uninteresting
    // image parts. (saves a lot of time)

    vigra::FImage * prevMask = new vigra::FImage(2,2,0.9);
    vigra::FImage * currentMask = new vigra::FImage();
    // start matching with highest level (lowest resolution images)
    float threshold=0;
    for (int i=maxLevel; i>=0; i--) {
//    for (int i=0; i>=0; i--) {
        DEBUG_DEBUG("correlation level " << i);
        std::stringstream t;
        t << filename << "_" << i << "_";
        std::string basename = t.str();
//        const vigra::BImage & s = ImageCache::getInstance().getPyramidImage(filename, i);
        DEBUG_DEBUG("subject size: " << s.width() << "," << s.height()
                    << "template size: " << templs[i].width() << "," << templs[i].height());
//        saveImage(templs[i], basename + "template.pnm");
//        saveImage(s, basename + "subject.pnm");
        currentMask->resize(s.width(),s.height());
        // scale up dest/mask Image
        // probably this should also use a threshold and extend the
        // resulting mask to the neighborhood pixels, just to catch unfortunate
        // correlations.
        vigra::resizeImageNoInterpolation(vigra::srcImageRange(*prevMask),vigra::destImageRange(*currentMask));

//        saveScaledImage(*currentMask, -1, 1, basename + "mask.pnm");

        res = correlateImage(s.upperLeft(),
                             s.lowerRight(),
                             vigra::StandardValueAccessor<unsigned char>(),
                             currentMask->upperLeft(),
                             vigra::StandardValueAccessor<float>(),
                             templs[i].upperLeft(),
                             vigra::StandardValueAccessor<unsigned char>(),
                             vigra::Diff2D(0,0),
                             templs[i].size() - vigra::Diff2D(1,1),
                             threshold
            );
//        saveScaledImage(*currentMask, -1, 1, basename + "result.pnm");
        DEBUG_DEBUG("Correlation result at level " << i << ":  max:" << res.maxi << " at: " << res.maxpos);
        // simple adaptive threshold.
        // FIXME, make this configurable? or select it according to some
        // statistical value
        threshold = res.maxi - 0.07;

        vigra::FImage *tmp = prevMask;
        prevMask = currentMask;
        currentMask = tmp;
    }
    delete prevMask;
    delete currentMask;
    return true;
}
#endif


} // namespace

#endif // VIGRA_EXT_CORRELATION_H
