// -*- c-basic-offset: 4 -*-
/** @file impexImageAlpha.h
 *
 *  Routines to save images with alpha masks.
 *
 *  These routines handle the conversion of byte alpha
 *  channels into the final output types.
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

#ifndef VIGRA_EXT_IMPEX_ALPHA_IMAGE_H
#define VIGRA_EXT_IMPEX_ALPHA_IMAGE_H

#include <vigra/imageiterator.hxx>
#include <vigra/transformimage.hxx>
#include <vigra/initimage.hxx>
#include <vigra/numerictraits.hxx>

#include <vigra_impex2/impex.hxx>


#include "vigra_ext/FunctorAccessor.h"

namespace vigra_impex2 {


template <class T1, class T2>
struct GetAlphaScaleFactor;

// define the scale factors to map from the alpha channel type (T2)
// to valid alpha channels in image type (T1)
// T1: image type
// T2: alpha type
//  S: scale factor (given as type of T1).
#define VIGRA_EXT_GETALPHASCALE(T1,T2, S) \
template<> \
struct GetAlphaScaleFactor<T1, T2> \
{ \
    static vigra::NumericTraits<T1>::RealPromote get() \
    { \
	return S; \
    } \
};

// conversion from/to unsigned char
VIGRA_EXT_GETALPHASCALE(unsigned char, unsigned char, 1);
VIGRA_EXT_GETALPHASCALE(short, unsigned char, 128.498);
VIGRA_EXT_GETALPHASCALE(unsigned short, unsigned char, 257);
VIGRA_EXT_GETALPHASCALE(int, unsigned char, 8421504.49803922);
VIGRA_EXT_GETALPHASCALE(unsigned int, unsigned char, 16843009);
VIGRA_EXT_GETALPHASCALE(float, unsigned char, 1.0/255);
VIGRA_EXT_GETALPHASCALE(double, unsigned char, 1.0/255);

// conversion from/to unsigned short
VIGRA_EXT_GETALPHASCALE(unsigned char, unsigned short, 0.00389105058365759);
VIGRA_EXT_GETALPHASCALE(short, unsigned short, 0.499992370489052);
VIGRA_EXT_GETALPHASCALE(unsigned short, unsigned short, 1);
VIGRA_EXT_GETALPHASCALE(int, unsigned short, 32768.4999923705);
VIGRA_EXT_GETALPHASCALE(unsigned int, unsigned short, 65537);
VIGRA_EXT_GETALPHASCALE(float, unsigned short, 1.0/65535);
VIGRA_EXT_GETALPHASCALE(double, unsigned short, 1.0/65535);

// conversion from/to unsigned int
VIGRA_EXT_GETALPHASCALE(unsigned char,  unsigned int, 5.93718141455603e-08);
VIGRA_EXT_GETALPHASCALE(short,          unsigned int, 7.62916170238265e-06);
VIGRA_EXT_GETALPHASCALE(unsigned short, unsigned int, 1.52585562354090e-05);
VIGRA_EXT_GETALPHASCALE(int,            unsigned int, 0.499999999883585);
VIGRA_EXT_GETALPHASCALE(unsigned int,   unsigned int, 1);
VIGRA_EXT_GETALPHASCALE(float,          unsigned int, 1.0/4294967295.0);
VIGRA_EXT_GETALPHASCALE(double,         unsigned int, 1.0/4294967295.0);

#undef VIGRA_EXT_GETALPHASCALE

// scalar image
template<class SrcIterator, class SrcAccessor,
         class AlphaIterator, class AlphaAccessor>
void exportImageAlpha(vigra::triple<SrcIterator, SrcIterator, SrcAccessor> image,
		      std::pair<AlphaIterator, AlphaAccessor> alpha,
		      vigra_impex2::ImageExportInfo const & info,
		      vigra::VigraTrueType)
{
    typedef typename SrcAccessor::value_type image_type;
    typedef typename AlphaAccessor::value_type alpha_type;

    typedef typename vigra::NumericTraits<image_type>::RealPromote ScaleType;
    ScaleType scale =  GetAlphaScaleFactor<image_type, alpha_type>::get();
    std::cerr << " export alpha factor: " << scale << std::endl;

    // construct scaling accessor, for reading from the mask image
    typedef vigra_ext::ReadFunctorAccessor<vigra::ScalarIntensityTransform<image_type>,
	AlphaAccessor> ScalingAccessor;

    vigra::ScalarIntensityTransform<image_type> scaler(scale);
    ScalingAccessor scaleA(scaler,
                           alpha.second);

    // virtually merge image and mask
    typedef vigra_ext::MergeScalarScalar2VectorAccessor<SrcIterator,
	SrcAccessor, AlphaIterator, ScalingAccessor> MergeAccessor;

    MergeAccessor mergeA(image.first, image.third, alpha.first, scaleA);

    // do the export.
    // need to use a Coordinate iterator, because the
    // MergeAccessor requires coordinates, and not pointers to some memory
    // of the first image.
    exportImage(vigra::CoordinateIterator(),
                vigra::CoordinateIterator() + (image.second - image.first),
                mergeA, info);
}


// vector image
template<class SrcIterator, class SrcAccessor,
         class AlphaIterator, class AlphaAccessor>
void exportImageAlpha(vigra::triple<SrcIterator, SrcIterator, SrcAccessor> image,
		      std::pair<AlphaIterator, AlphaAccessor> alpha,
		      vigra_impex2::ImageExportInfo const & info,
		      vigra::VigraFalseType)
{
    typedef typename SrcAccessor::value_type image_type;
    typedef typename image_type::value_type component_type;
    typedef typename AlphaAccessor::value_type alpha_type;

    // get the correction factor
    typedef typename vigra::NumericTraits<component_type>::RealPromote ScaleType;
    ScaleType scale =  GetAlphaScaleFactor<component_type, alpha_type>::get();
    std::cerr << " export alpha factor: " << scale << std::endl;

    // construct scaling accessor.
    typedef vigra_ext::ReadFunctorAccessor<vigra::ScalarIntensityTransform<component_type>,
	AlphaAccessor> ScalingAccessor;

    vigra::ScalarIntensityTransform<component_type> scaler(scale);
    ScalingAccessor scaleA(scaler,
			   alpha.second);

    // virtually merge image and mask
    // this is a hack! it only works with 3 component images..
    // don't know how to get the size of a TinyVector at compile time
    typedef vigra_ext::MergeVectorScalar2VectorAccessor<SrcIterator,
	SrcAccessor, AlphaIterator, ScalingAccessor, 4> MergeAccessor;

    MergeAccessor mergeA(image.first, image.third, alpha.first, scaleA);

    // do the export.
    // need to use a Coordinate iterator, because the
    // MergeAccessor requires coordinates, and not pointers to some memory
    // of the first image.
    exportImage(vigra::CoordinateIterator(), vigra::CoordinateIterator(image.second - image.first),
		mergeA, info);
}


/** export an image with a differently typed alpha channel.
 *
 *  This function handles the merging of the images and the
 *  scales the alpha channel to the correct values.
 *
 *  can write to all output formats that support 4 channel images.
 *  (currently only png and tiff).
 */
