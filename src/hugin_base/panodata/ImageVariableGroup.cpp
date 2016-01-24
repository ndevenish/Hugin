// -*- c-basic-offset: 4 -*-
/** @file ImageVariableGroup.cpp
 *
 *  @author James Legg
 * 
 * @brief Implement the ImageVariableGroup class.
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

#include <hugin_utils/utils.h>

#include "ImageVariableGroup.h"

namespace HuginBase
{

ConstImageVariableGroup::ConstImageVariableGroup(std::set<ImageVariableEnum> variables,
                                                 const PanoramaData & pano)
    : m_variables (variables),
      m_pano (pano)
{
    // work out the initial image number to part number matching.
    setPartNumbers();
}

ConstImageVariableGroup::~ConstImageVariableGroup()
{
}

unsigned int ConstImageVariableGroup::getPartNumber(unsigned int imageNr) const
{
    DEBUG_ASSERT(imageNr < m_image_part_numbers.size());
    DEBUG_ASSERT(m_image_part_numbers.size() == m_pano.getNrOfImages());
    return m_image_part_numbers[imageNr];
}

UIntSetVector ConstImageVariableGroup::getPartsSet() const
{
    UIntSetVector result(getNumberOfParts(), HuginBase::UIntSet());
    for (unsigned int imgNr = 0; imgNr<m_image_part_numbers.size(); ++imgNr)
    {
        result[m_image_part_numbers[imgNr]].insert(imgNr);
    };
    return result;
}

bool ConstImageVariableGroup::getVarLinkedInPart(ImageVariableEnum variable,
                                                 std::size_t part) const
{
    /* Variables can be linked in strange ways, but this ignores most of that.
     * For the sake of the user interface, a variable should be linked across
     * all images in the part, or not linked across any images in a part. This
     * function returns true in the first case and false in the second, but is
     * a bit weird about any other cases:
     * If the first image in the part has links in this variable, true is
     * returned.
     * If the second image in the part has no links in this variable, false is
     * returned.
     * If there is only one image in this part, true is returned.
     * This is just to make it fast. We could check explicitly for one of the
     * cases above, but they are probably all that is used.
     */
    
    // Have we found a unlinked image previously?
    bool found_first = false;
    switch (variable)
    {
#define image_variable( name, type, default_value )\
        case IVE_##name:\
            for (std::size_t imageNr = 0; imageNr < m_pano.getNrOfImages(); imageNr++)\
            {\
                if (m_image_part_numbers[imageNr] == part)\
                {\
                    if (!found_first)\
                    {\
                        found_first = true;\
                        if (m_pano.getImage(imageNr).name##isLinked())\
                        {\
                            return true;\
                        }\
                    } else {\
                        return false;\
                    }\
                }\
            }\
            break;
#include "image_variables.h"
#undef image_variable
    }
    // only one image found:
    return true;
}

void ImageVariableGroup::unlinkVariablePart(ImageVariableEnum variable,
                                            unsigned int partNr)
{
    DEBUG_ASSERT(m_variables.find(variable) != m_variables.end());
    // find all images in the requested part.
    for (unsigned int i = 0; i < m_image_part_numbers.size(); i++)
    {
        if (m_image_part_numbers[i] == partNr)
        {
            // unlink the variable
            switch (variable)
            {
#define image_variable( name, type, default_value )\
                case IVE_##name:\
                    m_pano.unlinkImageVariable##name(i);\
                    break;
#include "image_variables.h"
#undef image_variable
            }
        }
    }
    setPartNumbers();
}

void ImageVariableGroup::unlinkVariableImage(ImageVariableEnum variable,
                                             unsigned int imageNr)
{
    unlinkVariablePart(variable, m_image_part_numbers[imageNr]);
}

void ImageVariableGroup::linkVariablePart(ImageVariableEnum variable,
                                          unsigned int partNr)
{
    DEBUG_ASSERT(m_variables.find(variable) != m_variables.end());
    // find all images in the requested part.
    bool found_first_image = false;
    int first_image_number;
    for (unsigned int i = 0; i < m_image_part_numbers.size(); i++)
    {
        if (m_image_part_numbers[i] == partNr)
        {
            // make a note of the first image.
            if (!found_first_image)
            {
                first_image_number = i;
                found_first_image = true;
                continue;
            }
            // for the other images, link the variable to the first image.
            switch (variable)
            {
#define image_variable( name, type, default_value )\
                case IVE_##name:\
                    m_pano.linkImageVariable##name(first_image_number, i);\
                    break;
#include "image_variables.h"
#undef image_variable
            }
        }
    }
    setPartNumbers(); 
}

void ImageVariableGroup::linkVariableImage(ImageVariableEnum variable,
                                           unsigned int imageNr)
{
    linkVariablePart(variable, m_image_part_numbers[imageNr]);
}

