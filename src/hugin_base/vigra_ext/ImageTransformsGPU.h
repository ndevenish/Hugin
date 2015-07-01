// -*- c-basic-offset: 4 -*-
/** @file ImageTransformsGPU.h
 *
 *  Contains functions to transform whole images.
 *  Uses PTools::Transform for the calculations
 *
 *  @author Andrew Mihal
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

#ifndef _VIGRA_EXT_IMAGETRANSFORMSGPU_H
#define _VIGRA_EXT_IMAGETRANSFORMSGPU_H

#include <fstream>
#include <iostream>
#include <iomanip>

#include <hugin_shared.h>
#include <vigra/basicimage.hxx>
#include <vigra_ext/ROIImage.h>
#include <vigra_ext/Interpolators.h>
#include <vigra/accessor.hxx>
#include <vigra_ext/FunctorAccessor.h>

#include <hugin_math/hugin_math.h>
#include <hugin_utils/utils.h>
#include <appbase/ProgressDisplay.h>

using vigra::NumericTraits;

namespace vigra_ext
{

IMPEX void SetGPUDebugMessages(const bool doPrint);

bool IMPEX transformImageGPUIntern(const std::string& coordXformGLSL,
                             const std::string& interpolatorGLSL,
                             const int interpolatorSize,
                             const std::string& photometricGLSL,
                             const std::vector<double>& invLut,
                             const std::vector<double>& destLut,
                             const vigra::Diff2D srcSize,
                             const void* const srcBuffer,
                             const int srcGLInternalFormat, const int srcGLTransferFormat, const int srcGLFormat, const int srcGLType,
                             const void* const srcAlphaBuffer,
                             const int srcAlphaGLType,
                             const vigra::Diff2D destUL,
                             const vigra::Diff2D destSize,
                             void* const destBuffer,
                             const int destGLInternalFormat, const int destGLTransferFormat, const int destGLFormat, const int destGLType,
                             void* const destAlphaBuffer,
                             const int destAlphaGLType,
                             const bool warparound);

// This is to avoid including GL headers in this file.
enum {
    // gltypes
    XGL_BYTE=0, XGL_UNSIGNED_BYTE, XGL_SHORT, XGL_UNSIGNED_SHORT, XGL_INT, XGL_UNSIGNED_INT, XGL_FLOAT,
    // Internalformats
    XGL_RGBA8, XGL_RGBA16, XGL_RGBA32F, XGL_LUMINANCE8_ALPHA8, XGL_LUMINANCE16_ALPHA16, XGL_LUMINANCE_ALPHA32F,
    XGL_RGB8,  XGL_RGB16,  XGL_RGB32F,  XGL_LUMINANCE8,        XGL_LUMINANCE16,         XGL_LUMINANCE32F,
    // formats
    XGL_LUMINANCE, XGL_RGB, XGL_LUMINANCE_ALPHA, XGL_RGBA
};

struct Error_GpuNumericTraits_not_specialized_for_this_case { };

template <class A>
struct GpuNumericTraits {
    typedef Error_GpuNumericTraits_not_specialized_for_this_case ImagePixelComponentType;
    enum {NumBands = 0};
    enum {ImageGLInternalFormat = 0};
    enum {ImageGLTransferFormat = 0};
    enum {ImageGLFormat = 0};
    enum {ImagePixelComponentGLType = 0};
};

#define DEFINE_GPUNUMERICTRAITS(IMAGECOMPONENT, GLFORMAT, GLFORMATRGB, GLTRANSFER, GLTRANSFERRGB, GLTYPE) \
template<> \
struct GpuNumericTraits<IMAGECOMPONENT> { \
    typedef IMAGECOMPONENT ImagePixelComponentType; \
    enum {NumBands = 1}; \
    enum {ImageGLInternalFormat = GLFORMAT}; \
    enum {ImageGLTransferFormat = GLTRANSFER}; \
    enum {ImageGLFormat = XGL_LUMINANCE}; \
    enum {ImagePixelComponentGLType = GLTYPE}; \
}; \
template<> \
struct GpuNumericTraits<vigra::RGBValue<IMAGECOMPONENT, 0, 1, 2> > { \
    typedef IMAGECOMPONENT ImagePixelComponentType; \
    enum {NumBands = 3}; \
    enum {ImageGLInternalFormat = GLFORMATRGB}; \
    enum {ImageGLTransferFormat = GLTRANSFERRGB}; \
    enum {ImageGLFormat = XGL_RGB}; \
    enum {ImagePixelComponentGLType = GLTYPE}; \
}; \
template<> \
struct GpuNumericTraits<vigra::TinyVector<IMAGECOMPONENT, 2> > { \
    typedef IMAGECOMPONENT ImagePixelComponentType; \
    enum {NumBands = 2}; \
    enum {ImageGLInternalFormat = GLFORMAT}; \
    enum {ImageGLTransferFormat = GLFORMAT}; \
    enum {ImageGLFormat = XGL_LUMINANCE_ALPHA}; \
    enum {ImagePixelComponentGLType = GLTYPE}; \
}; \
template<> \
struct GpuNumericTraits<vigra::TinyVector<IMAGECOMPONENT, 4> > { \
    typedef IMAGECOMPONENT ImagePixelComponentType; \
    enum {NumBands = 4}; \
    enum {ImageGLInternalFormat = GLFORMATRGB}; \
    enum {ImageGLTransferFormat = GLFORMATRGB}; \
    enum {ImageGLFormat = XGL_RGBA}; \
    enum {ImagePixelComponentGLType = GLTYPE}; \
};

DEFINE_GPUNUMERICTRAITS(vigra::Int8,   XGL_LUMINANCE8_ALPHA8,   XGL_RGBA8,   XGL_LUMINANCE8,   XGL_RGB8,   XGL_BYTE);
DEFINE_GPUNUMERICTRAITS(vigra::UInt8,  XGL_LUMINANCE8_ALPHA8,   XGL_RGBA8,   XGL_LUMINANCE8,   XGL_RGB8,   XGL_UNSIGNED_BYTE);
DEFINE_GPUNUMERICTRAITS(vigra::Int16,  XGL_LUMINANCE16_ALPHA16, XGL_RGBA16,  XGL_LUMINANCE16,  XGL_RGB16,  XGL_SHORT);
DEFINE_GPUNUMERICTRAITS(vigra::UInt16, XGL_LUMINANCE16_ALPHA16, XGL_RGBA16,  XGL_LUMINANCE16,  XGL_RGB16,  XGL_UNSIGNED_SHORT);
DEFINE_GPUNUMERICTRAITS(vigra::Int32,  XGL_LUMINANCE_ALPHA32F,  XGL_RGBA32F, XGL_LUMINANCE32F, XGL_RGB32F, XGL_INT);
DEFINE_GPUNUMERICTRAITS(vigra::UInt32, XGL_LUMINANCE_ALPHA32F,  XGL_RGBA32F, XGL_LUMINANCE32F, XGL_RGB32F, XGL_UNSIGNED_INT);
DEFINE_GPUNUMERICTRAITS(float,         XGL_LUMINANCE_ALPHA32F,  XGL_RGBA32F, XGL_LUMINANCE32F, XGL_RGB32F, XGL_FLOAT);

// FIXME ACM this is wrong, there is no GL_DOUBLE for transfering doubles.
DEFINE_GPUNUMERICTRAITS(double,        XGL_LUMINANCE_ALPHA32F,  XGL_RGBA32F, XGL_LUMINANCE32F, XGL_RGB32F, XGL_FLOAT);


/** Transform an image into the panorama
 *
 *  Uses the GPU for processing.
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
void transformImageGPUIntern(vigra::triple<SrcImageIterator, SrcImageIterator, SrcAccessor> src,
                             vigra::triple<DestImageIterator, DestImageIterator, DestAccessor> dest,
                             std::pair<AlphaImageIterator, AlphaAccessor> alpha,
                             TRANSFORM & transform,
                             PixelTransform & pixelTransform,
                             vigra::Diff2D destUL,
                             Interpolator interp,
                             bool warparound,
                             AppBase::ProgressDisplay* progress)
{
    typedef typename SrcAccessor::value_type SrcValueType;
    typedef typename DestAccessor::value_type DestValueType;
    typedef typename AlphaAccessor::value_type AlphaValueType;

    vigra::Diff2D srcSize = src.second - src.first;
    vigra::Diff2D destSize = dest.second - dest.first;

    vigra_ext::ImageInterpolator<SrcImageIterator, SrcAccessor, Interpolator>
                                 interpol (src, interp, warparound);

    // Emit coordinate transform and interpolator as GLSL shader program.

    std::ostringstream coordXformOss;
    coordXformOss << std::setprecision(20) << std::showpoint;
    transform.emitGLSL(coordXformOss);

    std::ostringstream interpolatorOss;
    interpolatorOss << std::setprecision(20) << std::showpoint;
    interpol.emitGLSL(interpolatorOss);

    std::ostringstream photometricOss;
    std::vector<double> invLut;
    std::vector<double> destLut;
    photometricOss << std::setprecision(20) << std::showpoint;
    pixelTransform.emitGLSL(photometricOss, invLut, destLut);

    // Do remapping.
    // Give the GPU pointers directly to the image data, bypassing the vigra iterators and accessors.
    // This is cheating. It will not work if the iterators describe subsets of the images or if
    // the accessors perform computation.
    transformImageGPUIntern(coordXformOss.str(),
                            interpolatorOss.str(),
                            interp.size,
                            photometricOss.str(),
                            invLut,
                            destLut,
                            srcSize,
                            src.first[0],
                            GpuNumericTraits<SrcValueType>::ImageGLInternalFormat,
                            GpuNumericTraits<SrcValueType>::ImageGLTransferFormat,
                            GpuNumericTraits<SrcValueType>::ImageGLFormat,
                            GpuNumericTraits<SrcValueType>::ImagePixelComponentGLType,
                            NULL, /* no alpha buffer */
                            XGL_BYTE,
                            destUL,
                            destSize,
                            dest.first[0],
                            GpuNumericTraits<DestValueType>::ImageGLInternalFormat,
                            GpuNumericTraits<DestValueType>::ImageGLTransferFormat,
                            GpuNumericTraits<DestValueType>::ImageGLFormat,
                            GpuNumericTraits<DestValueType>::ImagePixelComponentGLType,
                            alpha.first[0],
                            GpuNumericTraits<AlphaValueType>::ImagePixelComponentGLType,
                            warparound);

}


