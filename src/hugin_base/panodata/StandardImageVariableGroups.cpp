// -*- c-basic-offset: 4 -*-
/** @file StandardImageVariableGroups.cpp
 *
 *  @author James Legg
 * 
 *  @brief Implement the StandardImageVariableGroups object.
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

#include <set>
#include "StandardImageVariableGroups.h"
#include "hugin_utils/utils.h"
#include "hugin_utils/stl_utils.h"
#include "ImageVariableTranslate.h"

namespace HuginBase {

/** The image variables that are specific to lenses. These are by default linked
 * across images in the same lens.
 * 
 * If you wish to change this, you'll need to set the ending offset in
 * lens_variables_set below.
 */
const ConstImageVariableGroup::ImageVariableEnum lens_variables_array[]
            = {ImageVariableGroup::IVE_Size,
               ImageVariableGroup::IVE_Projection,
               ImageVariableGroup::IVE_HFOV,
               ImageVariableGroup::IVE_ResponseType,
               ImageVariableGroup::IVE_EMoRParams,
               ImageVariableGroup::IVE_ExposureValue,
               ImageVariableGroup::IVE_Gamma,
               ImageVariableGroup::IVE_WhiteBalanceRed,
               ImageVariableGroup::IVE_WhiteBalanceBlue,
               ImageVariableGroup::IVE_RadialDistortion,
               ImageVariableGroup::IVE_RadialDistortionRed,
               ImageVariableGroup::IVE_RadialDistortionBlue,
               ImageVariableGroup::IVE_RadialDistortionCenterShift,
               ImageVariableGroup::IVE_Shear,
               ImageVariableGroup::IVE_VigCorrMode,
               ImageVariableGroup::IVE_FlatfieldFilename,
               ImageVariableGroup::IVE_RadialVigCorrCoeff,
               ImageVariableGroup::IVE_RadialVigCorrCenterShift
            };

/** A set containing the lens image variables.
 * 
 *  the offset on the second construtor argument is the size of the array.
 */
const std::set<ConstImageVariableGroup::ImageVariableEnum> lens_variables_set =
    std::set<ConstImageVariableGroup::ImageVariableEnum>(lens_variables_array,
                                                         lens_variables_array + 18);


/** The image variables that are specific to stack. These are by default linked
 * across images in the same stack.
 * 
 * If you wish to change this, you'll need to set the ending offset in
 * stack_variables_set below.
 */
const ConstImageVariableGroup::ImageVariableEnum stack_variables_array[]
            = {ImageVariableGroup::IVE_Yaw,
               ImageVariableGroup::IVE_Pitch,
               ImageVariableGroup::IVE_Roll,
               ImageVariableGroup::IVE_Stack,
               ImageVariableGroup::IVE_X,
               ImageVariableGroup::IVE_Y,
               ImageVariableGroup::IVE_Z,
               ImageVariableGroup::IVE_TranslationPlaneYaw,
               ImageVariableGroup::IVE_TranslationPlanePitch
            };

/** A set containing the stack image variables.
 * 
 *  the offset on the second construtor argument is the size of the array.
 */
const std::set<ConstImageVariableGroup::ImageVariableEnum> stack_variables_set =
    std::set<ConstImageVariableGroup::ImageVariableEnum>(stack_variables_array,
                                                         stack_variables_array + 9);

// constructor
ConstStandardImageVariableGroups::ConstStandardImageVariableGroups(const PanoramaData &pano)
        :   m_lenses (lens_variables_set, pano), // initialise lenses.
            m_stacks (stack_variables_set, pano), // initalise stacks.
            m_pano (pano)
{
}

const std::set<ConstImageVariableGroup::ImageVariableEnum> &
ConstStandardImageVariableGroups::getLensVariables()
{
    return lens_variables_set;
}

const std::set<ConstImageVariableGroup::ImageVariableEnum> &
ConstStandardImageVariableGroups::getStackVariables()
{
    return stack_variables_set;
}

Lens ConstStandardImageVariableGroups::getLens(std::size_t lens_number)
{
    // find an image with the requested lens number.
    DEBUG_ASSERT(lens_number < m_lenses.getNumberOfParts());
    std::size_t image_number;
    std::size_t number_of_images = m_pano.getNrOfImages();
    for (image_number = 0; image_number < number_of_images; image_number++)
    {
        if (m_lenses.getPartNumber(image_number) == lens_number)
        {
            return getLensForImage(image_number);
        }
    }
    DEBUG_ERROR("Cannot find an image with requested lens number.");
    DEBUG_ASSERT(false);
    return getLensForImage(0);
}

Lens ConstStandardImageVariableGroups::getLensForImage(std::size_t image_number)
{
    Lens result;
    const SrcPanoImage & image = m_pano.getImage(image_number);
    result.setProjection((Lens::LensProjectionFormat) image.getProjection());
    result.setImageSize(image.getSize());
    // set the sensor size by using the crop factor
    result.setCropFactor(image.getExifCropFactor());
    /* Convert the lens image variables into a map of lens variables.
     * We make a tempory VariableMap for rach lens image variable. The Panorama
     * tools script codes map to the values in this. We then use the name and
     * value to set the apropriate value in the lens' variables field. If the
     * variable has any links, they must be links to the same lens, so we
     * specify this as a linked variable.
     */
#define image_variable( name, type, default_value)\
    {\
        VariableMap temp_vars;\
        if (set_contains(lens_variables_set, ImageVariableGroup::IVE_##name))\
        {\
            PTOVariableConverterFor##name::addToVariableMap(image.get##name##IV(), temp_vars);\
            for (VariableMap::iterator tvi = temp_vars.begin(); tvi != temp_vars.end(); tvi++)\
            {\
                LensVariable & lens_var = map_get(result.variables, tvi->first);\
                lens_var.setValue(tvi->second.getValue());\
                lens_var.setLinked(image.name##isLinked());\
            }\
        }\
    }
#include "image_variables.h"
#undef image_variable
    return result;
}

void ConstStandardImageVariableGroups::update()
{
    // update all the ImageVariablesGroup object's part numbers.
    m_lenses.updatePartNumbers();
    m_stacks.updatePartNumbers();
}

void StandardImageVariableGroups::update()
{
    // update all the ImageVariablesGroup object's part numbers.
    m_lenses.updatePartNumbers();
    m_stacks.updatePartNumbers();
    /* There are two m_lenses objects, one ConstImageVariablesGroup and one
     * ImageVariableGroup. We should update both of them so the inherited
     * functions can continue using Const version.
     */
    ConstStandardImageVariableGroups::update();
}


StandardImageVariableGroups::StandardImageVariableGroups(PanoramaData &pano)
        :   ConstStandardImageVariableGroups(pano),
            m_lenses (lens_variables_set, pano), // initialise lenses.
            m_stacks (stack_variables_set, pano), // initialise stacks.
            m_pano (pano)
{
}

}// namespace HuginBase
