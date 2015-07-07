// -*- c-basic-offset: 4 -*-

/** @file MeshRemapper.cpp
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

#include <math.h>
#include "MeshRemapper.h"
#include "ViewState.h"

MeshRemapper::MeshRemapper(HuginBase::Panorama *m_pano_in,
                           HuginBase::SrcPanoImage * image_in,
                           VisualizationState *visualization_state_in)
{
    m_pano = m_pano_in;
    image = image_in;
    visualization_state = visualization_state_in;
}

MeshRemapper::~MeshRemapper()
{
}

void MeshRemapper::UpdateAndResetIndex()
{
    // we want to make a remapped mesh, get some generic information:
//    const HuginBase::SrcPanoImage *src = visualization_state->GetSrcImage(image_number);
    // get the size of the image.
    width = (double) image->getSize().width();
    height = (double) image->getSize().height();
  
    // use the scale to determine edge lengths in pixels for any
    // resolution selection.
    scale = visualization_state->GetScale();
    // It is up to the child classes to determine what to do here. They should
    // probably use set up transform and use it to fill some data structure that
    // stores coordinates.
}

void MeshRemapper::SetCrop()
{
//    const HuginBase::SrcPanoImage *src = visualization_state->GetSrcImage(image_number);
    crop_mode = image->getCropMode();
    crop_x1 = (double) image->getCropRect().left() / width;
    crop_x2 = (double) image->getCropRect().right() / width;
    crop_y1 = (double) image->getCropRect().top() / height;
    crop_y2 = (double) image->getCropRect().bottom() / height;
    // set variables for circular crop.
    circle_crop = image->getCropMode() == HuginBase::SrcPanoImage::CROP_CIRCLE;
    if (circle_crop)
    {
        circle_crop_centre_x = (crop_x1 + crop_x2) / 2.0;
        circle_crop_centre_y = (crop_y1 + crop_y2) / 2.0;
        double crop_width_px = (double) image->getCropRect().width(),
               crop_hieght_px = (double) image->getCropRect().height(),
               crop_radius = (crop_width_px < crop_hieght_px ?
                                crop_width_px : crop_hieght_px) / 2.0;
        circle_crop_radius_x = crop_radius / width;
        circle_crop_radius_y = crop_radius / height;
    }
    // hugin allows negative cropping regions, but we are expected to only
    // output the regions that exist in the original image, so clamp the values:
    if (crop_x1 < 0.0) crop_x1 = 0.0; if (crop_x1 > 1.0) crop_x1 = 1.0;
    if (crop_x2 < 0.0) crop_x2 = 0.0; if (crop_x2 > 1.0) crop_x2 = 1.0;
    if (crop_y1 < 0.0) crop_y1 = 0.0; if (crop_y1 > 1.0) crop_y1 = 1.0;
    if (crop_y2 < 0.0) crop_y2 = 0.0; if (crop_y2 > 1.0) crop_y2 = 1.0;
}

/******************************************************************************
 * Clipping stuff                                                             *
 ******************************************************************************/
 
// a vertex has vertex coordinates and a texture coordinates.
class Vertex
{
public:
    Vertex(double vx, double vy, double tx, double ty);
    Vertex() {}
    double vertex_c[2];
    double tex_c[2];
};

Vertex::Vertex(double vx, double vy, double tx, double ty)
{
    vertex_c[0] = vx;
    vertex_c[1] = vy;
    tex_c[0] = tx;
    tex_c[1] = ty;
}

// The A_Polygon class stores an arbitrary polygon, and can give you another
// polygon that is the result of clipping it with some line.
// hmmm... Using the name "Polygon" causes errors on windows, hence A_Polgon.
class A_Polygon
{
public:
    // Create the polygon from a quadrilateral
    explicit A_Polygon(MeshRemapper::Coords *face);
    A_Polygon() {}
    // clip lines are the line defined by x * p[0] + y * p[1] + p[2] = 0.
    // inside is the side where x * p[0] + y *p[1] + p[2] > 0.
    A_Polygon Clip(const double clip_line[3]);
    // add a vertex on the end of the list. Used to build the polygon when not
    // created from a quad.
    inline void AddVertex(Vertex v);
    // convert to a list of quads. A bit approximate.
    std::vector<MeshRemapper::ArrayCoords> ConvertToQuads();
private:
    // our list of vertices
    std::vector<Vertex> verts;
    // Test which side of the clip plane we are on
    inline bool Inside(const unsigned int vertex,
                       const double clip_line[3]) const;
    // given that the edge between the specified vertices crosses the clip line,
    // find where they cross and set the vertex and texture coordinates there.
    Vertex Intersect(const unsigned int v1, const unsigned int v2,
                     const double clip_line[3]) const;
};

A_Polygon::A_Polygon(MeshRemapper::Coords *face)
{
    verts.resize(4);
    for (unsigned short int v = 0; v < 4; v++)
    {
        // give verticies counter-clockwise
        unsigned short int x = (v == 2 || v==3) ? 1 : 0,
                           y = (v == 1 || v==2) ? 1 : 0;
        for (unsigned short int a = 0; a < 2; a++)
        {
            verts[v].vertex_c[a] = face->vertex_c[x][y][a];
            verts[v].tex_c[a]    = face->tex_c[x][y][a];
        }
    }
}