/** transform input images with alpha channel */
template <class SrcImageIterator, class SrcAccessor,
          class SrcAlphaIterator, class SrcAlphaAccessor,
          class DestImageIterator, class DestAccessor,
          class TRANSFORM,
          class PixelTransform,
          class AlphaImageIterator, class AlphaAccessor,
          class Interpolator>
void transformImageAlphaGPUIntern(vigra::triple<SrcImageIterator, SrcImageIterator, SrcAccessor> src,
                                  std::pair<SrcAlphaIterator, SrcAlphaAccessor> srcAlpha,
                                  vigra::triple<DestImageIterator, DestImageIterator, DestAccessor> dest,
                                  std::pair<AlphaImageIterator, AlphaAccessor> alpha,
                                  TRANSFORM & transform,
                                  PixelTransform & pixelTransform,
                                  vigra::Diff2D destUL,
                                  Interpolator interp,
                                  bool warparound,
                                  AppBase::ProgressDisplay* progress)
{
    typedef typename SrcAccessor::value_type SrcValueType;
    typedef typename SrcAlphaAccessor::value_type SrcAlphaType;
    typedef typename DestAccessor::value_type DestValueType;
    typedef typename AlphaAccessor::value_type DestAlphaType;

    vigra::Diff2D srcSize = src.second - src.first;
    vigra::Diff2D destSize = dest.second - dest.first;

    // Note that GPU interpolators are the same for source images with and without alpha channels.
    vigra_ext::ImageInterpolator<SrcImageIterator, SrcAccessor, Interpolator>
                                 interpol (src, interp, warparound);

    // Emit coordinate transform and interpolator as GLSL shader program.

    std::ostringstream coordXformOss;
    coordXformOss << std::setprecision(20) << std::showpoint;
    if(!transform.emitGLSL(coordXformOss))
    {
        std::cerr << "nona: Found unsupported transformation in stack." << std::endl
             << "      This geometric transformation is not supported by GPU." << std::endl
             << "      Remove -g switch and try with CPU transformation." << std::endl;
        exit(1);
    };

    std::ostringstream interpolatorOss;
    interpolatorOss << std::setprecision(20) << std::showpoint;
    interpol.emitGLSL(interpolatorOss);

    std::ostringstream photometricOss;
    std::vector<double> invLut;
    std::vector<double> destLut;
    photometricOss << std::setprecision(20) << std::showpoint;
    pixelTransform.emitGLSL(photometricOss, invLut, destLut);

/*
    // Experiment: measure speedup with TinyVector4 source images.
    typedef typename GpuNumericTraits<SrcValueType>::ImagePixelComponentType SrcImagePixelComponentType;
    typedef vigra::TinyVector<SrcImagePixelComponentType, 4> SrcVectorValueType;
    //class SrcVectorPixelType : public SrcVectorValueType {
    //public:
    //    SrcVectorPixelType() : SrcVectorValueType() { }
    //    SrcVectorPixelType(const SrcImagePixelComponentType & s) : SrcVectorValueType(s) { }
    //    SrcVectorPixelType(const vigra::RGBValue<SrcImagePixelComponentType> & s) : SrcVectorValueType(s.red(), s.green(), s.blue(), 0) { }
    //};
    //std::allocator<SrcVectorPixelType> al;
    //vigra::BasicImage<SrcVectorPixelType, std::allocator<SrcVectorPixelType> > srcVectorImage(srcSize, al);
    vigra::BasicImage<SrcVectorValueType> srcVectorImage(srcSize);
    //vigra::copyImage(src, destImage(srcVectorImage));
    vigra::copyImage(srcAlpha.first, srcAlpha.first + (src.second - src.first), srcAlpha.second, srcVectorImage.upperLeft(), vigra::VectorComponentAccessor<SrcVectorValueType>(3));

    transformImageGPUIntern(coordXformOss.str(),
                            interpolatorOss.str(),
                            interp.size,
                            srcSize,
                            srcVectorImage[0],
                            GpuNumericTraits<SrcVectorValueType>::ImageGLInternalFormat,
                            GpuNumericTraits<SrcVectorValueType>::ImageGLTransferFormat,
                            GpuNumericTraits<SrcVectorValueType>::ImageGLFormat,
                            GpuNumericTraits<SrcVectorValueType>::ImagePixelComponentGLType,
                            NULL,
                            XGL_BYTE,
                            destUL,
                            destSize,
                            dest.first[0],
                            GpuNumericTraits<DestValueType>::ImageGLInternalFormat,
                            GpuNumericTraits<DestValueType>::ImageGLTransferFormat,
                            GpuNumericTraits<DestValueType>::ImageGLFormat,
                            GpuNumericTraits<DestValueType>::ImagePixelComponentGLType,
                            alpha.first[0],
                            GpuNumericTraits<DestAlphaType>::ImagePixelComponentGLType,
                            warparound);
*/
    // Do remapping.
    // Give the GPU pointers directly to the image data, bypassing the vigra iterators and accessors.
    // This is cheating. It will not work if the iterators describe subsets of the images or if
    // the accessors perform computation.
#if 1
    transformImageGPUIntern(coordXformOss.str(),
                            interpolatorOss.str(),
                            interp.size,
                            photometricOss.str(),
                            invLut,
                            destLut,
                            srcSize,
                            src.first[0],
                            GpuNumericTraits<SrcValueType>::ImageGLInternalFormat,
                            GpuNumericTraits<SrcValueType>::ImageGLTransferFormat,
                            GpuNumericTraits<SrcValueType>::ImageGLFormat,
                            GpuNumericTraits<SrcValueType>::ImagePixelComponentGLType,
                            srcAlpha.first[0],
                            GpuNumericTraits<SrcAlphaType>::ImagePixelComponentGLType,
                            destUL,
                            destSize,
                            dest.first[0],
                            GpuNumericTraits<DestValueType>::ImageGLInternalFormat,
                            GpuNumericTraits<DestValueType>::ImageGLTransferFormat,
                            GpuNumericTraits<DestValueType>::ImageGLFormat,
                            GpuNumericTraits<DestValueType>::ImagePixelComponentGLType,
                            alpha.first[0],
                            GpuNumericTraits<DestAlphaType>::ImagePixelComponentGLType,
                            warparound);
#endif

#if 0
    //vigra::FRGBImage phonyDest(destSize);
    vigra::FVector4Image phonyDest(destSize);
    vigra::FImage phonyDestAlpha(destSize);
    vigra::FRGBImage phonySrc(srcSize);
    vigra::FImage phonySrcAlpha(srcSize);

    transformImageGPUIntern(oss.str(), srcSize, phonySrc[0], XGL_FLOAT, XGL_RGB,
                            phonySrcAlpha[0], XGL_FLOAT, XGL_RED,
                            destUL, destSize, phonyDest[0], XGL_FLOAT, XGL_RGBA,
                            phonyDestAlpha[0], XGL_FLOAT, XGL_RED,
                            warparound);

    // Test accuracy
    int xstart = destUL.x;
    int xend   = destUL.x + destSize.x;
    int ystart = destUL.y;
    int yend   = destUL.y + destSize.y;

    double samples = 0.0;
    double sumError = 0.0;
    double sumErrorSq = 0.0;

    // create dest y iterator
    //vigra::FRGBImage::traverser yd = phonyDest.upperLeft();
    vigra::FVector4Image::traverser yd = phonyDest.upperLeft();
    vigra::FImage::traverser ydm = phonyDestAlpha.upperLeft();
    DestImageIterator ydest(dest.first);
    AlphaImageIterator ydesta(alpha.first);

    int numMessages = 0;

    for(int y=ystart; y < yend; ++y, ++yd.y, ++ydm.y, ++ydest.y, ++ydesta.y)
    {
        // create x iterators
        //vigra::FRGBImage::traverser xd(yd);
        vigra::FVector4Image::traverser xd(yd);
        vigra::FImage::traverser xdm(ydm);
        DestImageIterator xdest(ydest);
        AlphaImageIterator xdesta(ydesta);

        for(int x=xstart; x < xend; ++x, ++xd.x, ++xdm.x, ++xdest.x, ++xdesta.x)
        {
            double sx,sy;
            bool result = transform.transformImgCoordPartial(sx,sy,x,y);
            float errorX = (*xd)[2] - static_cast<float>(sx);
            float errorY = (*xd)[1] - static_cast<float>(sy);
            double error = sqrt(errorX * errorX + errorY * errorY);

            if (error != 0.0 && numMessages < 150) {
                std::cout << std::setprecision(20) << "pos=[" << x << ", " << y << "] shouldBe=[" << sx << ", " << sy << " is=[" << (*xd)[2] << ", " << (*xd)[1] << "] error=" << error << std::endl;
                ++numMessages;
            }

            sumError += error;
            sumErrorSq += (error * error);

            samples += 1.0;

            dest.third.set(NumericTraits<DestValueType>::fromRealPromote(std::min(255.0, std::max(0.0, 255.0 * 10.0 * error))), xdest);
            alpha.second.set(255, xdesta);
        }
    }

    double avgError = sumError/samples;

    std::cout << "numSamples=" << samples << std::endl
              << "average error=" << avgError << std::endl
              << "stddev=" << (sumErrorSq/samples - avgError*avgError) << std::endl;
#endif

};


