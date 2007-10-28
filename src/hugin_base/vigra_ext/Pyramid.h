// -*- c-basic-offset: 4 -*-
/** @file Pyramid.h
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

#ifndef VIGRA_EXT_PYRAMID_H
#define VIGRA_EXT_PYRAMID_H

#include <vigra/separableconvolution.hxx>

#include <hugin_utils/utils.h>


// TODO: use fast version from enblend 3.0
#include <vigra_ext/pyramid2.h>


namespace vigra_ext {

template <class ImageIn, class Image>
void reduceNTimes(ImageIn & in, Image & out, int n)
{
    typedef typename Image::value_type vt;
    typedef typename vigra::NumericTraits<vt>::RealPromote SKIPSMType;
    if (n <= 0) {
        out = in;
        return;
    }

    size_t w = in.width();
    size_t h = in.height();
    // Size of next level
    w = (w + 1) >> 1;
    h = (h + 1) >> 1;

    Image temp;
    Image * curr = &temp;
    Image * next = &out;
    if( (n % 2) == 1)
    {
        // even number of reduce operations, first reduce into output image
        curr = &out;
        next = &temp;
    }
    curr->resize(w,h);
    enblend::reduce<SKIPSMType>(false, srcImageRange(in), destImageRange(*curr));
    n--;
    w = (w + 1) >> 1;
    h = (h + 1) >> 1;
    for( ; n > 0 ; --n) {
        next->resize(w,h);
        enblend::reduce<SKIPSMType>(false, srcImageRange(*curr), destImageRange(*next));
        w = (w + 1) >> 1;
        h = (h + 1) >> 1;
        Image * t = curr;
        curr = next;
        next = t;
    }
}

template <class Image, class ImageMask>
void reduceNTimes(Image & in, ImageMask & inMask, Image & out, ImageMask & outMask, int n)
{
    typedef typename Image::value_type vt;
    typedef typename vigra::NumericTraits<vt>::RealPromote SKIPSMType;
    typedef typename ImageMask::value_type mt;
//    typedef typename vigra::NumericTraits<mt>::Promote SKIPSMAlphaType;
    typedef double SKIPSMAlphaType;

    if (n <= 0) {
        out = in;
        outMask = inMask;
        return;
    }

    size_t w = in.width();
    size_t h = in.height();
    // Size of next level
    w = (w + 1) >> 1;
    h = (h + 1) >> 1;

    Image temp;
    ImageMask tempMask;
    Image * curr = &temp;
    ImageMask * currMask = &tempMask;
    Image * next = &out;
    ImageMask * nextMask = &outMask;
    if( (n % 2) == 1)
    {
        // even number of reduce operations, first reduce into output image
        curr = &out;
        currMask = &outMask;
        next = &temp;
        nextMask = &tempMask;
    }
    curr->resize(w,h);
    currMask->resize(w,h);
    enblend::reduce<SKIPSMType, SKIPSMAlphaType>(false, srcImageRange(in), srcImage(inMask), 
                                destImageRange(*curr), destImageRange(*currMask));
    n--;
    w = (w + 1) >> 1;
    h = (h + 1) >> 1;
    for( ; n > 0 ; --n) {
        next->resize(w,h);
        nextMask->resize(w,h);
        enblend::reduce<SKIPSMType, SKIPSMAlphaType>(false, srcImageRange(*curr), srcImage(*currMask),
                                    destImageRange(*next), destImageRange(*nextMask));
        w = (w + 1) >> 1;
        h = (h + 1) >> 1;
        Image * t = curr;
        ImageMask * tm = currMask;
        curr = next;
        currMask = nextMask;
        next = t;
        nextMask = tm;
    }
}

template <class ImageIn, class ImageOut>
void reduceToNextLevel(ImageIn & in, ImageOut & out)
{
    typedef typename ImageOut::value_type vt;
    typedef typename vigra::NumericTraits<vt>::RealPromote SKIPSMType;

    size_t w = in.width();
    size_t h = in.height();
    // Size of next level
    w = (w + 1) >> 1;
    h = (h + 1) >> 1;
    out.resize(w,h);
    enblend::reduce<SKIPSMType>(false, srcImageRange(in), destImageRange(out));
}

template <class ImageIn, class ImageInMask, class ImageOut, class ImageOutMask>
void reduceToNextLevel(ImageIn & in, ImageInMask & inMask, ImageOut & out, ImageOutMask & outMask)
{
    typedef typename ImageOut::value_type vt;
    typedef typename vigra::NumericTraits<vt>::RealPromote SKIPSMType;
    //typedef typename vigra::NumericTraits<typename ImageOutMask::value_type>::Promote SKIPSMAlphaType;
    typedef double SKIPSMAlphaType;

    size_t w = in.width();
    size_t h = in.height();
    // Size of next level
    w = (w + 1) >> 1;
    h = (h + 1) >> 1;
    out.resize(w,h);
    enblend::reduce<SKIPSMType, SKIPSMAlphaType>(false, srcImageRange(in), srcImage(inMask),
                                destImageRange(out), destImageRange(outMask));
}


static const double AA = 0.4;
static const double W[] = {0.25 - AA / 2.0, 0.25, AA, 0.25, 0.25 - AA / 2.0};

/** Gaussian reduction to next pyramid level
 *
 *  out is rescaled to the correct size.
 */
template <class Image>
void reduceToNextLevelOld(Image & in, Image & out)
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
    filter.initExplicitly(-2, 2) = W[0], W[1], W[2], W[3], W[4];

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

} // namespace


#endif // VIGRA_EXT_PYRAMID_H