inline void A_Polygon::AddVertex(Vertex v)
{
    verts.push_back(v);
}

inline bool A_Polygon::Inside(const unsigned int v, const double l[3]) const
{
    return verts[v].tex_c[0] * l[0] + verts[v].tex_c[1] * l[1] + l[2] > 0;
}

Vertex A_Polygon::Intersect(const unsigned int v1_index,
                          const unsigned int v2_index,
                          const double cl[3]) const
{
    // find the point of intersection from the given edge with a clip plane.
    // the edge is the vertex with the same number and the one before it.
    // Get pointers to the vertecies we will use
    const Vertex *v1 = &verts[v1_index],
                 // the vertex before is the last one if we at the beginning.
                 *v2 = &verts[v2_index];
    // find a vector along the path of the edge we are clipping
    double dx = v2->tex_c[0] - v1->tex_c[0],
           dy = v2->tex_c[1] - v1->tex_c[1];
    // The line along the edge we clip is defined by (x,y) = v1 + t(dx, dy)
    // The line of the clipping plane is cl[0] * x + cl[1] * y + cl[2] = 0
    // now find what value of t that is on both lines.
    // substitute x and y from the edge line into the equation of the clip line:
    // cl[0] * (v1_x + t * dx) + cl[1] * (v1_y + t * dy) + cl[2] = 0
    // then rearrange to get t:
    // cl[0] * v1_x + t * cl[0] * dx + cl[1] * v1_y + t * cl[1] * dy + cl[2] = 0
    // t * (cl[0] * dx + cl[1] * dy) + cl[0] * v1_x + cl[1] * v1_y + cl[2] = 0
    // t * (cl[0] * dx + cl[1] * dy) = -cl[0] * v1_x - cl[1] * v1_y - cl[2]
    // so we get:
    /* FIXME this assertion sometimes fails. t should always be between 0 and 1,
     * but sometimes isn't even when this one passes:
     */
    // DEBUG_ASSERT(cl[0] * dx + cl[1] * dy);
    double t = (-cl[0] * v1->tex_c[0] - cl[1] * v1->tex_c[1] - cl[2]) /
               (cl[0] * dx + cl[1] * dy),
    // now substitute t into the edge we are testing's line:
           x = dx * t + v1->tex_c[0],
           y = dy * t + v1->tex_c[1],
    // move the vertex coordinates to match the texture ones.
  // t = 0 would give v1, t = 1 would give v2; so we linearly interpolate by t:
           td1 = 1.0 - t,
           xc = v1->vertex_c[0] * td1 + v2->vertex_c[0] * t,
           yc = v1->vertex_c[1] * td1 + v2->vertex_c[1] * t;
    // DEBUG_ASSERT(-0.1 <= t && t <= 1.1);
    return Vertex(xc, yc, x, y);
}

A_Polygon A_Polygon::Clip(const double clip_line[3])
{
    // We'll use the Sutherland-Hodgman clipping algorithm.
    // see http://en.wikipedia.org/wiki/Sutherland-Hodgeman
    A_Polygon result;
    unsigned int vertices_count = verts.size(),
                 v_previous = vertices_count - 1;
    // we want to examine all edges in turn, and find the vertices to keep:
    for (unsigned int v_index = 0; v_index < vertices_count; v_index++)
    {
        bool v_index_inside = Inside(v_index, clip_line),
             v_previous_inside = Inside(v_previous, clip_line);
        if (v_index_inside != v_previous_inside)
        {
            // one in, one out, therefore the edge instersts the clip line.
            result.AddVertex(Intersect(v_previous, v_index, clip_line));
        }
        if (v_index_inside)
        {
            // keep any inner vertices.
            result.AddVertex(verts[v_index]);
        }
        v_previous = v_index;
    }
    return result;
}