void ImageVariableGroup::switchParts (unsigned int imageNr, unsigned int partNr)
{
    if (partNr == m_image_part_numbers[imageNr])
    {
        // We're asked to switch an image to its own part, achieving nothing:
        return;
    }
    DEBUG_TRACE("Switching image " << imageNr << " to part " << partNr);
    if (partNr > m_num_parts)
    {
        DEBUG_ERROR( "Request to switch an image to a nonexistent part." );
        return;
    }
    // find an image in this part.
    unsigned int part_image_index;
    for (part_image_index = 0; m_image_part_numbers[part_image_index] != partNr; part_image_index++);
    
    // Decide which variables to link.
    // Find which variables are linked in the other image.
    std::set<ImageVariableEnum> linked_variables;
    for(std::set<ImageVariableEnum>::iterator i = m_variables.begin(); i != m_variables.end(); ++i)
    {
        switch (*i)
        {
#define image_variable( name, type, default_value ) \
            case IVE_##name: \
                if(m_pano.getImage(part_image_index).name##isLinked())\
                {\
                    linked_variables.insert(IVE_##name);\
                }\
            break;
#include "image_variables.h"
#undef image_variable
        }
    }
    
    // If none share links, link them all. The image must be the only one of its
    // part.
    bool singular = linked_variables.empty();
    if (singular)
    {
        linked_variables = m_variables;
    }
    
    // unlink the image from the part it originally was part off.
    for(std::set<ImageVariableEnum>::iterator i = m_variables.begin(); i != m_variables.end(); ++i)
    {
        switch (*i)
        {
#define image_variable( name, type, default_value ) \
            case IVE_##name: \
                m_pano.unlinkImageVariable##name(imageNr);\
                break;
#include "image_variables.h"
#undef image_variable
        }
    }
    
    // link the variables
    for(std::set<ImageVariableEnum>::iterator i = linked_variables.begin(); i != linked_variables.end(); ++i)
    {
        switch (*i)
        {
            /* part_image_index and imageNr must be different, since if they
             * were the same, the image would have already been in the
             * correct part. This was the first thing we checked.
             */
#define image_variable( name, type, default_value ) \
            case IVE_##name: \
                m_pano.linkImageVariable##name(part_image_index, imageNr);\
                break;
#include "image_variables.h"
#undef image_variable
        }
    }
    
    // Changing the links inherits variable values from the new lens, so the
    // variable values may have changed and the linking certainly has.
    m_pano.imageChanged(imageNr);
    
    // update the mapping of image numbers to part numbers.
    setPartNumbers();
}

std::size_t ConstImageVariableGroup::getNumberOfParts() const
{
    return m_num_parts;
}

void ConstImageVariableGroup::updatePartNumbers()
{
    setPartNumbers();
}

void ConstImageVariableGroup::setPartNumbers()
{
    DEBUG_TRACE("")
    // Find links.
    m_image_part_numbers.clear();
    if (m_pano.getNrOfImages() == 0)
    {
        // no images.
        m_num_parts = 0;
        return;
    }
    /* We will keep a list of parts, containing the image number of the first
     * image that uses that part. When we want to find if another image is in
     * a part, we can then check if it is linked to any images in the list.
     */
    std::vector<std::size_t> parts_first_image;
    // image 0 always has part 0
    parts_first_image.push_back(0);
    m_image_part_numbers.push_back(0);
    for (std::size_t i = 1; i < m_pano.getNrOfImages(); i++)
    {
        // find a part for this image.
        /* We use parts_first_image.size() as a flag to determine when the
         * images is not linked to any part we have previously found, as it
         * will be the part number when we are done. */
        std::size_t part_number = parts_first_image.size();;
        for (std::size_t j = 0; j < parts_first_image.size(); j++)
        {
            // check each variable in the group
            for (std::set<ImageVariableEnum>::const_iterator k = m_variables.begin();
                 (k != m_variables.end()) && (part_number != j); ++k)
            {
                switch (*k)
                {
                    /** @todo Check for multiple parts. If there is some complex
                     * set of links, we may need to merge two parts together.
                     */
#define image_variable( name, type, default_value ) \
                    case IVE_##name:\
                        if (m_pano.getImage(i).name##isLinkedWith(m_pano.getImage(parts_first_image[j]))) \
                            part_number = j;\
                        break;
#include "image_variables.h"
#undef image_variable
                }
            }
        }
        // We should have a suitable part number for now.
        m_image_part_numbers.push_back(part_number);
        // If this is a new part, keep this image number to check links.
        if (part_number == parts_first_image.size())
        {
            parts_first_image.push_back(i);
        }
    }
    // set the number of parts
    m_num_parts = parts_first_image.size();
    
}

} // HuginBase namespace
