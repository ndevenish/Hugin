// -*- c-basic-offset: 4 -*-
/** @file LayoutRemapper.h
 *
 *  @author James Legg
 * 
 *  @brief Define a remapper to use in the layout mode.
 *
 *  This program is free software; you can redistribute it and/or
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

#ifndef LAYOUT_REMAPPER_H
#define LAYOUT_REMAPPER_H

#include "MeshRemapper.h"

/** Draw undistored images, but with the correct centre position. Must be
 * given a scale to draw with, using SetScale.
 */
class LayoutRemapper: public MeshRemapper
{
public:
    LayoutRemapper(HuginBase::Panorama *m_pano, HuginBase::SrcPanoImage * image,
                     VisualizationState *visualization_state);
    virtual void UpdateAndResetIndex();
    virtual bool GetNextFaceCoordinates(Coords *result);
    /** Set the size to draw the images.
     * 
     * Specify the maximum dimension you would like the image to have. 
     * The actual size of the image will have this maximum dimension, and the
     * same aspect ratio as the original image file.
     */
    void setScale(double scale);
private:
    double m_layoutScale;
    bool done;
    ArrayCoords face;
};

#endif
