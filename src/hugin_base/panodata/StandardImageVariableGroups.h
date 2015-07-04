// -*- c-basic-offset: 4 -*-
/** @file StandardImageVariableGroups.h
 *
 *  @author James Legg
 * 
 *  @brief Somewhere to specify what variables belong to what.
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

#ifndef STANDARD_IMAGE_VARIABLE_GROUPS_H
#define STANDARD_IMAGE_VARIABLE_GROUPS_H

#include <hugin_shared.h>
#include "ImageVariableGroup.h"
#include "PanoramaData.h"

// remove when todo below is complete.
#include "Lens.h"

namespace HuginBase {

/** Make an ImageVariableGroup for lenses and other common concepts.
 *
 * Must be given a const panorama when created, this panorama remains with the
 * object its entire life, but will never be changed by the
 * StandardImageVariableGroups object. It can still react to changes in the
 * panorama, to update part numbers use UpdatePartNumbers().
 */
class IMPEX ConstStandardImageVariableGroups
{
public:
    /** constructor.
     * 
     * creates ImageVariableGroups for lenses, etc.
     * @param pano The panorama data to use for the lenses, etc.
     */
    explicit ConstStandardImageVariableGroups(const PanoramaData & pano);
    
    /** Get the ImageVariableGroup representing the group of lens variables.
     * 
     * Use this to manipulate lens variable links, and to find lens numbers.
     * @return a reference to the lens ImageVariableGroup.
     */
    ConstImageVariableGroup & getLenses()
    { return m_lenses; }
    
    /** Get the set of lens image variables.
     * 
     * @return A constant reference to the set of image variables used for the
     * lens ImageVariableGroup.
     */
    static const std::set<ConstImageVariableGroup::ImageVariableEnum> & getLensVariables();
    
    /** A panorama.getLens equivalent, not for new code.
     * 
     * This is a temporary solution to replace code that used Panorama.getLens.
     * It will generate a lens on the spot using the SrcPanoImg object. New code
     * should instead use the ImageVariableGroup returned by getLenses(), as it
     * does not generate the lens object on the fly.
     *
     * @param lens_number the number of the lens.
     * @return A lens object for the specified lens number. The lens number of
     * each image can be found by getLenses().getPartNumber(imageNr). Lens
     * numbers are allocated from 0, and the number of them is returned by
     * getLenses().getNumberOfParts().
     * @todo replace everything using this and remove the function.
     */ 
     Lens getLens(std::size_t lens_number);
     
    /** Get the lens object for a specific image, also not for new code.
     * 
     * Called by getLens.
     * 
     * @param imgNr the number of the image to generate a Lens object for.
     * @return a Lens object for the specified image.
     * @todo replace everything using this and remove the function.
     */
    Lens getLensForImage(std::size_t imgNr);
    
    /** Get the ImageVariableGroup representing the group of stack variables.
     * 
     * Use this to manipulate angle links, and to find stack numbers.
     * @return a reference to the stack ImageVariableGroup.
     */
    ConstImageVariableGroup & getStacks()
    { return m_stacks; }
    
    /** Get the set of stack image variables.
     * 
     * @return A constant reference to the set of image variables used for the
     * stack ImageVariableGroup. (yaw, pitch, roll, stack...)
     */
    static const std::set<ConstImageVariableGroup::ImageVariableEnum> & getStackVariables();
    
    /** Update part numbers for each variable group.
     * 
     * Should be called when the panorama images change
     */
    void update();
protected:
    /** the lens ImageVariableGroup.
     * 
     * @see StandardImageVariableGroups.cpp for a list of lens variables.
     */
    ConstImageVariableGroup m_lenses;
    
    /** the stack ImageVariableGroup.
     * 
     * @see StandardImageVariableGroups.cpp for a list of stack variables.
     */
    ConstImageVariableGroup m_stacks;
    
    const PanoramaData & m_pano;
};

class IMPEX StandardImageVariableGroups: public ConstStandardImageVariableGroups
{
public:
    explicit StandardImageVariableGroups(PanoramaData & pano);
    
    /** Get the ImageVariableGroup representing the group of lens variables.
     * 
     * Use this to manipulate lens variable links, and to find lens numbers.
     * @return a reference to the lens ImageVariableGroup.
     */
    ImageVariableGroup & getLenses()
    { return m_lenses; }
    
    /** Get the ImageVariableGroup representing the group of stack variables
     * 
     * Use this to manipulate angle links, and get stack numbers.
     * @return a reference to the stack ImageVariableGroup.
     */
    ImageVariableGroup & getStacks()
    { return m_stacks; }
    
    /** Update part numbers for each variable group.
     * 
     * Should be called when the panorama images change
     */
    void update();
protected:
        ImageVariableGroup m_lenses;
        ImageVariableGroup m_stacks;
        PanoramaData & m_pano;
};

} // namespace

#endif