template<class SrcIterator, class SrcAccessor,
         class AlphaIterator, class AlphaAccessor>
void exportImageAlpha(vigra::triple<SrcIterator, SrcIterator, SrcAccessor> image,
		      std::pair<AlphaIterator, AlphaAccessor> alpha,
		      vigra_impex2::ImageExportInfo const & info)
		      {
    typedef typename vigra::NumericTraits<typename SrcAccessor::value_type>::isScalar is_scalar;
    // select function for scalar, or vector image, depending on source type.
    // the alpha image has to be scalar all the time. stuff will break with strange
    // compile error if it isn't
    exportImageAlpha( image, alpha, info, is_scalar());
}


// vector image
template<class DestIterator, class DestAccessor,
         class AlphaIterator, class AlphaAccessor>
void importImageAlpha(vigra_impex2::ImageImportInfo const & info,
		      std::pair<DestIterator, DestAccessor> image,
		      std::pair<AlphaIterator, AlphaAccessor> alpha,
		      		      vigra::VigraFalseType)
{
    typedef typename DestAccessor::value_type image_type;
    typedef typename image_type::value_type component_type;
    typedef typename AlphaAccessor::value_type alpha_type;

    typedef typename vigra::NumericTraits<component_type>::RealPromote ScaleType;

    // get the correction factor
    ScaleType scale = vigra::NumericTraits<ScaleType>::one()/GetAlphaScaleFactor<component_type, alpha_type>::get();
    std::cerr << " import alpha factor: " << scale << std::endl;

    // construct scaling accessor.
    typedef vigra_ext::WriteFunctorAccessor<vigra::ScalarIntensityTransform<ScaleType>,
	AlphaAccessor> ScalingAccessor;

    vigra::ScalarIntensityTransform<ScaleType> scaler(scale);
    ScalingAccessor scaleA(scaler,
			   alpha.second);

    // virtually merge image and mask
    // this is a hack! it only works with 3 component images..
    // don't know how to get the size of a TinyVector at compile time
    typedef vigra_ext::SplitVectorNAccessor<DestIterator,
	DestAccessor, AlphaIterator, ScalingAccessor, 4> SplitRGBAccessor;

    typedef vigra_ext::SplitVectorNAccessor<DestIterator,
	DestAccessor, AlphaIterator, ScalingAccessor, 2> SplitAccessor;

    switch (image.second(image.first).size()) {
    case 1:
    {
        vigra_ext::SplitVectorNAccessor<DestIterator, DestAccessor, AlphaIterator, ScalingAccessor, 2>
            splitA(image.first, image.second, alpha.first, scaleA);

        importImage(info,
		vigra::CoordinateIterator(),
		splitA);
        break;
    }
    case 3:
    {
        vigra_ext::SplitVectorNAccessor<DestIterator, DestAccessor, AlphaIterator, ScalingAccessor, 4>
            splitA(image.first, image.second, alpha.first, scaleA);

        importImage(info,
		vigra::CoordinateIterator(),
		splitA);
        break;
    }
    default:
        vigra_fail("only 1 and 3 channel images supported by impexalpha.hxx");
    }
}

