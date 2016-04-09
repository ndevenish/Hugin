// -*- c-basic-offset: 4 -*-

/** @file BlendPoisson.h
*
*  @brief solve poisson equation for blending images
*
*
*  @author T. Modes
*
*/

/*  This program is free software; you can redistribute it and/or
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
*  License along with this software. If not, see
*  <http://www.gnu.org/licenses/>.
*
*/

#ifndef POISSON_BLEND_H
#define POISSON_BLEND_H

#include <iostream>
#include <vigra/stdimage.hxx>
#include <vigra/convolution.hxx>
#include <vigra/stdconvolution.hxx>
#include <vigra/basicgeometry.hxx>
#include "openmp_vigra.h"

#define DEBUG_TIMING

namespace vigra_ext
{

namespace poisson
{

namespace detail
{
// helper functions for build gradient map
template <class Image, class Mask>
inline typename vigra::NumericTraits<typename Image::PixelType>::RealPromote ProcessNeighborPixels(const int x, const int y, const int dx, const int dy, const Image& image, const Mask& mask)
{
    const typename Mask::PixelType m1 = mask[y + dy][x + dx];
    const typename Mask::PixelType m2 = mask[y - dy][x - dx];
    if (m1 > 0 && m2 > 0)
    {
        return image[y + dy][x + dx] + image[y - dy][x - dx];
    };
    // if one of the 2 pixel has no information (outside of image area)
    // we extrapolate the value from neighbor pixel
    if (m1 > 0)
    {
        return 2 * image[y + dy][x + dx];
    }
    else
    {
        return 2 * image[y - dy][x - dx];
    };
};

template <class Image, class Mask, class SeamMask>
inline typename vigra::NumericTraits<typename Image::PixelType>::RealPromote ProcessBorderPixel(const int x, const int y, const int dx, const int dy, const Image& image, const Mask& mask, const SeamMask& seam)
{
    const typename SeamMask::PixelType seam1 = seam[y + dy][x + dx];
    const typename SeamMask::PixelType seam2 = seam[y - dy][x - dx];
    const typename Mask::PixelType mask1 = mask[y + dy][x + dx];
    const typename Mask::PixelType mask2 = mask[y - dy][x - dx];
    if (seam1 > 0 && seam2 > 0)
    {
        if (mask1 > 0 && mask2 > 0)
        {
            return image[y + dy][x + dx] + image[y - dy][x - dx];
        };
        if (mask1 > 0)
        {
            return 2 *image[y + dy][x + dx];
        }
        else
        {
            return 2 * image[y - dy][x - dx];
        };
    };
    if (seam1 > 0)
    {
        if (mask1 > 0)
        {
            return 2 * image[y + dy][x + dx];
        }
        else
        {
            return vigra::NumericTraits<typename vigra::NumericTraits<typename Image::PixelType>::RealPromote>::zero();
        };
    };
    if (seam2 > 0)
    {
        if (mask2 > 0)
        {
            return 2 * image[y - dy][x - dx];
        }
        else
        {
            return vigra::NumericTraits<typename vigra::NumericTraits<typename Image::PixelType>::RealPromote>::zero();
        };
    };
    return vigra::NumericTraits<typename vigra::NumericTraits<typename Image::PixelType>::RealPromote>::zero();
};

template <class Image, class SeamMask>
inline typename Image::PixelType GetBorderGradient(const int x, const int y, const int dx, const int dy, const SeamMask& seams, const Image& image1, const vigra::Point2D& offset)
{
    if (seams[y + dy][x + dx] == 1)
    {
        return image1[offset.y + y + dy][offset.x + x + dx];
    };
    return vigra::NumericTraits<typename Image::PixelType>::zero();
};

template <class Image, class SeamMask>
inline typename vigra::NumericTraits<typename Image::PixelType>::RealPromote GetBorderValues(const int x, const int y, int dx, int dy, const Image& image, const SeamMask& seams)
{
    const typename SeamMask::PixelType s1 = seams[y + dy][x + dx];
    const typename SeamMask::PixelType s2 = seams[y - dy][x - dx];
    if (s1 > 1 && s2 > 1)
    {
        return image[y + dy][x + dx] + image[y - dy][x - dx];
    }
    else
    {
        return (2 - std::min<int>(s2, 2))*image[y + dy][x + dx] + (2 - std::min<int>(s1, 2))*image[y - dy][x - dx];
    };
};

template <class Image>
void RestrictErrorToNextLevel(const Image & in, Image & out)
{
    typedef typename Image::PixelType ImagePixelType;
    const int smallWidth = out.width() - 1;
    const int smallHeight = out.height() - 1;
#pragma omp parallel for schedule(dynamic, 100)
    for (int y = 0; y < smallHeight; y++)
    {
        for (int x = 0; x < smallWidth; x++)
        {
            out[y][x] = in[2 * y][2 * x] + in[2 * y][2 * x + 1] + in[2 * y + 1][2 * x] + in[2 * y + 1][2 * x + 1];
        };
        // special case of most right pixel
        ImagePixelType p2 = 0;
        if (2 * smallWidth + 1 < in.width())
        {
            p2 = in[2 * y][2 * smallWidth + 1];
        }
        ImagePixelType p3 = 0;
        ImagePixelType p4 = 0;
        if (2 * y + 1 < in.height())
        {
            p3 = in[2 * y + 1][2 * smallWidth];
            if (2 * smallWidth + 1 < in.width())
            {
                p4 = in[2 * y + 1][2 * smallWidth + 1];
            };
        };
        out[y][smallWidth] = in[2 * y][2 * smallWidth] + p2 + p3 + p4;
    };
    // special case of last line
    for (int x = 0; x < smallWidth; x++)
    {
        ImagePixelType p34 = 0;
        if (2 * smallHeight + 1 < in.height())
        {
            p34 = in[2 * smallHeight + 1][2 * x] + in[2 * smallHeight + 1][2 * x + 1];
        };
        out[smallHeight][x] = in[2 * smallHeight][2 * x] + in[2 * smallHeight][2 * x + 1] + p34;
    };
    ImagePixelType p2 = 0;
    if (2 * smallWidth + 1 < in.width())
    {
        p2 = in[2 * smallHeight][2 * smallWidth + 1];
    }
    ImagePixelType p3 = 0;
    ImagePixelType p4 = 0;
    if (2 * smallHeight + 1 < in.height())
    {
        p3 = in[2 * smallHeight + 1][2 * smallWidth];
        if (2 * smallWidth + 1 < in.width())
        {
            p4 = in[2 * smallHeight + 1][2 * smallWidth + 1];
        };
    };
    out[smallHeight][smallWidth] = in[2 * smallHeight][2 * smallWidth] + p2 + p3 + p4;
};

// internal use by FindEdgesForPoisson
struct FilterEdges
{
    vigra::Int8 operator()(float const& vf) const
    {
        const int v = static_cast<int>(vf);
        if (v > 0 && v < 17)
        {
            return 1;
        }
        else
        {
            if (v >= 85)
            {
                if (v == 85 || v == 89 || v == 93 || v == 97)
                {
                    return 3;
                }
                else
                {
                    return 2;
                }
            }
            else
            {
                return 0;
            };
        };
    };
};

// multi-threaded variant for convolution of images, only 4-neighborhood is considerd
template <class Image1, class Image2>
void SimpleConvolveImage4(const Image1& image1, Image2& image2, const double factor1, const double factor2)
{
    vigra_precondition(image1.size() == image2.size(), "ConvolveImage: Image size does not match");
    vigra_precondition(image1.width() >= 2 && image1.height() >= 2, "ConvolveImage: Image too small");
    const int width = image1.width();
    const int height = image1.height();
    //special treatment of first line
    image2[0][0] = factor1*image1[0][0] + factor2*image1[0][1] + factor2*image1[1][0];
    for (int x = 1; x < width - 1; ++x)
    {
        image2[0][x] = factor1*image1[0][x] + factor2*image1[0][x - 1] + factor2*image1[0][x + 1] + factor2*image1[1][x];
    };
    image2[0][width - 1] = factor1*image1[0][width - 1] + factor2*image1[0][width - 2] + factor2*image1[1][width - 1];
#pragma omp parallel for
    for (int y = 1; y < height - 1; ++y)
    {
        image2[y][0] = factor1*image1[y][0] + factor2*image1[y - 1][0]
            + factor2*image1[y][1] + factor2*image1[y + 1][0];
        for (size_t x = 1; x < width - 1; ++x)
        {
            image2[y][x] = factor1*image1[y][x] + factor2*image1[y - 1][x]
                + factor2*image1[y][x - 1] + factor2*image1[y + 1][x] + factor2*image1[y][x + 1];
        }
        image2[y][width - 1] = factor1*image1[y][width - 1] + factor2*image1[y - 1][width - 1]
            + factor2*image1[y][width - 2] + factor2*image1[y + 1][width - 1];
    };
    //special treatment of last line
    image2[height - 1][0] = factor1*image1[height - 1][0] + factor2*image1[height - 1][1] + factor2*image1[height - 2][0];
    for (size_t x = 1; x < width - 1; ++x)
    {
        image2[height - 1][x] = factor1*image1[height - 1][x] + factor2*image1[height - 1][x - 1] + factor2*image1[height - 1][x + 1] + factor2*image1[height - 2][x];
    };
    image2[height - 1][width - 1] = factor1*image1[height - 1][width - 1] + factor2*image1[height - 1][width - 2] + factor2*image1[height - 2][width - 1];
};

/** mark edges in input image for poisson blending *
 *  input: expected an image with following meanings
 *    labels has now the following values:
 *     0: no image here
 *     1: use information from image 1
 *     5: use information from image 2
 *  output: contains the following pixel values
 *     0: no information available
 *     1: at boundary between image 1 and 2: here we use Dirichlet boundary condition
 *     2: at boundary between image 2 and empty pixels: here Neumann boundary condition applies
 *     3: inside image 2
*/
template <class Image>
vigra::Int8Image FindEdgesForPoisson(const Image& input)
{
    vigra::Int8Image output(input.size());
    SimpleConvolveImage4(input, output, 21, -1);
    vigra::omp::transformImage(srcImageRange(output), destImage(output), detail::FilterEdges());
    return output;
};

template <class ComponentType>
double GetRealValue(const ComponentType& val) { return val; }

template <class ComponentType>
double GetRealValue(const vigra::RGBValue<ComponentType>& val) { return val.magnitude(); }

template <class Image, class SeamMask>
void SOR(Image& target, const Image& gradient, const SeamMask& seams, const float omega, const float errorThreshold, const int maxIter, const bool doWrap)
{
    typedef typename Image::PixelType TargetPixelType;
    const int width = target.width();
    const int height = target.height();

    // changes in last iteration
    double oldError = 0;
    for (int j = 0; j < maxIter; j++)
    {
        // changes in current iteration
        double error = 0;
        if (seams[0][0]>1)
        {
            if (doWrap)
            {
                const TargetPixelType delta = omega*((gradient[0][0] + target[0][1] + 2 * target[1][0] + target[0][width - 1]) / 4.0f - target[0][0]);
                error += detail::GetRealValue(delta*delta);
                target[0][0] += delta;
            }
            else
            {
                const TargetPixelType delta = omega*((gradient[0][0] + 2 * target[0][1] + 2 * target[1][0]) / 4.0f - target[0][0]);
                error += detail::GetRealValue(delta*delta);
                target[0][0] += delta;
            };
        };
        for (int x = 1; x < width - 1; ++x)
        {
            if (seams[0][x]>1)
            {
                const TargetPixelType delta = omega*((gradient[0][x] + detail::GetBorderValues(x, 0, 1, 0, target, seams) + 2 * target[1][x]) / 4.0f - target[0][x]);
                error += detail::GetRealValue(delta*delta);
                target[0][x] += delta;
            };
        };
        if (seams[0][width - 1] > 1)
        {
            if (doWrap)
            {
                const TargetPixelType delta = omega*((gradient[0][width - 1] + target[0][width - 2] + 2 * target[1][width - 1] + target[0][0]) / 4.0f - target[0][width - 1]);
                error += detail::GetRealValue(delta*delta);
                target[0][width - 1] += delta;
            }
            else
            {
                const TargetPixelType delta = omega*((gradient[0][width - 1] + 2 * target[0][width - 2] + 2 * target[1][width - 1]) / 4.0f - target[0][width - 1]);
                error += detail::GetRealValue(delta*delta);
                target[0][width - 1] += delta;
            };
        };
#pragma omp parallel for reduction(+: error) schedule(dynamic, 100)
        for (int y = 1; y < height - 1; ++y)
        {
            if (seams[y][0] > 1)
            {
                if (doWrap)
                {
                    const TargetPixelType delta = omega*((gradient[y][0] + detail::GetBorderValues(0, y, 0, 1, target, seams)
                        + target[y][1] + target[y][width - 1]) / 4.0f - target[y][0]);
                    error += detail::GetRealValue(delta*delta);
                    target[y][0] += delta;
                }
                else
                {
                    const TargetPixelType delta = omega*((gradient[y][0] + detail::GetBorderValues(0, y, 0, 1, target, seams) + 2 * target[y][1]) / 4.0f - target[y][0]);
                    error += detail::GetRealValue(delta*delta);
                    target[y][0] += delta;
                };
            };
            for (int x = 1; x < width - 1; ++x)
            {
                const typename SeamMask::value_type maskValue = seams[y][x];
                if (maskValue > 1)
                {
                    if (maskValue == 2)
                    {
                        // border pixel
                        const TargetPixelType sum = detail::GetBorderValues(x, y, 1, 0, target, seams) + detail::GetBorderValues(x, y, 0, 1, target, seams);
                        const TargetPixelType delta = omega*((gradient[y][x] + sum) / 4.0f - target[y][x]);
                        error += detail::GetRealValue(delta*delta);
                        target[y][x] += delta;
                    }
                    else
                    {
                        const TargetPixelType sum = target[y + 1][x] + target[y][x + 1] + target[y - 1][x] + target[y][x - 1];
                        const TargetPixelType delta = omega*((gradient[y][x] + sum) / 4.0f - target[y][x]);
                        error += detail::GetRealValue(delta*delta);
                        target[y][x] += delta;
                    };
                };
            };
            if (seams[y][width - 1] > 1)
            {
                if (doWrap)
                {
                    const TargetPixelType delta = omega*((gradient[y][width - 1] + detail::GetBorderValues(width - 1, y, 0, 1, target, seams) + target[y][width - 2] + target[y][0]) / 4.0f - target[y][width - 1]);
                    error += detail::GetRealValue(delta*delta);
                    target[y][width - 1] += delta;
                }
                else
                {
                    const TargetPixelType delta = omega*((gradient[y][width - 1] + detail::GetBorderValues(width - 1, y, 0, 1, target, seams) + 2 * target[y][width - 2]) / 4.0f - target[y][width - 1]);
                    error += detail::GetRealValue(delta*delta);
                    target[y][width - 1] += delta;
                };
            };
        };
        // last line
        if (seams[height - 1][0] > 1)
        {
            if (doWrap)
            {
                const TargetPixelType delta = omega*((gradient[height - 1][0] + 2 * target[height - 2][0] + target[height - 1][1] + target[height - 1][width - 1]) / 4.0f - target[height - 1][0]);
                error += detail::GetRealValue(delta*delta);
                target[height - 1][0] += delta;
            }
            else
            {
                const TargetPixelType delta = omega*((gradient[height - 1][0] + 2 * target[height - 2][0] + 2 * target[height - 1][1]) / 4.0f - target[height - 1][0]);
                error += detail::GetRealValue(delta*delta);
                target[height - 1][0] += delta;
            };
        };
        for (int x = 1; x < width - 1; ++x)
        {
            if (seams[height - 1][x] > 1)
            {
                const TargetPixelType delta = omega*((gradient[height - 1][x] + detail::GetBorderValues(x, height - 1, 1, 0, target, seams) + 2 * target[height - 2][x]) / 4.0f - target[height - 1][x]);
                error += detail::GetRealValue(delta*delta);
                target[height - 1][x] += delta;
            };
        };
        if (seams[height - 1][width - 1] > 1)
        {
            if (doWrap)
            {
                const TargetPixelType delta = omega*((gradient[height - 1][width - 1] + 2 * target[height - 2][width - 1] + target[height - 1][width - 2] + target[height - 1][0]) / 4.0f - target[height - 1][width - 1]);
                error += detail::GetRealValue(delta*delta);
                target[height - 1][width - 1] += delta;
            }
            else
            {
                const TargetPixelType delta = omega*((gradient[height - 1][width - 1] + 2 * target[height - 2][width - 1] + 2 * target[height - 1][width - 2]) / 4.0f - target[height - 1][width - 1]);
                error += detail::GetRealValue(delta*delta);
                target[height - 1][width - 1] += delta;
            };
        };

        if (oldError > 0 && log(oldError / error) / log(10.0) < errorThreshold)
        {
            break;
        }
        oldError = error;
    }
}

template <class Image, class SeamMask>
void CalcResidualError(Image& error, const Image& target, const Image& gradient, const SeamMask& seam, const bool doWrap)
{
    typedef typename Image::PixelType ImagePixelType;
    const int width = target.width();
    const int height = target.height();

    if (seam[0][0] > 1)
    {
        if (doWrap)
        {
            const ImagePixelType sum = 2 * target[1][0] + target[0][1] + target[0][width - 1];
            error[0][0] = (4 * target[0][0] - sum - gradient[0][0]);
        }
        else
        {
            const ImagePixelType sum = 2 * target[1][0] + 2 * target[0][1];
            error[0][0] = (4 * target[0][0] - sum - gradient[0][0]);
        };
    };
    for (int x = 1; x < width - 1; ++x)
    {
        if (seam[0][x]>1)
        {
            const ImagePixelType sum = 2 * target[1][x] + detail::GetBorderValues(x, 0, 1, 0, target, seam);
            error[0][x] = (4 * target[0][x] - sum - gradient[0][x]);
        };
    };
    if (seam[0][width - 1] > 1)
    {
        if (doWrap)
        {
            const ImagePixelType sum = 2 * target[1][width - 1] + target[0][width - 2] + target[0][0];
            error[0][width - 1] = (4 * target[0][width - 1] - sum - gradient[0][width - 1]);
        }
        else
        {
            const ImagePixelType sum = 2 * target[1][width - 1] + 2 * target[0][width - 2];
            error[0][width - 1] = (4 * target[0][width - 1] - sum - gradient[0][width - 1]);
        }
    };
#pragma omp parallel for schedule(dynamic, 100)
    for (int y = 1; y < height - 1; ++y)
    {
        if (seam[y][0] > 1)
        {
            if (doWrap)
            {
                const ImagePixelType sum = detail::GetBorderValues(0, y, 0, 1, target, seam) + target[y][1] + target[y][width - 1];
                error[y][0] = (4 * target[y][0] - sum - gradient[y][0]);
            }
            else
            {
                const ImagePixelType sum = detail::GetBorderValues(0, y, 0, 1, target, seam) + 2 * target[y][1];
                error[y][0] = (4 * target[y][0] - sum - gradient[y][0]);
            };
        }
        for (int x = 1; x < width - 1; ++x)
        {
            const typename SeamMask::value_type maskValue = seam[y][x];
            if (maskValue > 1)
            {
                if (maskValue == 2)
                {
                    // border pixel
                    const ImagePixelType sum = detail::GetBorderValues(x, y, 1, 0, target, seam) + detail::GetBorderValues(x, y, 0, 1, target, seam);
                    error[y][x] = (4 * target[y][x] - sum - gradient[y][x]);
                }
                else
                {
                    const ImagePixelType sum = target[y + 1][x] + target[y][x + 1] + target[y - 1][x] + target[y][x - 1];
                    error[y][x] = (4 * target[y][x] - sum - gradient[y][x]);
                };
            };
        };
        if (seam[y][width - 1] > 1)
        {
            if (doWrap)
            {
                const ImagePixelType sum = detail::GetBorderValues(width - 1, y, 0, 1, target, seam) + target[y][width - 2] + target[y][0];
                error[y][width - 1] = (4 * target[y][width - 1] - sum - gradient[y][width - 1]);
            }
            else
            {
                const ImagePixelType sum = detail::GetBorderValues(width - 1, y, 0, 1, target, seam) + 2 * target[y][width - 2];
                error[y][width - 1] = (4 * target[y][width - 1] - sum - gradient[y][width - 1]);
            };
        };
    };
    // last line
    if (seam[height - 1][0] > 1)
    {
        if (doWrap)
        {
            const ImagePixelType sum = 2 * target[height - 2][0] + target[height - 1][width - 1] + target[height - 1][1];
            error[height - 1][0] = (4 * target[height - 1][0] - sum - gradient[height - 1][0]);
        }
        else
        {
            const ImagePixelType sum = 2 * target[height - 2][0] + 2 * target[height - 1][1];
            error[height - 1][0] = (4 * target[height - 1][0] - sum - gradient[height - 1][0]);
        };
    };
    for (int x = 1; x < width - 1; ++x)
    {
        if (seam[height - 1][x]>1)
        {
            const ImagePixelType sum = 2 * target[height-2][x] + detail::GetBorderValues(x, height - 1, 1, 0, target, seam);
            error[height - 1][x] = (4 * target[height - 1][x] - sum - gradient[height - 1][x]);
        };
    };
    if (seam[height - 1][width - 1] > 1)
    {
        if (doWrap)
        {
            const ImagePixelType sum = 2 * target[height - 2][width - 1] + target[height - 1][width - 2] + target[height - 1][0];
            error[height - 1][width - 1] = (4 * target[height - 1][width - 1] - sum - gradient[height - 1][width - 1]);
        }
        else
        {
            const ImagePixelType sum = 2 * target[height - 2][width - 1] + 2 * target[height - 1][width - 2];
            error[height - 1][width - 1] = (4 * target[height - 1][width - 1] - sum - gradient[height - 1][width - 1]);
        };
    };

}

} // namespace detail
template <class PixelType>
class MaskGreaterAccessor
{
public:
    explicit MaskGreaterAccessor(PixelType val) : m_value(val) {};