std::vector<MeshRemapper::ArrayCoords> A_Polygon::ConvertToQuads()
{
    // we want to convert any old polygon to quadrilaterals.
    /* If the input was from a TexCoordRemapper, we'll have a rectangle with
     * some edges chopped off, this could be empty, or a convex polygon.
     * If we got our input from a VertexCoordRemapper it is likely to be (but
     * not necessarily) convex.
     * This is only guaranteed to work on convex polygons, though it will work 
     * some concave ones.
     * FIXME Either use something more stable for concave polygons, or allow
     * the Coords structure to take arbitrary polygons.
     * Warning: The output of the clipping algorithm can create coincident lines
     * where it has clipped through something concave. See this:
     * http://en.wikipedia.org/wiki/Sutherland-Hodgman_clipping_algorithm
     * It would be fine to leave those in if we don't mess up converting it to
     * quadrilaterals (for example if sending the polygon directly to OpenGL)
     */
    std::vector<MeshRemapper::ArrayCoords> result;
    unsigned int vertices_count = verts.size();
    unsigned int i = 2;
    // use vertex 0 for each face, and share another vertex with the last drawn,
    // to (at least in convex cases) get the shape we intended, like this:
    /*      0+------,7
     *      /|\    c \
     *   1 / |   \    \ 6
     *    |a |     \   |
     *    |  |        \|
     *   2 \ |   b    / 5
     *      \|       /
     *      3+------'4
     */
    while (i < vertices_count)
    {
        if (i < vertices_count - 1)
        {
            // make a new quadrilateral
            MeshRemapper::ArrayCoords quad;
            for (unsigned short int c = 0; c < 2; c++)
            {
                quad.vertex_c[0][0][c] = verts[  0  ].vertex_c[c];
                quad.tex_c   [0][0][c] = verts[  0  ].tex_c   [c];
                quad.vertex_c[0][1][c] = verts[i - 1].vertex_c[c];
                quad.tex_c   [0][1][c] = verts[i - 1].tex_c   [c];
                quad.vertex_c[1][1][c] = verts[  i  ].vertex_c[c];
                quad.tex_c   [1][1][c] = verts[  i  ].tex_c   [c];
                quad.vertex_c[1][0][c] = verts[i + 1].vertex_c[c];
                quad.tex_c   [1][0][c] = verts[i + 1].tex_c   [c];
            }
            result.push_back(quad);
            i += 2;
        }
        else
        {
            // make a new triangle, but repeat the last vertex so it is a quad
            MeshRemapper::ArrayCoords quad;
            for (unsigned short int c = 0; c < 2; c++)
            {
                quad.vertex_c[0][0][c] = verts[  0  ].vertex_c[c];
                quad.tex_c   [0][0][c] = verts[  0  ].tex_c   [c];
                quad.vertex_c[0][1][c] = verts[i - 1].vertex_c[c];
                quad.tex_c   [0][1][c] = verts[i - 1].tex_c   [c];
                quad.vertex_c[1][1][c] = verts[  i  ].vertex_c[c];
                quad.tex_c   [1][1][c] = verts[  i  ].tex_c   [c];
                quad.vertex_c[1][0][c] = verts[  i  ].vertex_c[c];
                quad.tex_c   [1][0][c] = verts[  i  ].tex_c   [c];
            }
            result.push_back(quad);
            i ++;
        }
    }
    return result;
}

void MeshRemapper::ClipFace(MeshRemapper::Coords *face)
{
    // Clip the given face so that it only shows the cropped region of the
    // source image.
    // We use the texture coordinates to work out where to crop, and adjust the
    // vertices locations accordingly.
    
    // convert the face to a polygon:
    A_Polygon poly(face);
    // work out where the clipping lines are, we use the cropping rectangle:
    double clip_lines[4][3] = {{ 1.0,  0.0, -crop_x1},
                               {-1.0,  0.0,  crop_x2},
                               { 0.0,  1.0, -crop_y1},
                               { 0.0, -1.0,  crop_y2}};
    // cut off each side
    poly = poly.Clip(clip_lines[0]);
    poly = poly.Clip(clip_lines[1]);
    poly = poly.Clip(clip_lines[2]);
    poly = poly.Clip(clip_lines[3]);
    // Feign circular clipping by trimming to some tangent lines.
    /* Work out what angles to clip to based on the locations of the vertices.
     * This is generally good, although it will break down where {a huge part /
     * all / even more} of the image contained within the input face. However in
     * this case the interpolation would make it well out of sync anyway, so
     * it's no big deal.
     */
    /* Since the coordinates are scaled to between 0 and 1, we actually need to
     * crop to a elipse to get the aspect ratio right.
     */
    if (circle_crop)
    {
        for (unsigned int edge = 0; edge < 4; edge++)
        {
            double angle = M_PI + atan2(face->tex_c[edge % 2][edge / 2][1]
                                                        - circle_crop_centre_y,
                                        face->tex_c[edge % 2][edge / 2][0]
                                                        - circle_crop_centre_x),
                    ac_x = cos(angle) / circle_crop_radius_x,
                    ac_y = sin(angle) / circle_crop_radius_y,
            clip_line[3] = {ac_x, ac_y, 1.0 - ac_x * circle_crop_centre_x
                                            - ac_y * circle_crop_centre_y};
            poly = poly.Clip(clip_line);
        }
    }
    // now convert to quadrilaterals.
    face_list = poly.ConvertToQuads();
}

bool MeshRemapper::GiveClipFaceResult(Coords * result)
{
    if (face_list.empty())
    {
        // no more faces
        return false;
    } else {
        // return a face
        for (unsigned short int x = 0; x < 2; x++)
        {
            for (unsigned short int y = 0; y < 2; y++)
            {
                for (unsigned short int c = 0; c < 2; c++)
                {
                    result->tex_c[x][y][c] = face_list.back().tex_c[x][y][c];
                    result->vertex_c[x][y][c] = face_list.back().vertex_c[x][y][c];
                }
            }
        }
        face_list.pop_back();
        return true;
    }
}

