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

#include <vigra/numerictraits.hxx>
#include <vigra/imageiterator.hxx>
#include <vigra/stdimage.hxx>
#include <vigra/transformimage.hxx>
#include <vigra/copyimage.hxx>
#include <vigra/functorexpression.hxx>
#include <vigra/convolution.hxx>
#include <vigra/functorexpression.hxx>

#include "hugin/wxVigraImage.h"
#include "hugin/ImageCache.h"
#include "common/utils.h"

class wxImage;

typedef vigra::ImageIterator<vigra::RGBValue<unsigned char> > wxImageIterator;



/// create vigra iterators for wxImage
wxImageIterator wxImageUpperLeft(wxImage & img);

/// create vigra iterators for wxImage
wxImageIterator wxImageLowerRight(wxImage & img);

struct NormalizeToUChar
{
    NormalizeToUChar(double max, double min)
        : min(min), scale(255/(max - min))
        {}

        template <class PixelType>
        unsigned char operator()(PixelType const& v) const
        {
            float res = (v - min)*scale;
            if (res > 255) {
                DEBUG_ERROR("overflow:" << res);
                res = 255;
            }
            if (res < 0) {
                DEBUG_ERROR("underflow" << res);
                res = 0;
            }
            return (unsigned char)res;
        }
    double min;
    double scale;
};


/** save an image
 *
 *  so that all values in the range [@p min @p max] are mapped to
 *  [0 255]
 */
template<typename Image>
bool saveScaledImage(const Image &img, typename Image::PixelType min,
                        typename Image::PixelType max,
                        const std::string & filename)
{
    using namespace vigra;
    using namespace vigra::functor;

    DEBUG_DEBUG(filename << " min: " << min << " max:" << max);

    wxImage wxI(img.width(), img.height());
    wxVigraImage ii(wxI);
    NormalizeToUChar scale(min, max);
    transformImage(srcImageRange(img),
                   destImage(ii),
                   scale);

//    transformImage(img.upperLeft(), img.lowerRight(),
//                   typename Image::Accessor(),
//                   wxImageUpperLeft(wxI),
//                   RGBAccessor<vigra::RGBValue<unsigned char> >(),
//                   (Arg1() - Param(min))*Param(scale));

    if (!wxI.SaveFile(filename.c_str())) {
        DEBUG_ERROR("Saving of " << filename << " failed");
        return false;
    }
    return true;
}

template<typename Image>
bool saveImage(const Image &img, const std::string & filename)
{
    DEBUG_DEBUG(filename);
    wxImage wxI(img.width(), img.height());
    vigra::copyImage(img.upperLeft(), img.lowerRight(),
                     vigra::BImage::Accessor(),
                     wxImageUpperLeft(wxI),
                     vigra::RGBAccessor<vigra::RGBValue<unsigned char> >());

    if (!wxI.SaveFile(filename.c_str())) {
        DEBUG_ERROR("Saving of " << filename << " failed");
        return false;
    }
    return true;
}

// template functions have to be in the header..

struct CorrelationResult
{
    CorrelationResult()
        : max(-1),pos(-1,-1)
        { }
    double max;
    vigra::Diff2D pos;
};

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
    vigra::inspectImage(ki + kul, ki + klr,
                        ak,
                        average);
    KSumType kmean = average();

    CorrelationResult res;

    // create y iterators
    DestIterator yd = dul + vigra::Diff2D(xstart, ystart);
    SrcIterator ys = sul + vigra::Diff2D(xstart, ystart);


    for(y=ystart; y < yend; ++y, ++ys.y, ++yd.y)
    {
        std::cerr << y << " " << std::flush;
        // create x iterators
        DestIterator xd(yd);
        SrcIterator xs(ys);

        for(x=xstart; x < xend; ++x, ++xs.x, ++xd.x)
        {
            if (as(xd) < threshold) {
                continue;
            }
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
            vigra::inspectImage(yys, yys + vigra::Diff2D(wk, hk), as, average);
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
            if (numerator > res.max) {
                res.max = numerator;
                res.pos.x = x;
                res.pos.y = y;
            }
            numerator = numerator;
            // store correlation in destination pixel
            ad.set(DestTraits::fromRealPromote(numerator), xd);
        }
    }
    return res;
}


/** multi resolution template matching using cross correlatation.
 *
 */

template <class Image>
bool findTemplate(const Image & templ, const std::string & filename,
                  CorrelationResult & res)
{
    DEBUG_TRACE("")
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

    vigra::BImage templs[levels];
    templs[0].resize(templ.width(), templ.height());
    vigra::copyImage(srcImageRange(templ), destImage(templs[0]));

    // create other levels
    for(int i=1; i<levels; i++) {
        reduceToNextLevel(templs[i-1], templs[i]);
    }

    // this image will store the correlation coefficient
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
        const vigra::BImage & s = ImageCache::getInstance().getPyramidImage(filename, i);
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
                             templs[i].size(),
                             threshold
            );
//        saveScaledImage(*currentMask, -1, 1, basename + "result.pnm");
        DEBUG_DEBUG("Correlation result at level " << i << ":  max:" << res.max << " at: " << res.pos.x << "," << res.pos.y);
        // simple adaptive threshold.
        // FIXME, make this configurable? or select it according to some
        // statistical value
        threshold = res.max - 0.07;

        vigra::FImage *tmp = prevMask;
        prevMask = currentMask;
        currentMask = tmp;
    }
    delete prevMask;
    delete currentMask;
    return true;
}

/** Gaussian reduction to next pyramid level
 *
 *  out is rescaled to the correct size.
 */
template <class Image>
void reduceToNextLevel(Image & in, Image & out)
{
    DEBUG_TRACE("");
    // image size at current level
    int width = in.width();
    int height = in.height();

    // image size at next smaller level
    int newwidth = (width + 1) / 2;
    int newheight = (height + 1) / 2;

    // resize result image to appropriate size
    out.resize(newwidth, newheight);

    // define a Gaussian kernel (size 5x1)
    // with sigma = 1
    vigra::Kernel1D<double> filter;
    filter.initExplicitly(-2, 2) = 0.054, 0.242, 0.4, 0.242, 0.054;

    vigra::BasicImage<typename Image::value_type> tmpimage1(width, height);
    vigra::BasicImage<typename Image::value_type> tmpimage2(width, height);

    // smooth (band limit) input image
    separableConvolveX(srcImageRange(in),
                       destImage(tmpimage1), kernel1d(filter));
    separableConvolveY(srcImageRange(tmpimage1),
                       destImage(tmpimage2), kernel1d(filter));

    // downsample smoothed image
    resizeImageNoInterpolation(srcImageRange(tmpimage2), destImageRange(out));

}


#endif // _IMAGEPROCESSING_H
