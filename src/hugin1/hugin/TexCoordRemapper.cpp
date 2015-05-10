// -*- c-basic-offset: 4 -*-

/** @file TexCoordRemapper.cpp
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

#ifdef __WXMAC__
#include "panoinc_WX.h"
#include "panoinc.h"
#endif

#include "TexCoordRemapper.h"
#include "algorithms/nona/ComputeImageROI.h"
#include "ViewState.h"

// higher values make the mesh more detailed, but slower and using more memory:
// Value is in faces per pixel in each direction, so it should be positive and
// less than 1. Faces will be around this size, approximately square.
const double mesh_frequency = 0.07;

TexCoordRemapper::TexCoordRemapper(HuginBase::Panorama *m_pano,
                                   HuginBase::SrcPanoImage * image,
                                   VisualizationState *visualization_state)
                  : MeshRemapper(m_pano, image, visualization_state)
{
    
}

void TexCoordRemapper::UpdateAndResetIndex()
{
    // work what area we should cover in what detail.
    SetSize();
    // we want to make a remapped mesh, get the transformation we need:
//    HuginBase::SrcPanoImage *src_img = visualization_state->GetSrcImage(image_number);
    transform.createTransform(*image, *(visualization_state->GetOptions()));
//    DEBUG_INFO("updating mesh for image " << image_number
//              << ", using faces spaced about " << scale << " units apart.\n");
    // fill the map with transformed points.
    for (unsigned int x = 0; x < divisions_x; x++)
    {
        for (unsigned int y = 0; y < divisions_y; y++)
        {
            transform.transformImgCoord(map[x][y].x,
                                        map[x][y].y,
                                        (double) x * face_width + start_x,
                                        (double) y * face_height + start_y);
            // texture coordinates on the image range from 0 to 1.
            map[x][y].x /= width;
            map[x][y].y /= height;
        }
    }
    face_index = 0;
    SetCrop();
}

bool TexCoordRemapper::GetNextFaceCoordinates(Coords *result)
{
    result->tex_c = texture_coords;
    result->vertex_c = vertex_coords;
    // return any remaining results of a previous clipping operation.
    if (GiveClipFaceResult(result)) return true;
    // try to find a face that is at least partly covered by the image.
    while (true)
    {
        if (face_index == number_of_faces) return false;
        unsigned int x_f = face_index % (divisions_x - 1),
                     y_f = face_index / (divisions_x - 1);
        bool all_left = true, all_right = true,
             all_above = true, all_below = true;
        for (unsigned short int x = 0; x < 2; x++)
        {
            for (unsigned short int y = 0; y < 2; y++)
            {
                unsigned int xt = x_f + x, yt = y_f + y;
                if (map[xt][yt].x > crop_x1) all_left = false;
                if (map[xt][yt].x < crop_x2) all_right = false;
                if (map[xt][yt].y > crop_y1) all_above = false;
                if (map[xt][yt].y < crop_y2) all_below = false;
            }
        }
        /* check if this quad shows any of the input image.
         * We could possibly drop some more faces, but this is a pretty good
         * optimisation by itself. Proper clipping will alert us otherwise.
         */
        if (!(all_left || all_right || all_above || all_below)) break;
        face_index++;
    }
    // now set the coordinates.
    unsigned int x_f = face_index % (divisions_x - 1),
                 y_f = face_index / (divisions_x - 1);
    for (unsigned short int x = 0; x < 2; x++)
    {
        for (unsigned short int y = 0; y < 2; y++)
        {
            unsigned int xt = x_f + x, yt = y_f + y;
            result->tex_c[x][y][0] = map[xt][yt].x;
            result->tex_c[x][y][1] = map[xt][yt].y;
            result->vertex_c[x][y][0] = (double) xt * face_width + start_x;
            result->vertex_c[x][y][1] = (double) yt * face_height + start_y;
        }
    }
    face_index++;
    /* Since we only crop to convex regions, having all four points inside the
     * wanted region implies we don't need to do any clipping. It should be
     * faster to test for this and skip full clipping in that case, as the vast
     * majority of faces will not need any clipping or fail the test above.
     */
//    HuginBase::SrcPanoImage *src_img = visualization_state->GetSrcImage(image_number);
    if (   image->isInside(vigra::Point2D(int(result->tex_c[0][0][0] * width),
                                            int(result->tex_c[0][0][1] * height)))
        && image->isInside(vigra::Point2D(int(result->tex_c[0][1][0] * width),
                                            int(result->tex_c[0][1][1] * height)))
        && image->isInside(vigra::Point2D(int(result->tex_c[1][0][0] * width),
                                            int(result->tex_c[1][0][1] * height)))
        && image->isInside(vigra::Point2D(int(result->tex_c[1][1][0] * width),
                                            int(result->tex_c[1][1][1] * height))))
    {
        // all inside, doesn't need clipping.
        /* FIXME Alber's equal area conic projection needs to be clipped to the
         * sides space in the output that maps to the panorama...
         */
        return true;
    }
    /* We have to clip the face to the source image. This may produce many faces
     * or none, so we store a list and pop elements off it until the list is
     * empty, or try from the top in the case we get none.
     */ 
    ClipFace(result);
    if (GiveClipFaceResult(result))
    {
        return true;
    } else {
        return GetNextFaceCoordinates(result);
    }
}

void TexCoordRemapper::SetSize()
{
//    const HuginBase::SrcPanoImage *src = visualization_state->GetSrcImage(image_number);
    width = (double) image->getSize().width();
    height = (double) image->getSize().height();
    // set the bounding rectangle.
    // FIXME
    // 1. If there is an efficient way to find a good bounding rectangle, use it
    //            (I had a look at ComputeImageROI but seemed a bit brute force)
    // 2. With zooming, we could clip the stuff off the edge of the screen.
    // For now we stick with everything that is visible.
    vigra::Rect2D visible_area = visualization_state->GetVisibleArea();
    start_x = (double) visible_area.left() - 0.5;
    start_y = (double) visible_area.top() - 0.5;
    end_x = (double) visible_area.right() - 0.5;
    end_y = (double) visible_area.bottom() - 0.5;
    o_width = end_x - start_x;
    o_height = end_y - start_y;
    // use the scale to determine edge lengths in pixels for subdivision
    scale = visualization_state->GetScale() * mesh_frequency;
    // round the number of divisions we need to get a whole number of faces
    divisions_x = (int) ((end_x - start_x) * scale + 0.5);
    if (divisions_x < 2) divisions_x = 2;
    divisions_y = (int) ((end_y - start_y) * scale + 0.5);
    if (divisions_y < 2) divisions_y = 2;
    // the face height and width uses the rounded number, we don't want gaps at
    // the edges of the panorama. Therefore scale is approximate now.
    /* FIXME there is a line  on the bottom edge when an image covers the top
     * pole in equirectangular sometimes. These will get clipped, but it means
     *  extra stuff is being done along a wrong edge.
     */
    // the minus 1 is because we need the last division to cover the far edge.
    // note we have divisions_x - 1 faces to cover the divisions_x vertices.
    face_width = o_width / (double) (divisions_x - 1);
    face_height = o_height / (double) (divisions_y - 1);
    // work out the number of faces.
    number_of_faces = (divisions_x - 1) * (divisions_y - 1);
    // resize our data stucture for holding the vertex locations.
    map.resize(divisions_x);
    for (unsigned  int column = 0; column < divisions_x; column++)
    {
        map[column].resize(divisions_y);
    }
}

