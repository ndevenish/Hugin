// -*- c-basic-offset: 4 -*-
/** @file cms.h
*
*  functions to handle icc profiles in images
*
*  @author T. Modes
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
*  License along with this software. If not, see
*  <http://www.gnu.org/licenses/>.
*
*/

#ifndef _CMS_H
#define _CMS_H

#include "lcms2.h"

namespace HuginBase
{
namespace Color
{
/** converts given image with iccProfile to sRGB/gray space, need to give pixel type in lcms2 format 
 *  works with color and grayscale images */
template<class ImageType>
void ApplyICCProfile(ImageType& image, const vigra::ImageImportInfo::ICCProfile& iccProfile, const cmsUInt32Number imageFormat)
{
    // is color or grayscale image given?
    typedef typename vigra::NumericTraits<typename ImageType::value_type>::isScalar is_scalar;
    const bool isGrayscale(is_scalar().asBool);
    // create input icc profile
    cmsHPROFILE inputICC = NULL;
    if (!iccProfile.empty())
    {
        inputICC = cmsOpenProfileFromMem(iccProfile.data(), iccProfile.size());
    };
    if (inputICC != NULL)
    {
        // check that input profile matches image type
        if (isGrayscale)
        {
            if (cmsGetColorSpace(inputICC) != cmsSigGrayData)
            {
                cmsCloseProfile(inputICC);
                inputICC = NULL;
                return;
            };
        }
        else
        {
            if (cmsGetColorSpace(inputICC) != cmsSigRgbData)
            {
                cmsCloseProfile(inputICC);
                inputICC = NULL;
                return;
            };
        };
    };
    // if there is no icc profile in file do nothing
    if (inputICC == NULL)
    {
        return;
    };
    // build output profile
    cmsHPROFILE outputICC;
    if (isGrayscale)
    {
        // default grayscale curve with gamma of 2.2
        cmsToneCurve* Curve = cmsBuildGamma(NULL, 2.2);
        if (Curve == NULL)
        {
            return;
        };
        outputICC = cmsCreateGrayProfile(cmsD50_xyY(), Curve);
        cmsFreeToneCurve(Curve);
    }
    else
    {
        // sRGB for color images
        outputICC = cmsCreate_sRGBProfile();
    }
    // now build transform and do actual transformation
    cmsHTRANSFORM transform = cmsCreateTransform(inputICC, imageFormat, outputICC, imageFormat,
        INTENT_PERCEPTUAL, cmsFLAGS_BLACKPOINTCOMPENSATION);
    cmsDoTransform(transform, image.begin(), image.begin(), image.width()*image.height());
    // clean up
    cmsDeleteTransform(transform);
    cmsCloseProfile(inputICC);
    cmsCloseProfile(outputICC);
};

};  // namespace Color

}; // namespace HuginBase

#endif // _CMS_H
