// -*- c-basic-offset: 4 -*-

/** @file TexCoordRemapper.h
 *
 *  @author James Legg
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

/* A TexCoordRemapper uses the reamapping transformations to create a set of
 * quadrilatrials that approximate the remapping. Each quad represents a small
 * axis-aligned rectangle of the output panorama.
 *
 * The mapping works by setting the texture coordinates of the input image to
 * reflect the output one.
 */

#ifndef __TEXCOORDREMAPPER_H
#define __TEXCOORDREMAPPER_H

#include "MeshRemapper.h"
#include <vector>
#include "hugin_math/hugin_math.h"

class TexCoordRemapper: public MeshRemapper
{
public:
    TexCoordRemapper(HuginBase::Panorama *m_pano, HuginBase::SrcPanoImage * image,
                     VisualizationState *visualization_state);
    virtual void UpdateAndResetIndex();
    virtual bool GetNextFaceCoordinates(Coords *result);
private:
    void SetSize();
    // this stores all the coordinates, and can return rows of them.
    // arrays don't meet the requirements for std::vector, so we use a pair.
    std::vector< std::vector<hugin_utils::FDiff2D> > map;
    // dimensions for the area we cover. This can be used for clipping.
    double start_x, start_y, end_x, end_y;
    // this is the number of vertices we use in each direction.
    unsigned int divisions_x, divisions_y;
    // the size of the output
    double o_width, o_height;
    // the size of each face
    double face_width;
    double face_height;
    // these are used to index faces between calls of GetNextFaceCoordinates
    unsigned int face_index, number_of_faces;
    double vertex_coords[2][2][2];
    double texture_coords[2][2][2];
};

#endif


