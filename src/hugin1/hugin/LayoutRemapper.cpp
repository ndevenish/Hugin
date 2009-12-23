// -*- c-basic-offset: 4 -*-
/** @file LayoutRemapper.cpp
 *
 *  @author James Legg
 * 
 *  @brief Implement LayoutRemapper, a remapper to use in the layout mode.
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
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "LayoutRemapper.h"
#include "ViewState.h"

LayoutRemapper::LayoutRemapper(HuginBase::Panorama *m_pano,
                               unsigned int image_number,
                               ViewState *view_state)
        :MeshRemapper(m_pano, image_number, view_state),
         scale(0)
        
{
    // We'll never use any different texture coordinates, so record them now:
    face.tex_c[0][0][0] = 0.0;
    face.tex_c[0][0][1] = 0.0;
    face.tex_c[0][1][0] = 0.0;
    face.tex_c[0][1][1] = 1.0;
    face.tex_c[1][0][0] = 1.0;
    face.tex_c[1][0][1] = 0.0;
    face.tex_c[1][1][0] = 1.0;
    face.tex_c[1][1][1] = 1.0;
}

void LayoutRemapper::UpdateAndResetIndex()
{
    HuginBase::SrcPanoImage *src_img = view_state->GetSrcImage(image_number);
    
    // find the image size.
    double image_width = (double) src_img->getSize().width();
    double image_height = (double) src_img->getSize().height();
    
    // remap the middle of the image to find centre coordinates.
    double centre_x, centre_y;
    // create a transformation from source image to destination.
    transform.createInvTransform(*src_img, *(view_state->GetOptions()));
    transform.transformImgCoord(centre_x, centre_y,
                                image_width / 2.0, image_height / 2.0);
    /** @todo Offset the centre position for images in brackets, when showing
     * all brackets together.
     * 
     * I think a good offset would be about scale * bracket number / 5,
     * assuming bracket numbers are a sequence of consecutive integers starting
     * at 0.
     */
    
    // work out the size to draw the image
    bool landscape = image_width > image_height;
    double preview_width, preview_height;
    if (landscape)
    {
        preview_width = scale;
        preview_height = scale / image_width * image_height;
    } else {
        preview_height = scale;
        preview_width = scale / image_height * image_width;
    }
    
    // find bounds for image drawing
    double offset_x = preview_width / 2.0;
    double offset_y = preview_height / 2.0;
    double start_x = centre_x - offset_x;
    double start_y = centre_y - offset_y;
    double end_x = centre_x + offset_x;
    double end_y = centre_y + offset_y;
    
    face.vertex_c[0][0][0] = start_x;
    face.vertex_c[0][0][1] = start_y;
    face.vertex_c[0][1][0] = start_x;
    face.vertex_c[0][1][1] = end_y;
    face.vertex_c[1][0][0] = end_x;
    face.vertex_c[1][0][1] = start_y;
    face.vertex_c[1][1][0] = end_x;
    face.vertex_c[1][1][1] = end_y;
    
    // Specify our one face next time GetNextFaceCoordinates is called.
    done = false;
}

bool LayoutRemapper::GetNextFaceCoordinates(Coords *result)
{
    if (!done)
    {
        // point caller at our coordinates.
        result->tex_c = face.tex_c;
        result->vertex_c = face.vertex_c;
        // record that they have seen this face.
        done = true;
        return true;
    }
    // We've specified the one face required. No need for more.
    return false;
}

void LayoutRemapper::setScale(double scale_in)
{
    scale = scale_in;
}
