// -*- c-basic-offset: 4 -*-
/** @file ImageVariableGroup.h
 *
 *  @author James Legg
 * 
 *  @brief Declare the ImageVariableGroup and ImageVariableGroupObserver
 * classes.
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

#ifndef IMAGE_VARIABLE_GROUP_H
#define IMAGE_VARIABLE_GROUP_H

// if this file is preprocessed for SWIG, we want to ignore
// all the header inclusions that follow:

#ifndef _HSI_IGNORE_SECTION

#include <set>
#include "Panorama.h"

#endif // _HSI_IGNORE_SECTION

namespace HuginBase {

/**
 * An ImageVariableGroup is a collection of image variables that can have some
 * shared variable values. It is useful to group the image variables to those
 * that are shared because of the same setup: for example lens variables,
 * sensor variables, exposures of particular brackets, positions of particular
 * stacks.
 * 
 * We can partition the set of images in a panorama based on shared (linked)
 * values for some of the variables in the group. (For example, we can Partion
 * the images by Lens given that a lens consists of a projection type,
 * horizontal field of view, and distortion coefficents.)
 * The parts are assigned a number. (e.g. Lens number)
 * We can also unlink a variable across a part, if it is part of
 * the group of variables (For example, it might be necessary to unlink the
 * centre shift of a particular lens).
 * Note that if all the variables in a part are unlinked, this is equivalent
 * to each image being in a separate part, which changes the part numbers.
 * We can also switch an image into a different partition by changing what its
 * variables are linked to.
 * 
 * Another object can listen to changes of part allocations in an
 * ImageVariableGroup, it should inherit from ImageVariableGroupObserver, and
 * register itself with the ImageVariableGroup(s) it wants to hear from.
 */
class IMPEX ConstImageVariableGroup
{
public:
    enum ImageVariableEnum {
#define image_variable( name, type, default_value ) \
IVE_##name, 
#include "image_variables.h"
#undef image_variable
    };
    /** constructor
     * 
     * Assign the ImageVariableGroup the image variables and the panorama that
     * it should be handling.
     */
    ConstImageVariableGroup(std::set<ImageVariableEnum> variables,
                            const PanoramaData & pano);
    
    /// destructor
    virtual ~ConstImageVariableGroup();
    
    /**
     * Get a part number from an image number.
     * 
     * @return a number unique to a subset in a partition of the images, such
     * that there is at least of the variables in this group linked.
     */
    unsigned int getPartNumber(unsigned int imageNr) const;
    
    /** get the number of parts.
     *
     * @return the number of unlinked parts in the associated panorama's images.
     */
    std::size_t getNumberOfParts();
    
    /** Get the linked status of a particular variable for a given part number.
     * @param variable the variable to check
     * @param part the part number to check
     * @return true if there is only one image in this part or the first image
     * in this part has links. False otherwise.
     * @note If there is only one image in the given part, true is returned.
     * This should make sense as variables are linked by default.
     */
    bool getVarLinkedInPart(ImageVariableEnum variable, std::size_t part) const;
    
    /** Update the part numbers, call this when the panorama changes.
     */
    void updatePartNumbers();
protected:
    // Member variables
    /// The set of variables which make up this group.
    std::set<ImageVariableEnum> m_variables;
    
    /// The panorama this group works on.
    const PanoramaData & m_pano;
    
    /** The part numbers for each image.
     * The image number is used as an index.
     */
    std::vector<unsigned int> m_image_part_numbers;
    
    unsigned int m_num_parts;
    
    // Member functions
    /**
     * Set the part numbers in m_image_part_numbers, and notify observers of
     * changes.
     * 
     * This should be called whenever the images change.
     */
    void setPartNumbers();
    
};

/** Same as above, but use a non const panorama.
 * 
 * This can be used for changing part numbers and linking and unlink of
 * variables.
 */
class IMPEX ImageVariableGroup: public ConstImageVariableGroup
{
public:
    /** constructor
     * 
     * Assign the ImageVariableGroup the image variables and the panorama that
     * it should be handling.
     */
    ImageVariableGroup(std::set<ImageVariableEnum> variables,
                       PanoramaData & pano)
            : ConstImageVariableGroup(variables, pano),
                m_pano (pano)
    {
    }
    
    /** @note Linking and unlinking variables in a part where there is only one
     * image has no effect, since there are no other images to be linked and
     * unlinked from.
     */
    
    /** unlink one of the variables across a given part.
     */
    void unlinkVariablePart(ImageVariableEnum variable, unsigned int partNr);
    
    /// unlink one the variables across the part containing a given image.
    void unlinkVariableImage(ImageVariableEnum variable, unsigned int imageNr);
    
    /// link one of the variables across a given part
    void linkVariablePart(ImageVariableEnum variable, unsigned int partNr);
    
    /// link one of the variables across a part containing a given image
    void linkVariableImage(ImageVariableEnum variable, unsigned int imageNr);
    
    /** switch a given image to a different part number.
     * 
     * The part numbers may change after this call, so it not necessarily the
     * case that the image has the given part number, but it is the case that
     * the part the image is in after the call has the properties of the part
     * specified as it was just before the call.
     * 
     * @note When switching part numbers, Variables are linked by default,
     * unless the variable is unlinked across multiple images in the part
     * specified, in which case those variables are unlinked in the new member
     * of that part.
     */
    void switchParts (unsigned int ImageNr, unsigned int partNr);
private:
    PanoramaData & m_pano;
};

} // HuginBase namespace

#endif
