// -*- c-basic-offset: 4 -*-
/** @file image_variables.h
 *
 *  @author James Legg
 * 
 *  @brief This file specifies what image variables SrcPanoImg should have.
 *
 * This file is #included multiple times after defining a macro for
 * image_variable. The idea is to reduce the amount of tedoius code copy &
 * pasted for each image variable. The list will be used to generate things the
 * following:
 * -# Member variables:
 *          ImageVariable<[type]> m_[name];
 * -# Accessor functions:
 *          [type] get[name]() const;
 * -# Set functions:
 *          void set[name](const [type] data);
 * -# Link functions:
 *          void link[name](SrcPanoImg)
 * -# Unlink functions:
 *          void unlink[name]();
 * @par
 * The arguments work as follows:
 * -# the name of the variable
 * -# the type of the variable
 * -# the default value
 * 
 * @par
 * There is some non automatic stuff in ImageVariableTranslate.h that will need
 * changing if this list changes. That file handles the translation to and from
 * PTO file format style variables.
 * @see ImageVariableTranslate.h
 */

/*  This is free software; you can redistribute it and/or
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

/* Hmmm... I'ld like commas in template arguments, but < and > aren't real
 * brackets so the template arguments will be interprated as separate macro
 * arguments instead.
 * Solution: Have an object in ImageVariableTranslate.h which handles all the
 * pto conversion code. We can generate the code in SrcPanoImg.h automatically
 * as ImageVariableTranslate.h typedefs PTOVariableConverterFor[name] for each
 * variable.
 * So we don't have a parameter that specifies the codes to use in a PTO file.
 */

// file variables
image_variable( Filename, std::string, "" )
image_variable( Size, vigra::Size2D , vigra::Size2D(0,0) )
// projection variables
image_variable( Projection, HuginBase::BaseSrcPanoImage::Projection, RECTILINEAR )
image_variable( HFOV, double, 50.0 )

// colour response variables
image_variable( ResponseType, HuginBase::BaseSrcPanoImage::ResponseType, RESPONSE_EMOR )
image_variable( EMoRParams, std::vector<float>, std::vector<float>(5, 0.0) )
image_variable( ExposureValue, double, 0.0 )
image_variable( Gamma, double, 1.0 )
image_variable( WhiteBalanceRed, double, 1.0 )
image_variable( WhiteBalanceBlue, double, 1.0 )

// orientation in degrees
image_variable( Roll, double , 0.0 )
image_variable( Pitch, double , 0.0 )
image_variable( Yaw, double, 0.0 )

image_variable( X, double , 0.0 )
image_variable( Y, double , 0.0 )
image_variable( Z, double, 0.0 )

image_variable( TranslationPlaneYaw, double, 0.0)
image_variable( TranslationPlanePitch, double, 0.0)
// stack information
// Currently only the link information is used, the value means nothing.
image_variable( Stack, double, 0.0 )

// radial lens distortion
image_variable( RadialDistortion, std::vector<double>, distortion_default )

// radial lens distortion (red, blue channel), for TCA correction
image_variable( RadialDistortionRed, std::vector<double>, distortion_default )
image_variable( RadialDistortionBlue, std::vector<double>, distortion_default )

// Center shift
image_variable( RadialDistortionCenterShift, hugin_utils::FDiff2D, hugin_utils::FDiff2D(0.0, 0.0) )

// shear
image_variable( Shear, hugin_utils::FDiff2D, hugin_utils::FDiff2D(0, 0)  )

// crop description
image_variable( CropMode, HuginBase::BaseSrcPanoImage::CropMode, NO_CROP )
image_variable( CropRect, vigra::Rect2D, vigra::Rect2D(0, 0, 0, 0) )
image_variable( AutoCenterCrop, bool, true )

// vignetting correction
image_variable( VigCorrMode, int, VIGCORR_RADIAL|VIGCORR_DIV )

// coefficients for vignetting correction (even degrees: 0,2,4,6, ...)
image_variable( FlatfieldFilename, std::string, "" )
image_variable( RadialVigCorrCoeff, std::vector<double>, RadialVigCorrCoeff_default )
image_variable( RadialVigCorrCenterShift, hugin_utils::FDiff2D, hugin_utils::FDiff2D(0.0, 0.0) )

// linear pixel transform
// (doesn't seem to be used, removing with #if 0)
#if 0
image_variable( ka, std::vector<double>, ,  )
image_variable( kb, std::vector<double>, ,  )
#endif

// store camera information from exif tags...
image_variable( ExifModel, std::string, "" )
image_variable( ExifMake, std::string, "" )
image_variable( ExifLens, std::string, "" )
image_variable( ExifCropFactor, double, 0 )
image_variable( ExifFocalLength, double, 0 )
image_variable( ExifOrientation, double, 0 )
image_variable( ExifAperture, double, 0 )
image_variable( ExifISO, double, 0 )
image_variable( ExifDistance, double, 0 )
image_variable( ExifFocalLength35, double, 0)
image_variable( ExifExposureTime, double, 0)
image_variable( ExifDate, std::string, "")
image_variable( ExifExposureMode, int, 0)

#if 0
//
// panotools options, currently not used
//
// width of feather for stitching.
image_variable( FeatherWidth, unsigned int, 10 )
// Morph-to-fit using control points.
image_variable( Morph, bool, false )
#endif

// mask handling
// Masks is list of loaded or created textures
image_variable( Masks, HuginBase::MaskPolygonVector, defaultMaskVector)
// ActiveMasks contains list of all negative masks, which should be applied to
// a given image, this is used to propagate positive masks
image_variable( ActiveMasks, HuginBase::MaskPolygonVector, defaultMaskVector)

// If the image is selected to be used in the preview and for optimisation.
image_variable( Active, bool, true )

