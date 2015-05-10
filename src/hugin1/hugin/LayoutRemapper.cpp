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
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifdef __WXMAC__
#include "panoinc_WX.h"
#include "panoinc.h"
#endif

#include <cmath>

#include "LayoutRemapper.h"
#include "ViewState.h"

LayoutRemapper::LayoutRemapper(HuginBase::Panorama *m_pano,
                               HuginBase::SrcPanoImage* image,
                               VisualizationState *visualization_state)
        :MeshRemapper(m_pano, image, visualization_state),
         m_layoutScale(0)
        
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
//    HuginBase::SrcPanoImage *src_img = visualization_state->GetSrcImage(image_number);
    
    // find the image size.
    double image_width = (double) image->getSize().width();
    double image_height = (double) image->getSize().height();
    
    // remap the middle of the image to find centre coordinates.
    double centre_x, centre_y;
    // create a transformation from source image to destination.
    transform.createInvTransform(*image, *(visualization_state->GetOptions()));
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
        preview_width = m_layoutScale;
        preview_height = m_layoutScale / image_width * image_height;
    } else {
        preview_height = m_layoutScale;
        preview_width = m_layoutScale / image_height * image_width;
    }
    
    // find bounds for image drawing
    double offset_x = preview_width / 2.0;
    double offset_y = preview_height / 2.0;
    
    // find the roll of the image.
    double angle = image->getRoll() * (M_PI / 180.0) + (M_PI / 2.0);
    double rsin = std::sin(angle);
    double rcos = std::cos(angle);
    
    double rsin_x = rsin * offset_x;
    double rcos_x = rcos * offset_x;
    double rsin_y = rsin * offset_y;
    double rcos_y = rcos * offset_y;
    
    face.vertex_c[0][0][0] = -rsin_x - rcos_y + centre_x;
    face.vertex_c[0][0][1] = rcos_x -rsin_y + centre_y;
    face.vertex_c[0][1][0] = -rsin_x + rcos_y + centre_x;
    face.vertex_c[0][1][1] = rcos_x + rsin_y + centre_y;
    face.vertex_c[1][0][0] = rsin_x - rcos_y + centre_x;
    face.vertex_c[1][0][1] = -rcos_x - rsin_y + centre_y;
    face.vertex_c[1][1][0] = rsin_x + rcos_y + centre_x;
    face.vertex_c[1][1][1] = -rcos_x + rsin_y + centre_y;
    
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
    m_layoutScale = scale_in;
}