    template <class ITERATOR>
    PixelType operator()(ITERATOR const & i) const {
        if (PixelType(*i) >= m_value)
        {
            return vigra::NumericTraits<PixelType>::max();
        }
        else
        {
            return vigra::NumericTraits<PixelType>::zero();
        };
    }

    template <class ITERATOR, class DIFFERENCE>
    PixelType operator()(ITERATOR const & i, DIFFERENCE d) const
    {
        if (PixelType(i[d]) >= m_value)
        {
            return vigra::NumericTraits<PixelType>::max();
        }
        else
        {
            return vigra::NumericTraits<PixelType>::zero();
        };
    }
    PixelType m_value;
};

template <class PixelType>
class MaskSmallerAccessor
{
public:
    explicit MaskSmallerAccessor(PixelType val) : m_value(val) {};

    template <class ITERATOR>
    PixelType operator()(ITERATOR const & i) const {
        if (PixelType(*i) < m_value)
        {
            return vigra::NumericTraits<PixelType>::max();
        }
        else
        {
            return vigra::NumericTraits<PixelType>::zero();
        };
    }

    template <class ITERATOR, class DIFFERENCE>
    PixelType operator()(ITERATOR const & i, DIFFERENCE d) const
    {
        if (PixelType(i[d]) < m_value)
        {
            return vigra::NumericTraits<PixelType>::max();
        }
        else
        {
            return vigra::NumericTraits<PixelType>::zero();
        };
    }
    PixelType m_value;
};

template <class Image, class PyramidImage>
void BuildSeamPyramid(const Image& input, vigra::ImagePyramid<PyramidImage>& seams, const int minLength)
{
    const int nlevels = (int)ceil(log(std::min(input.width(), input.height()) / (double)minLength) / log(2.0));
    seams.resize(0, nlevels, input.size());
    Image scaledImage = input;
    seams[0] = detail::FindEdgesForPoisson(input);
    for (size_t i = 1; i <= seams.highestLevel(); ++i)
    {
        Image smaller((scaledImage.width() + 1) / 2, (scaledImage.height() + 1) / 2);
        vigra::resizeImageNoInterpolation(vigra::srcImageRange(scaledImage), vigra::destImageRange(smaller));
        seams[i] = detail::FindEdgesForPoisson(smaller);
        scaledImage = smaller;
    };
}

template<class Image, class Mask, class SeamMask, class GradientType>
void BuildGradientMap(const Image& image1, const Image& image2, const Mask& mask2, const SeamMask& seam, GradientType& gradient, const vigra::Point2D& offset, const bool doWrap)
{
    typedef typename GradientType::PixelType GradientPixelType;
    const int width = image2.width();
    const int height = image2.height();
    //special treatment of first line
    if (seam[0][0] == 2)
    {
        if (doWrap)
        {
            GradientPixelType value = 4 * image2[0][0] - image2[0][1] - 2 * image2[1][0] - image2[0][width - 1];
            // copy values for Dirichlet boundary condition 
            value += detail::GetBorderGradient(0, 0, 1, 0, seam, image1, offset);
            value += detail::GetBorderGradient(0, 0, 0, 1, seam, image1, offset);
            value += detail::GetBorderGradient(width - 2, 0, 1, 0, seam, image1, offset);
            gradient[0][0] = value;
        }
        else
        {
            GradientPixelType value = 4 * image2[0][0] - 2 * image2[0][1] - 2 * image2[1][0];
            // copy values for Dirichlet boundary condition 
            value += detail::GetBorderGradient(0, 0, 1, 0, seam, image1, offset);
            value += detail::GetBorderGradient(0, 0, 0, 1, seam, image1, offset);
            gradient[0][0] = value;
        };
    };
    for (int x = 1; x < width - 1; ++x)
    {
        if (seam[0][x] == 2)
        {
            GradientPixelType value = 4 * image2[0][x] - detail::ProcessBorderPixel(x, 0, 1, 0, image2, mask2, seam) - 2 * image2[1][x];
            value += detail::GetBorderGradient(x, 0, -1, 0, seam, image1, offset);
            value += detail::GetBorderGradient(x, 0, 1, 0, seam, image1, offset);
            value += detail::GetBorderGradient(x, 0, 0, 1, seam, image1, offset);
            gradient[0][x] = value;
        };
    };
    if (seam[0][width - 1] == 2)
    {
        if (doWrap)
        {
            GradientPixelType value = 4 * image2[0][width - 1] - image2[0][width - 2] - 2 * image2[1][width - 1] - image2[0][0];
            value += detail::GetBorderGradient(width - 1, 0, -1, 0, seam, image1, offset);
            value += detail::GetBorderGradient(width - 1, 0, 0, 1, seam, image1, offset);
            value += detail::GetBorderGradient(1, 0, -1, 0, seam, image1, offset);
            gradient[0][width - 1] = value;
        }
        else
        {
            GradientPixelType value = 4 * image2[0][width - 1] - 2 * image2[0][width - 2] - 2 * image2[1][width - 1];
            value += detail::GetBorderGradient(width - 1, 0, -1, 0, seam, image1, offset);
            value += detail::GetBorderGradient(width - 1, 0, 0, 1, seam, image1, offset);
            gradient[0][width - 1] = value;
        };
    };
#pragma omp parallel for
    for (int y = 1; y < height - 1; ++y)
    {
        if (seam[y][0] == 2)
        {
            if (doWrap)
            {
                GradientPixelType value = 4 * image2[y][0] - detail::ProcessBorderPixel(0, y, 0, 1, image2, mask2, seam) - image2[y][1] - image2[y][width - 1];
                value += detail::GetBorderGradient(0, y, 1, 0, seam, image1, offset);
                value += detail::GetBorderGradient(0, y, 0, 1, seam, image1, offset);
                value += detail::GetBorderGradient(0, y, 0, -1, seam, image1, offset);
                value += detail::GetBorderGradient(width - 2, y, 1, 0, seam, image1, offset);
                gradient[y][0] = value;
            }
            else
            {
                GradientPixelType value = 4 * image2[y][0] - detail::ProcessBorderPixel(0, y, 0, 1, image2, mask2, seam) - 2 * image2[y][1];
                value += detail::GetBorderGradient(0, y, 1, 0, seam, image1, offset);
                value += detail::GetBorderGradient(0, y, 0, 1, seam, image1, offset);
                value += detail::GetBorderGradient(0, y, 0, -1, seam, image1, offset);
                gradient[y][0] = value;
            };
        };
        for (int x = 1; x < width - 1; ++x)
        {
            const typename SeamMask::PixelType seamVal = seam[y][x];
            if (seamVal>1)
            {
                GradientPixelType value = 4.0 * image2[y][x];
                if (seamVal == 3)
                {
                    value -= detail::ProcessNeighborPixels(x, y, 1, 0, image2, mask2);
                    value -= detail::ProcessNeighborPixels(x, y, 0, 1, image2, mask2);
                }
                else
                {
                    // seamVal==2, border pixel
                    value -= detail::ProcessBorderPixel(x, y, 1, 0, image2, mask2, seam);
                    value -= detail::ProcessBorderPixel(x, y, 0, 1, image2, mask2, seam);
                };
                // copy values for Dirichlet boundary condition 
                value += detail::GetBorderGradient(x, y, 1, 0, seam, image1, offset);
                value += detail::GetBorderGradient(x, y, 0, 1, seam, image1, offset);
                value += detail::GetBorderGradient(x, y, -1, 0, seam, image1, offset);
                value += detail::GetBorderGradient(x, y, 0, -1, seam, image1, offset);
                gradient[y][x] = value;
            }
        };
        if (seam[y][width - 1] == 2)
        {
            if (doWrap)
            {
                GradientPixelType value = 4.0 * image2[y][width - 1] - detail::ProcessBorderPixel(width - 1, y, 0, 1, image2, mask2, seam) - image2[y][width - 2] - image2[y][0];
                value += detail::GetBorderGradient(width - 1, y, -1, 0, seam, image1, offset);
                value += detail::GetBorderGradient(width - 1, y, 0, 1, seam, image1, offset);
                value += detail::GetBorderGradient(width - 1, y, 0, -1, seam, image1, offset);
                value += detail::GetBorderGradient(0, y, -1, 0, seam, image1, offset);
                gradient[y][width - 1] = value;
            }
            else
            {
                GradientPixelType value = 4.0 * image2[y][width - 1] - detail::ProcessBorderPixel(width - 1, y, 0, 1, image2, mask2, seam) - 2 * image2[y][width - 2];
                value += detail::GetBorderGradient(width - 1, y, -1, 0, seam, image1, offset);
                value += detail::GetBorderGradient(width - 1, y, 0, 1, seam, image1, offset);
                value += detail::GetBorderGradient(width - 1, y, 0, -1, seam, image1, offset);
                gradient[y][width - 1] = value;
            };
        };
    };
    //special treatment of last line
    if (seam[height - 1][0] == 2)
    {
        if (doWrap)
        {
            GradientPixelType value = 4 * image2[height - 1][0] - image2[height - 1][1] - 2 * image2[height - 2][0] - image2[height - 1][width - 1];
            value += detail::GetBorderGradient(0, height - 1, 1, 0, seam, image1, offset);
            value += detail::GetBorderGradient(0, height - 1, 0, -1, seam, image1, offset);
            value += detail::GetBorderGradient(width - 2, height - 1, 1, 0, seam, image1, offset);
            gradient[height - 1][0] = value;
        }
        else
        {
            GradientPixelType value = 4 * image2[height - 1][0] - 2 * image2[height - 1][1] - 2 * image2[height - 2][0];
            value += detail::GetBorderGradient(0, height - 1, 1, 0, seam, image1, offset);
            value += detail::GetBorderGradient(0, height - 1, 0, -1, seam, image1, offset);
            gradient[height - 1][0] = value;
        };
    };
    for (size_t x = 1; x < width - 1; ++x)
    {
        if (seam[height - 1][x] == 2)
        {
            GradientPixelType value = 4 * image2[height - 1][x] - detail::ProcessBorderPixel(x, height - 1, 1, 0, image2, mask2, seam) - 2 * image2[height - 2][x];
            value += detail::GetBorderGradient(x, height - 1, -1, 0, seam, image1, offset);
            value += detail::GetBorderGradient(x, height - 1, 1, 0, seam, image1, offset);
            value += detail::GetBorderGradient(x, height - 1, 0, -1, seam, image1, offset);
            gradient[height - 1][x] = value;
        };
    };
    if (seam[height - 1][width - 1] == 2)
    {
        if (doWrap)
        {
            GradientPixelType value = 4 * image2[height - 1][width - 1] - image2[height - 1][width - 2] - 2 * image2[height - 2][width - 1] - image2[height - 1][0];
            value += detail::GetBorderGradient(width - 1, height - 1, -1, 0, seam, image1, offset);
            value += detail::GetBorderGradient(width - 1, height - 1, 0, -1, seam, image1, offset);
            value += detail::GetBorderGradient(1, height - 1, -1, 0, seam, image1, offset);
            gradient[height - 1][width - 1] = value;
        }
        else
        {
            GradientPixelType value = 4 * image2[height - 1][width - 1] - 2 * image2[height - 1][width - 2] - 2 * image2[height - 2][width - 1];
            value += detail::GetBorderGradient(width - 1, height - 1, -1, 0, seam, image1, offset);
            value += detail::GetBorderGradient(width - 1, height - 1, 0, -1, seam, image1, offset);
            gradient[height - 1][width - 1] = value;
        };
    };
};

template <class Image, class SeamMask>
void Multigrid(Image& out, const Image& gradient, const vigra::ImagePyramid<SeamMask>& seamMaskPyramid, int minLen, const float errorThreshold, const int maxIter, const bool doWrap)
{
    const int width = out.width();
    const int height = out.height();

    if (width < minLen || height < minLen)
    {
        return;
    }
    Image err(width, height);
    Image err2((width + 1) / 2, (height + 1) / 2);
    Image out2(err2.size());
    int maskIndex = -1;
    for (int i = 0; i <= seamMaskPyramid.highestLevel(); ++i)
    {
        if (out.size() == seamMaskPyramid[i].size())
        {
            maskIndex = i;
            break;
        };
    }
    if (maskIndex == -1)
    {
        std::cout << "ERROR: No suitable mask, this should not happen." << std::endl
            << "searching " << out.size() << ", finest " << seamMaskPyramid[seamMaskPyramid.highestLevel()].size() << std::endl;
        return;
    };
    // pre-smoothing
    detail::SOR(out, gradient, seamMaskPyramid[maskIndex], 1.95f, errorThreshold, maxIter, doWrap);
    detail::CalcResidualError(err, out, gradient, seamMaskPyramid[maskIndex], doWrap);    // Fehler berechnen
    detail::RestrictErrorToNextLevel(err, err2);
    Multigrid(out2, err2, seamMaskPyramid, minLen, errorThreshold, maxIter, doWrap);
    // again W cycle
    Multigrid(out2, err2, seamMaskPyramid, minLen, errorThreshold, maxIter, doWrap); 
    vigra::resizeImageLinearInterpolation(srcImageRange(out2), destImageRange(err)); 
    vigra::omp::combineTwoImagesIf(vigra::srcImageRange(out), vigra::srcImage(err),
        vigra::srcImage(seamMaskPyramid[maskIndex], MaskGreaterAccessor<typename SeamMask::PixelType>(2)),
        vigra::destImage(out),
        vigra::functor::Arg1() - vigra::functor::Arg2());
    // post smoothing
    detail::SOR(out, gradient, seamMaskPyramid[maskIndex], 1.95f, 2.0f * errorThreshold, maxIter, doWrap);
    return;
}

} // namespace poisson
} // namespace vigra_ext

#endif //POISSON_BLEND_H