// scalar image
template<class DestIterator, class DestAccessor,
         class AlphaIterator, class AlphaAccessor>
void importImageAlpha(vigra_impex2::ImageImportInfo const & info,
		      std::pair<DestIterator, DestAccessor> image,
		      std::pair<AlphaIterator, AlphaAccessor> alpha,
		      vigra::VigraTrueType)
{
    typedef typename DestAccessor::value_type image_type;
    typedef typename AlphaAccessor::value_type alpha_type;

    typedef typename vigra::NumericTraits<image_type>::RealPromote ScaleType;
    
    // get the correction factor
    ScaleType scale = vigra::NumericTraits<ScaleType>::one()/GetAlphaScaleFactor<image_type, alpha_type>::get();

    std::cerr << " export alpha factor: " << scale << std::endl;

    // construct scaling accessor.
    typedef vigra_ext::WriteFunctorAccessor<vigra::ScalarIntensityTransform<ScaleType>,
	AlphaAccessor> ScalingAccessor;

    vigra::ScalarIntensityTransform<ScaleType> scaler(scale);
    ScalingAccessor scaleA(scaler,
			   alpha.second);

    // virtually merge image and mask
    // this is a hack! it only works with 3 component images..
    // don't know how to get the size of a TinyVector at compile time
    typedef vigra_ext::SplitVector2Accessor<DestIterator,
	DestAccessor, AlphaIterator, ScalingAccessor> SplitAccessor;

    SplitAccessor splitA(image.first, image.second, alpha.first, scaleA);

    // do the import
    // need to use a Coordinate iterator, because the
    // MergeAccessor requires coordinates, and not pointers to some memory
    // of the first image.
    importImage(info, vigra::CoordinateIterator(),
		splitA);
}


/** import an image with a differently typed alpha channel.
 *
 *  This function loads an image, and splits it into a
 *  color image and a separate alpha channel, the alpha channel
 *  should be a 8 bit image.
 *
 *  If the image doesn't contain any alpha channel, a completely
 *  white one is created.
 *
 *  can write to all output formats that support 4 channel images.
 *  (currently only png and tiff).
 */
template<class DestIterator, class DestAccessor,
         class AlphaIterator, class AlphaAccessor>
void importImageAlpha(vigra_impex2::ImageImportInfo const & info,
		      vigra::pair<DestIterator, DestAccessor> image,
		      std::pair<AlphaIterator, AlphaAccessor> alpha
		      )
{
    typedef typename vigra::NumericTraits<typename DestAccessor::value_type>::isScalar is_scalar;

    if (info.numExtraBands() == 1 ) {
	// import image and alpha channel
	importImageAlpha(info, image, alpha, is_scalar());
    } else if (info.numExtraBands() == 0 ) {
	// no alphachannel in file, import as usual.
	importImage(info, image);
	// fill alpha image
	vigra::initImage(alpha.first ,
                         alpha.first + vigra::Diff2D(info.width(), info.height()),
                         alpha.second,
                         255);
    } else {
	vigra_fail("Images with two or more alpha channel are not supported");
    }
}

} // namespace

#endif // VIGRA_EXT_IMPEX_ALPHA_IMAGE_H
