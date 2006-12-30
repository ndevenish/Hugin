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

namespace vigra_ext {

static const double AA = 0.4;
static const double W[] = {0.25 - AA / 2.0, 0.25, AA, 0.25, 0.25 - AA / 2.0};

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