/** Transform an image into the panorama
 *
 *  Uses the GPU for processing.
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
void transformImageGPU(vigra::triple<SrcImageIterator, SrcImageIterator, SrcAccessor> src,
                       vigra::triple<DestImageIterator, DestImageIterator, DestAccessor> dest,
                       std::pair<AlphaImageIterator, AlphaAccessor> alpha,
                       vigra::Diff2D destUL,
                       TRANSFORM & transform,
                       PixelTransform & pixelTransform,
                       bool warparound,
                       Interpolator interpol,
                       AppBase::ProgressDisplay* progress)
{
    switch (interpol) {
    case INTERP_CUBIC:
	DEBUG_DEBUG("using cubic interpolator");
    transformImageGPUIntern(src, dest, alpha, transform, pixelTransform, destUL,
                                 vigra_ext::interp_cubic(), warparound,
                                 progress);
	break;
    case INTERP_SPLINE_16:
	DEBUG_DEBUG("interpolator: spline16");
    transformImageGPUIntern(src, dest, alpha, transform, pixelTransform, destUL,
                                 vigra_ext::interp_spline16(), warparound,
                                 progress);
	break;
    case INTERP_SPLINE_36:
	DEBUG_DEBUG("interpolator: spline36");
    transformImageGPUIntern(src, dest, alpha, transform, pixelTransform, destUL,
                                 vigra_ext::interp_spline36(), warparound,
                                 progress);
	break;
    case INTERP_SPLINE_64:
	DEBUG_DEBUG("interpolator: spline64");
    transformImageGPUIntern(src, dest, alpha, transform, pixelTransform, destUL,
                                 vigra_ext::interp_spline64(), warparound,
                                 progress);
	break;
    case INTERP_SINC_256:
	DEBUG_DEBUG("interpolator: sinc 256");
    transformImageGPUIntern(src, dest, alpha, transform, pixelTransform, destUL,
                                 vigra_ext::interp_sinc<8>(), warparound,
                                 progress);
	break;
    case INTERP_BILINEAR:
        transformImageGPUIntern(src, dest, alpha, transform, pixelTransform, destUL,
                                 vigra_ext::interp_bilin(), warparound,
                                 progress);
	break;
    case INTERP_NEAREST_NEIGHBOUR:
        transformImageGPUIntern(src, dest, alpha, transform, pixelTransform, destUL,
                                 vigra_ext::interp_nearest(), warparound,
                                 progress);
	break;
    case INTERP_SINC_1024:
        transformImageGPUIntern(src, dest, alpha, transform, pixelTransform, destUL,
                                 vigra_ext::interp_sinc<32>(), warparound,
                                 progress);
	break;
    }
}

/** Transform image, and respect a possible alpha channel */
template <class SrcImageIterator, class SrcAccessor,
          class SrcAlphaIterator, class SrcAlphaAccessor,
          class DestImageIterator, class DestAccessor,
          class AlphaImageIterator, class AlphaAccessor,
          class TRANSFORM, class PixelTransform>
void transformImageAlphaGPU(vigra::triple<SrcImageIterator, SrcImageIterator, SrcAccessor> src,
                            std::pair<SrcAlphaIterator, SrcAlphaAccessor> srcAlpha,
                            vigra::triple<DestImageIterator, DestImageIterator, DestAccessor> dest,
                            std::pair<AlphaImageIterator, AlphaAccessor> alpha,
                            vigra::Diff2D destUL,
                            TRANSFORM & transform,
                            PixelTransform & pixelTransform,
                            bool warparound,
                            Interpolator interpol,
                            AppBase::ProgressDisplay* progress)
{
    switch (interpol) {
    case INTERP_CUBIC:
	DEBUG_DEBUG("using cubic interpolator");
	transformImageAlphaGPUIntern(src,srcAlpha, dest, alpha, transform, pixelTransform, destUL,
				              vigra_ext::interp_cubic(), warparound,
                              progress);
	break;
    case INTERP_SPLINE_16:
	DEBUG_DEBUG("interpolator: spline16");
    transformImageAlphaGPUIntern(src,srcAlpha, dest, alpha, transform, pixelTransform, destUL,
                              vigra_ext::interp_spline16(), warparound,
		                      progress);
	break;
    case INTERP_SPLINE_36:
	DEBUG_DEBUG("interpolator: spline36");
    transformImageAlphaGPUIntern(src,srcAlpha, dest, alpha, transform, pixelTransform, destUL,
                              vigra_ext::interp_spline36(),  warparound,
		                      progress);
	break;
    case INTERP_SPLINE_64:
	DEBUG_DEBUG("interpolator: spline64");
    transformImageAlphaGPUIntern(src,srcAlpha, dest, alpha, transform, pixelTransform, destUL,
                              vigra_ext::interp_spline64(),  warparound,
		                      progress);
	break;
    case INTERP_SINC_256:
	DEBUG_DEBUG("interpolator: sinc 256");
    transformImageAlphaGPUIntern(src,srcAlpha, dest, alpha, transform, pixelTransform, destUL,
                              vigra_ext::interp_sinc<8>(), warparound,
		                      progress);
	break;
    case INTERP_BILINEAR:
        transformImageAlphaGPUIntern(src,srcAlpha, dest, alpha, transform, pixelTransform, destUL,
                              vigra_ext::interp_bilin(), warparound,
		                      progress);
	break;
    case INTERP_NEAREST_NEIGHBOUR:
        transformImageAlphaGPUIntern(src,srcAlpha, dest, alpha, transform, pixelTransform, destUL,
                              vigra_ext::interp_nearest(), warparound,
		                      progress);
	break;
    case INTERP_SINC_1024:
        transformImageAlphaGPUIntern(src,srcAlpha, dest, alpha, transform, pixelTransform, destUL,
                              vigra_ext::interp_sinc<32>(), warparound,
		                      progress);
	break;
    }
}

}; // namespace

#endif // _VIGRA_EXT_IMAGETRANSFORMSGPU_H
