// -*- c-basic-offset: 4 -*-

/** @file VertexCoordRemapper.cpp
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
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifdef __WXMAC__
#include "panoinc_WX.h"
#include "panoinc.h"
#endif

#include "VertexCoordRemapper.h"
#include <math.h>
#include <iostream>
#include "ViewState.h"

#include "panodata/SrcPanoImage.h"

/*******************
 * Face's bit flags 
 *******************/
// split_x is set if the node has been split into two children (subdivided) in x
const unsigned short int split_flag_x = 1;
// patch_flag_x is set if we are patching a hole / overlap that could be caused
// by the subdivision in the previous x location being more dense. It is set
// instead of split_flag_x, but we subdivide into faces that do not provide more
// detail except to the side where there is higher subdivision. 
const unsigned short int patch_flag_x = 2;
// set if we have subdivided into two halves in the y direction
const unsigned short int split_flag_y = 4;
// patch misalignment due to higher subdivision a bit lower in the y direction.
const unsigned short int patch_flag_y = 8;
// We flag vertices in faces that should be two faces over each edge of the
// panormama's 180 degree seem, such that by flipping the flagged vertices to
// the other side we get a valid face for one side, and by flipping those not
// flagged we get the other.
const unsigned short int vertex_side_flag_start = 16;
// 32, 64 and 128 are for the other vertices.
// if the tranformation can't find a result, we set flags to indicate the
// vertices we had a problem with.
const unsigned short int transform_fail_flag = 256;
// 512, 1024, and 2048 are for the other vertices

/* where we have a set of flags refering to different vertices, vertex [x][y]
 * corresponds with first_vertex_flag << (y * 2 + x).
 */


/*************************************
 * Detail / Acuracy / Speed Tradeoffs
 *************************************/
// Range of acceptable subdivision stopping points:
// Depth of forced subdivisions. Increase if there are transformations that show
// no signs of increasing detail level until subdivided more times, or cause
// problems with the seam detection.
const unsigned int min_depth = 4;
// Depth at which subdivision stops.
// When adjusting also adjust this, also adjust the definition of Tree's nodes
// to account for the extra memory required. A high value uses more memory,
// a low value limits the detail. The amount of elements in the array should be
// the sum from n = 0 to max_depth of 4^n. (so for n=6, 1+4+16+64+256+2048+4096)
const unsigned int max_depth = 6;

// this is the length in screen pixels under which no subdivision occurs.
// higher values are faster, lower ones more accurate.
// TODO user preference? Increase during interactive changes?
const double min_length = 14.0;
// the angle in radians under which no subdivision occurs. Again, higher values
// will make it faster, lower ones will give higher accuracy. must be positive.
const double min_angle = 0.06;
// the distance in absolute screen pixels between twice the length of the
// children and the length of the parent nodes, under which no subdivision
// occurs. higher values are faster, lower values give higher accuracy. Must be
// positive.
const double min_length_difference = 3.0;
// This is  the margin around the edge of the screen in pixels, outside of which
// any face is not subdivided. Higher values are less likely to crop small
// features from the edge of the display, lower values add speed when there is
// faces that are significantly off-screen. It can be any number, but only
// positive numbers are recommended.
const double offscreen_safety_margin = 10.0;

template <class T>
inline T sqr(T val)
{
    return val * val;
};

VertexCoordRemapper::VertexCoordRemapper(HuginBase::Panorama *m_pano_in,
                                         HuginBase::SrcPanoImage *image,
                                         VisualizationState *visualization_state_in)
    : MeshRemapper(m_pano_in, image, visualization_state_in)
{
    
}

void VertexCoordRemapper::UpdateAndResetIndex()
{
    DEBUG_DEBUG("mesh update update reset index");
    // this sets scale, height, and width.
    MeshRemapper::UpdateAndResetIndex();
    // we need to record the output projection for flipping around 180 degrees.
    output_projection = visualization_state->GetOptions()->getProjection();
    o_width = visualization_state->GetOptions()->getWidth();
    o_height = visualization_state->GetOptions()->getHeight();
    // we want to make a remapped mesh, get the transformation we need:
//    HuginBase::SrcPanoImage *src = visualization_state->GetSrcImage(image_number);
    transform.createInvTransform(*image, *(visualization_state->GetOptions()));
    // use the scale to determine edge lengths in pixels for subdivision
//    DEBUG_INFO("updating mesh for image " << image_number << ", using scale "
//              << scale << ".\n");
    // find key points used for +/- 180 degree boundary correction
    // {x|y}_add_360's are added to a value near the left/top boundary to get
    // the corresponding point over the right/bottom boundary.
    // other values are used to check where the boundary is.
    OutputProjectionInfo *info = visualization_state->GetProjectionInfo();
    x_add_360 = info->GetXAdd360();
    radius = info->GetRadius();
    y_add_360 = info->GetYAdd360();
    x_midpoint = info->GetMiddleX();
    y_midpoint = info->GetMiddleY();
    lower_bound = info->GetLowerX();
    upper_bound = info->GetUpperX();
    lower_bound_h = info->GetLowerY();
    upper_bound_h = info->GetUpperY();
    // Find the cropping region of the source image, so we can stick to the
    // bounding rectangle.
    SetCrop();
    // the tree needs to know how to use the croping information for generating
    tree.x_crop_scale = crop_x2 - crop_x1;
    tree.x_crop_offs = crop_x1;
    tree.y_crop_scale = crop_y2 - crop_y1;
    tree.y_crop_offs = crop_y1;
    // make the tree reflect the transformation
    RecursiveUpdate(0, 0, 0);
    // now set up for examining the tree contents.
    tree.ResetIndex();
    done_node = true;
}

bool VertexCoordRemapper::GetNextFaceCoordinates(Coords *result)
{
//    DEBUG_DEBUG("mesh update get face coords");
    result->tex_c = tex_coords;
    result->vertex_c = s_vertex_coords;
    // if we have some faces left over from a previous clipping operation, give
    // one of those first:
    if (GiveClipFaceResult(result)) return true;
    // when 180 degree bounds correction is working, we'll have two nodes.
    if (done_node)
    {
        do
        {
            // this will search the tree for the next leaf node.
            tree_node_id = tree.GetNext();
            if (!tree_node_id)
            {
                // we've reached last one
                return false;
            }
        }
        // some of the verticies may have arrived from undefined transformations
        // if this is one, skip it and try to find another.
        while ((tree.nodes[tree_node_id].flags & (transform_fail_flag * 15)));
            
        // find the coordinates from the tree node
        result->vertex_c = tree.nodes[tree_node_id].verts;
        // check that the transformation is properly defined.

        tree.GetInputCoordinates(tree_node_id, tex_coords);
        // if the node has a discontinuity, we want to split it into two
        // faces and return each one on consecutive calls.
        discontinuity_flags =
                 (tree.nodes[tree_node_id].flags / vertex_side_flag_start) % 16;
        if (discontinuity_flags)
        {
            done_node = false;
            // flip the marked nodes to the other side. copy the coordinates 1st
            result->vertex_c = s_vertex_coords;            
            for (short unsigned int x = 0; x < 2; x++)
            {
                for (short unsigned int y = 0; y < 2; y++)
                {
                      s_vertex_coords[x][y][0] =
                                        tree.nodes[tree_node_id].verts[x][y][0];
                      s_vertex_coords[x][y][1] =
                                        tree.nodes[tree_node_id].verts[x][y][1];
                      if (discontinuity_flags & (1 << (x*2 + y)))
                      {
                          DiscontinuityFlip(s_vertex_coords[x][y]);
                      }
                }
            }
        }
    } else {
        // we flip the other vertices to the ones we did last time.
        done_node = true;
        for (short unsigned int x = 0; x < 2; x++)
        {
            for (short unsigned int y = 0; y < 2; y++)
            {
                  s_vertex_coords[x][y][0] =
                                        tree.nodes[tree_node_id].verts[x][y][0];
                  s_vertex_coords[x][y][1] =
                                        tree.nodes[tree_node_id].verts[x][y][1];
                  if (!(discontinuity_flags & (1 << (x*2 + y))))
                  {
                      DiscontinuityFlip(s_vertex_coords[x][y]);
                  }
            }
        }
    }
    // if we are doing circular cropping, clip the face so it makes the shape
    if (circle_crop)
    {
        // If all points are within the radius, then don't clip
//        HuginBase::SrcPanoImage *src_img = visualization_state->GetSrcImage(image_number);
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
            return true;
        }
        // we do need to clip:     
        ClipFace(result);
        // if there was anything left, return the first face and leave the rest
        // for later.
        if (GiveClipFaceResult(result)) return true;
        // we clipped to nothing... try and get another face: from the top...
        return (GetNextFaceCoordinates(result));
    }
    return true;
}

void VertexCoordRemapper::DiscontinuityFlip(double vertex_c[2])
{
    // we want to flip the given vertex to be the other side of the 180 degree
    // boundary, whatever the projection format.
    switch (output_projection)
    {
        case HuginBase::PanoramaOptions::RECTILINEAR:
            // There is no 180 degree boundary for rectilinear projections.
            // Anything containing a vertex beyond 90 degrees (i.e. behind the
            ///viewer) is not drawn.
            break;
        case HuginBase::PanoramaOptions::FULL_FRAME_FISHEYE:
        case HuginBase::PanoramaOptions::STEREOGRAPHIC:
        case HuginBase::PanoramaOptions::LAMBERT_AZIMUTHAL:
        case HuginBase::PanoramaOptions::ORTHOGRAPHIC:
        case HuginBase::PanoramaOptions::EQUISOLID:
        case HuginBase::PanoramaOptions::THOBY_PROJECTION:
            // circular projections. These stretch rather nastily over the
            // centre, and correcting them doesn't help much, so any image
            // covering the outer circle is switched to a TexCoordRemapper.
            break;
        case HuginBase::PanoramaOptions::CYLINDRICAL:
        case HuginBase::PanoramaOptions::EQUIRECTANGULAR:
        case HuginBase::PanoramaOptions::MERCATOR:
        case HuginBase::PanoramaOptions::LAMBERT:
        case HuginBase::PanoramaOptions::MILLER_CYLINDRICAL:
        case HuginBase::PanoramaOptions::ARCHITECTURAL:
            // flip to the other direction of the other side horizontally.
            if (vertex_c[0] < x_midpoint) vertex_c[0] += x_add_360;
            else vertex_c[0] -= x_add_360;
            break;
        case HuginBase::PanoramaOptions::SINUSOIDAL:
        case HuginBase::PanoramaOptions::ALBERS_EQUAL_AREA_CONIC:
            if (vertex_c[0] < x_midpoint)
            {
                vertex_c[0] +=
                      visualization_state->GetProjectionInfo()->GetXAdd360(vertex_c[1]);
            } else {
                vertex_c[0] -=
                      visualization_state->GetProjectionInfo()->GetXAdd360(vertex_c[1]);
            }
            break;
        case HuginBase::PanoramaOptions::TRANSVERSE_MERCATOR:
            // flip to the other direction of the other side vertically
            if (vertex_c[1] < y_midpoint) vertex_c[1] += y_add_360;
            else vertex_c[1] -= y_add_360;
            break;
        
    }
}

void VertexCoordRemapper::RecursiveUpdate(unsigned int node_num,
                                   unsigned int stretch_x, unsigned stretch_y)
{
    // find where we are and what we are mapping
    // TODO? GetPosition is called by GetInputCoordinates, reuse results?
    unsigned int x, y, row_size, depth;
    tree.GetPosition(node_num, x, y, row_size, depth);
    tree.GetInputCoordinates(node_num, tex_coords);
    TreeNode *node = &tree.nodes[node_num],
             *parent = (depth) ?
                       &tree.nodes[tree.GetParentId(x, y, row_size, depth)] : 0,
             *left = (x % 2) ?
                        &tree.nodes[tree.GetIndex(x-1, y, row_size, depth)] : 0,
             *top = (y % 2) ?
                        &tree.nodes[tree.GetIndex(x, y-1, row_size, depth)] : 0;
    bool valid[2][2];
    for (unsigned short int i = 0; i < 2; i++)
    {
        for (unsigned short int j = 0; j < 2; j++)
        {
            if (depth == 0)
            {
                // the top level has no parent, so we must calculate all points
                valid[i][j] =
                      transform.transformImgCoord(node->verts[i][j][0],
                                                  node->verts[i][j][1],
                                                  tex_coords[i][j][0] * width,
                                                  tex_coords[i][j][1] * height);
            } else
            // Look up where the point in the tree so far. If this is the first
            // occurrence of this point, we'll calculate the value.
            if (i == x %2 && j == y%2 && depth)
            {
                // extract a corner from the parent.
                node->verts[i][j][0] = parent->verts[i][j][0];
                node->verts[i][j][1] = parent->verts[i][j][1];
                valid[i][j] = !(parent->flags & (transform_fail_flag << (j*2 +i)));
            } else if (x % 2 && !i) {
                // copy from the left
                node->verts[0][j][0] = left->verts[1][j][0];
                node->verts[0][j][1] = left->verts[1][j][1];
                valid[i][j] = !(left->flags & (transform_fail_flag << (j *2 + 1)));
            } else if (y % 2 && !j) {
                // copy from the top
                node->verts[i][0][0] = top->verts[i][1][0];
                node->verts[i][0][1] = top->verts[i][1][1];
                valid[i][j] = !(top->flags & (transform_fail_flag << (2 + i)));
            } else {
                // We can't find it easily, try a more expensive search.
                // this will linearly interpolate along the edges where the
                // subdivision was higher, avoiding gaps when the detail level
                // was lower above or to the left.
                if (!tree.GetTransform(x + i * (1 << stretch_x),
                                       y + j * (1 << stretch_y),
                                       depth, x, y, node->verts[i][j][0],
                                       node->verts[i][j][1]))
                {
                    // We can't find it, so calculate it:
                    valid[i][j] =
                      transform.transformImgCoord(node->verts[i][j][0],
                                                  node->verts[i][j][1],
                                                  tex_coords[i][j][0] * width,
                                                  tex_coords[i][j][1] * height);
                    // If the face results from a patch subdivision (for
                    // aligning a subdivided face, otherwise it need not exist),
                    // use the midpoint of the parent's vertices instead of
                    // calculating a transformed one, so we can use less
                    // subdivision on the other side.
                    // subdivision in x
                    // FIXME there are still gaps. I think there's a logic error
                    if (   depth // not on the top level.
                        // patching in y
                        && (parent->flags & patch_flag_x)
                        // we must be in the middle of the split, the nodes on
                        // the corners of the parent line up anyway.
                        && ((i + x) % 2)
                        // we should be on the side away from the subdivison
                        // (+ve y).
                        && j
                        // If we are alao split in y we can use the middle
                        // node to provide more detail.
                        && (!((parent->flags & split_flag_y) && !(y % 2)))
                        // don't do this if we cross the 180 degree seam
                        && (!(parent->flags & (vertex_side_flag_start * 15))))
                    {
                        node->verts[i][1][0] = (parent->verts[0][1][0]
                                               + parent->verts[1][1][0]) / 2.0;
                        node->verts[i][1][1] = (parent->verts[0][1][1]
                                               + parent->verts[1][1][1]) / 2.0;
                    }
                    // subdivision in y
                    if (   depth
                        && (parent->flags & patch_flag_y)
                        && ((j + y) % 2)
                        && i
                        && (!((parent->flags & split_flag_x) && !(x % 2)))
                        && (!(parent->flags & (vertex_side_flag_start * 15))))
                    {
                        node->verts[1][j][0] = (parent->verts[1][0][0]
                                               + parent->verts[1][1][0]) / 2.0;
                        node->verts[1][j][1] = (parent->verts[1][0][1]
                                               + parent->verts[1][1][1]) / 2.0;
                    }
                } else {
                    // we managed to find it from data already known.
                    valid[i][j] = true;
                }
            }
        }
    }
    
    // now for the recursion
    // which directions should we divide in?
    TestSubdivide(node_num);
    // add the flags for invlaid transormations
    for (unsigned int i = 0; i < 2; i++)
    {
        for (unsigned int j = 0; j < 2; j++)
        {
            if (!valid[i][j])
            {
                node->flags |= transform_fail_flag << (j * 2 + i);
            }
        }
    }
    // if the face should be split, now recurse to the child nodes.
    if (node->flags & (split_flag_x | split_flag_y))
    {
        // we will split at least one way.
        if (!(node->flags & split_flag_x))
        {
            // we are not splitting across x, but will will across y.
            // so the quad will be twice as wide
            stretch_x++;
        }
        else if (!(node->flags & split_flag_y))
        {
            stretch_y++;
        }
        // find the top left sub-quad
        x *= 2;
        y *= 2;
        row_size *= 2;
        depth++;
        // the top left is always generated
        RecursiveUpdate(tree.GetIndex(x, y, row_size, depth),
                        stretch_x, stretch_y);
        // split in x
        if (node->flags & split_flag_x)
        {
            RecursiveUpdate(tree.GetIndex(x + 1, y, row_size, depth),
                            stretch_x, stretch_y);
        }
        // split in y
        if (node->flags & split_flag_y)
        {
            RecursiveUpdate(tree.GetIndex(x, y + 1, row_size, depth),
                            stretch_x, stretch_y);
            // if we are splitting in both directions, do the lower right corner
            if (node->flags & split_flag_x)
            {
                RecursiveUpdate(tree.GetIndex(x + 1, y + 1, row_size, depth),
                    stretch_x, stretch_y);
            }
        }
    }
}

void VertexCoordRemapper::TestSubdivide(unsigned int node_id)
{
    TreeNode *node = &tree.nodes[node_id];
    unsigned int x, y, row_size, depth;
    tree.GetPosition(node_id, x, y, row_size, depth);
    unsigned short int flags = 0;
    if (depth < min_depth)
    {
        // subdivide at least min_depth times
        // we will need more information for non-trivial children
        SetLengthAndAngle(node);
        flags |= split_flag_x | split_flag_y;
    } else {
        unsigned int parent_id = tree.GetParentId(node_id);        
        TreeNode *parent = &tree.nodes[parent_id];
        // minimum length check. We use the length of the top edge to test for
        // subdivision in x, and the length of the left edge for subdivision in
        // y.
        SetLengthAndAngle(node);
        bool do_not_split_x = node->length_x * scale < min_length,
             do_not_split_y = node->length_y * scale < min_length;
        if (depth == max_depth)
        {
            // don't subdivide more than max_depth times
            do_not_split_x = true;
            do_not_split_y = true;
        }    
        // if we have stopped splitting in some direction, don't consider
        // splitting in that direction again.
        if (!(tree.nodes[parent_id].flags & split_flag_x))
        {
            do_not_split_x = true;
        }
        else if (!(tree.nodes[parent_id].flags & split_flag_y))
        {
            do_not_split_y = true;
        }
        // if it has only subdivided to patch up between two subdivision levels,
        // don't consider subdividing for adding more detail.
        if (tree.nodes[parent_id].flags & patch_flag_x)
        {
            do_not_split_x = true;
        }
        else if (tree.nodes[parent_id].flags & patch_flag_y)
        {
            do_not_split_y = true;
        }

        // If the angles have deviated too much from the parent then we should
        // add more detail, however if it is fairly flat then we don't need to.
        // It is possible for the angles to remain constant but the length
        // of the lines to change dramatically, so we check for a big difference
        // between the length of the parent node and twice the length of child.
        // if the child does not change the length much and the angle is small,
        // then we have enough detail, and we don't split.
        float ang_x = node->angle_x - parent->angle_x;
        if (ang_x < 0) ang_x = -ang_x;
        if (ang_x > M_PI) ang_x = 2 * M_PI - ang_x;
        float length_difference_x
                          = (parent->length_x - (2.0 * node->length_x)) * scale;
        if (length_difference_x < 0.0)
        {
            length_difference_x = -length_difference_x;
        }
        if (ang_x < min_angle && length_difference_x < min_length_difference)
        {
            do_not_split_x = true;
        }
        float ang_y = node->angle_y - parent->angle_y;
        if (ang_y < 0) ang_y = -ang_y;
        if (ang_y > M_PI) ang_y = 2 * M_PI - ang_y;
        float length_difference_y
                          = (parent->length_y - (2.0 * node->length_y)) * scale;
        if (length_difference_y < 0.0)
        {
            length_difference_y = -length_difference_y;
        }
        if (ang_y < min_angle  && length_difference_y < min_length_difference)
        {
            do_not_split_y = true;
        }
        // if the face is entirely off the screen, we should not subdivide it.
        // get the screen area
        vigra::Rect2D viewport = visualization_state->GetVisibleArea();
        // add a margin for safety, we don't want to clip too much stuff that
        // curls back on to the screen. We add 2 as we need some space around 
        // very small panoramas that have enlarged to fit the preview window, 
        // and even with a fairly large margin rounding to int may lose the
        // border completely.
        viewport.addBorder((int) (2.0 + offscreen_safety_margin * scale));
        bool all_left = true, all_right = true,
             all_above = true, all_bellow = true;
        for (unsigned int ix = 0; ix < 2; ix++)
        {
            for (unsigned int iy = 0; iy < 2; iy++)
            {
                if (node->verts[ix][iy][0] > viewport.left())
                                                    all_left = false;
                if (node->verts[ix][iy][0] < viewport.right())
                                                    all_right = false;
                if (node->verts[ix][iy][1] > viewport.top())
                                                    all_above = false;
                if (node->verts[ix][iy][1] < viewport.bottom())
                                                    all_bellow = false;
            }
        }
        if (all_left || all_right || all_bellow || all_above)
        {
            // all vertices are off a side of the screen. This catches most
            // cases where the face is off the screen. Don't allow subdivisions:
            do_not_split_x = true;
            do_not_split_y = true;
        }
        if (!do_not_split_x) flags |= split_flag_x;
        if (!do_not_split_y) flags |= split_flag_y;
    }
    /* Flag the vertices on a different side of the +/-180 degree seam.
     * We don't want to flag any vertices if the face covers a continuous
     * area of the transformation.
     */
    // We don't need to mark the first few subdivisions, but this is necessary
    // when patch subdivisions become possible.
    if (depth >= min_depth)
    {
        // determine if it is likely to be non-continuous.
        // this needs to be false for leaf-node faces in the centre that span
        // across the '0 degree' point, and true for faces that span the +/-180
        // degree split. It doesn't really matter what it is set to otherwise.
        bool noncontinuous = false;
        OutputProjectionInfo *i = visualization_state->GetProjectionInfo();
        switch (output_projection)
        {
            case HuginBase::PanoramaOptions::RECTILINEAR:
                // we don't need faces to cross from one side to another. Faces
                // all / partially 'behind the viewer' are skipped because the
                // vertices behind the viewer are marked.
                break;
            case HuginBase::PanoramaOptions::FULL_FRAME_FISHEYE:
            case HuginBase::PanoramaOptions::STEREOGRAPHIC:
            case HuginBase::PanoramaOptions::LAMBERT_AZIMUTHAL:
                // A mesh covering the extremities of a disk projection should
                // be using a TexCoordRemapper instead, otherwise, a point
                // mapping to the border will be stretched across the disk.
                break;
            case HuginBase::PanoramaOptions::CYLINDRICAL:
            case HuginBase::PanoramaOptions::EQUIRECTANGULAR:
            case HuginBase::PanoramaOptions::MERCATOR:
            case HuginBase::PanoramaOptions::LAMBERT:
            case HuginBase::PanoramaOptions::MILLER_CYLINDRICAL:
            case HuginBase::PanoramaOptions::PANINI:
			case HuginBase::PanoramaOptions::BIPLANE:
			case HuginBase::PanoramaOptions::TRIPLANE:
            case HuginBase::PanoramaOptions::GENERAL_PANINI:
                // Cylinderical-like projections have the seam across the left
                // and right edge. We'll take any face within the middle third
                // to be continuous, the rest possibly noncontinuous.
                if (   node->verts[0][0][0] < lower_bound
                    || node->verts[0][0][0] > upper_bound
                    || node->verts[1][0][0] < lower_bound
                    || node->verts[1][0][0] > upper_bound
                    || node->verts[0][1][0] < lower_bound
                    || node->verts[0][1][0] > upper_bound
                    || node->verts[1][1][0] < lower_bound
                    || node->verts[1][1][0] > upper_bound)
                {
                    noncontinuous = true;
                    // flag nodes on the right hand side.
                    if (node->verts[0][0][0] > x_midpoint)
                    {
                        flags |= vertex_side_flag_start;
                    }
                    if (node->verts[0][1][0] > x_midpoint)
                    {
                        flags |= vertex_side_flag_start * 2;
                    }
                    if (node->verts[1][0][0] > x_midpoint)
                    {
                        flags |= vertex_side_flag_start * 4;
                    }
                    if (node->verts[1][1][0] > x_midpoint)
                    {
                        flags |= vertex_side_flag_start * 8;
                    }
                }
                break;
            case HuginBase::PanoramaOptions::SINUSOIDAL:
                // like above, but the bounds change with height
                if (   node->verts[0][0][0] < i->GetLowerX(node->verts[0][0][1])
                    || node->verts[0][0][0] > i->GetUpperX(node->verts[0][0][1])
                    || node->verts[1][0][0] < i->GetLowerX(node->verts[1][0][1])
                    || node->verts[1][0][0] > i->GetUpperX(node->verts[1][0][1])
                    || node->verts[0][1][0] < i->GetLowerX(node->verts[0][1][1])
                    || node->verts[0][1][0] > i->GetUpperX(node->verts[0][1][1])
                    || node->verts[1][1][0] < i->GetLowerX(node->verts[1][1][1])
                    || node->verts[1][1][0] > i->GetUpperX(node->verts[1][1][1])
                   )
                {
                    noncontinuous = true;
                    // flag nodes on the right hand side.
                    if (node->verts[0][0][0] > x_midpoint)
                    {
                        flags |= vertex_side_flag_start;
                    }
                    if (node->verts[0][1][0] > x_midpoint)
                    {
                        flags |= vertex_side_flag_start * 2;
                    }
                    if (node->verts[1][0][0] > x_midpoint)
                    {
                        flags |= vertex_side_flag_start * 4;
                    }
                    if (node->verts[1][1][0] > x_midpoint)
                    {
                        flags |= vertex_side_flag_start * 8;
                    }
                }
                break;
            case HuginBase::PanoramaOptions::TRANSVERSE_MERCATOR:
                // like the cylindrical ones, but vertically.
                if (   node->verts[0][0][1] < lower_bound_h
                    || node->verts[0][0][1] > upper_bound_h
                    || node->verts[1][0][1] < lower_bound_h
                    || node->verts[1][0][1] > upper_bound_h
                    || node->verts[0][1][1] < lower_bound_h
                    || node->verts[0][1][1] > upper_bound_h
                    || node->verts[1][1][1] < lower_bound_h
                    || node->verts[1][1][1] > upper_bound_h)
                {
                    noncontinuous = true;
                    // flag nodes on the bottom side.
                    if (node->verts[0][0][1] > y_midpoint)
                    {
                        flags |= vertex_side_flag_start;
                    }
                    if (node->verts[0][1][1] > y_midpoint)
                    {
                        flags |= vertex_side_flag_start * 2;
                    }
                    if (node->verts[1][0][1] > y_midpoint)
                    {
                        flags |= vertex_side_flag_start * 4;
                    }
                    if (node->verts[1][1][1] > y_midpoint)
                    {
                        flags |= vertex_side_flag_start * 8;
                    }
                }
                break;
            case HuginBase::PanoramaOptions::ALBERS_EQUAL_AREA_CONIC:
                break;
        }
        if (noncontinuous)
        {
            // If all the side flags are set, we only have one face on one side:
            // Remove all of those flags and we have the same face, but we can
            // use the presence of any flags to detect when we have two.
            if ((flags / vertex_side_flag_start) % 16 == 15)
            {
                flags &= ~(vertex_side_flag_start * 15);
            }
        }
    }

    node->flags = flags;
    // check if the faces to the left are subdivided at a higher level
    if (x && !(flags & split_flag_y) && (depth < max_depth))
    {
        // get the potentially more subdivided version
        double dest_x, dest_y;
        unsigned int subdiv_node;
        subdiv_node = tree.GetTransform(x * 2, y * 2 + 1,
                                        depth * 2,
                                        x * 2, y * 2, dest_x, dest_y);
        if (subdiv_node > node_id)
        {
            // we should have a more subdivided node on the left.
            // mark it for patching up to line up with it.
            node->flags |= split_flag_y | patch_flag_y;
        }
    }
    if (y && !(flags & split_flag_x) && (depth < max_depth))
    {
        // get the potentially more subdivided version
        double dest_x, dest_y;
        unsigned int subdiv_node;
        subdiv_node = tree.GetTransform(x * 2 +  1, y * 2,
                                        depth * 2,
                                        x * 2, y * 2, dest_x, dest_y);
        if (subdiv_node > node_id)
        {
            // we should have a more subdivided node on the left.
            // mark it for patching up to line up with it.
            node->flags |= split_flag_x | patch_flag_x;
        }
    }
}

void VertexCoordRemapper::SetLengthAndAngle(TreeNode *node)
{
    float xdx = node->verts[0][0][0] - node->verts[1][0][0],
          xdy = node->verts[0][0][1] - node->verts[1][0][1],
          ydx = node->verts[0][0][0] - node->verts[0][1][0],
          ydy = node->verts[0][0][1] - node->verts[0][1][1];
    // this is the length of the edge with y = 0
    node->length_x = sqrt(sqr(xdx) + sqr(xdy));
    // this is the length of the edge with x = 0
    node->length_y = sqrt(sqr(ydx) + sqr(ydy));
    // find the angle of the top and left edge.
    node->angle_x = atan2(xdy, xdx);
    node->angle_y = atan2(ydy, ydx);
}

unsigned int VertexCoordRemapper::Tree::GetParentId(const unsigned int nodenum)
{
    // get the parent id of a node specifed by its index.
    unsigned int x, y, row_size, depth;
    GetPosition(nodenum, x, y, row_size, depth);
    return GetParentId(x, y, row_size, depth);
}

unsigned int VertexCoordRemapper::Tree::GetParentId(unsigned int x,
                                             unsigned int y,
                                             unsigned int row_size,
                                             unsigned int depth)
{
    // get the parent id of a node specified by an exact location.
    // the parent is the one that takes the group of four elements around this
    // one in the above depth. All the dimensions are halved.
    x /= 2;
    y /= 2;
    row_size /= 2;
    depth--;    
    return GetIndex(x, y, row_size, depth);
}

unsigned int VertexCoordRemapper::Tree::GetDepth(const unsigned int node_num)
{
    unsigned int depth = 0, count = 0;
    while (node_num > count)
    {
        depth++;
        count += 1 << (depth * 2);
    }
    return depth;
}

void VertexCoordRemapper::Tree::GetPosition(const unsigned int node_num,
                                     unsigned int &x, unsigned int &y,
                                     unsigned int &row_size,
                                     unsigned int &depth)
{
    depth = GetDepth(node_num);
    row_size = 1 << depth;
    unsigned int sub = 0;
    if (depth)
    {
      for (unsigned int d = 0; d < depth; d++)
      {
          sub += (1 << (d * 2));
      }
    }
    unsigned int position_id = node_num - sub;
    x = position_id % row_size;
    y = position_id / row_size;
}

unsigned int VertexCoordRemapper::Tree::GetIndex(const unsigned int x,
                                          const unsigned int y,
                                          const unsigned int row_size,
                                          unsigned int depth)
{
    unsigned int add = 0;
    while (depth)
    {
        depth--;
        add += 1 << (depth * 2);
    }
    return add + x + y * row_size;
}

void VertexCoordRemapper::Tree::GetInputCoordinates(const unsigned int node_num,
                                             double coords[2][2][2])
{
    // find the coordinates of each point at the vertices in the original image.
    // this is used to look up the remapped coordinates and provide texture
    // coordinates.
    // it halves in size several times depending on depth.
    unsigned int x, y, row_size, depth;
    GetPosition(node_num, x, y, row_size, depth);
    // now we can locate the upper right corner
    double row_spacing = 1.0 / (double) row_size;
    coords[0][0][0] = row_spacing * (double) x;
    coords[0][0][1] = row_spacing * (double) y;
    // the extent is dependent on the direction of the subdivisions.
    // at least one direction has an extent of row_spacing. It can double up
    // if the parent did not subdivide in a direction, recursively up the tree.
    bool scale_x = false;
    double opp = 1.0;
    if (node_num != 0)
    {    
      unsigned int parent_id = GetParentId(x, y, row_size, depth);
      if (!(nodes[parent_id].flags & split_flag_x))
      {
          while (!(nodes[parent_id].flags & split_flag_x))
          {
              parent_id = GetParentId(parent_id);
              opp *= 2.0;
          }
          scale_x = true;
      } else {
          while (!(nodes[parent_id].flags & split_flag_y))
          {
              parent_id = GetParentId(parent_id);
              opp *= 2.0;
          }
      }
    }
    opp *= row_spacing;
    coords[1][0][0] = coords[0][0][0] + (scale_x ? opp : row_spacing);
    coords[1][0][1] = coords[0][0][1];
    coords[0][1][0] = coords[0][0][0];
    coords[0][1][1] = coords[0][0][1] + (scale_x ? row_spacing : opp);
    coords[1][1][0] = coords[1][0][0];
    coords[1][1][1] = coords[0][1][1];
    // now we transform the results so that we map to the cropped region instead
    // of the whole image.
    for (unsigned int i = 0; i < 2; i++)
    {
        for (unsigned int j = 0; j < 2; j++)
        {
            coords[i][j][0] = coords[i][j][0] * x_crop_scale + x_crop_offs;
            coords[i][j][1] = coords[i][j][1] * y_crop_scale + y_crop_offs;
        }
    }
}

void VertexCoordRemapper::Tree::ResetIndex()
{
    cur_tree_node = 0;
}

unsigned int VertexCoordRemapper::Tree::GetNext()
{
    // depth first search for leaf nodes with cur_tree_node
    unsigned int x, y, row_size, depth;
    GetPosition(cur_tree_node, x, y, row_size, depth);
    // we want to backtrack until we find an unexplored sub branch. At this 
    //     point, we trace down the tree until we get to a leaf.
    if (cur_tree_node != 0)
    {
        unsigned int xd = 0, yd = 0;
        bool done = false;
        while (!done && cur_tree_node != 0)
        {
            xd = x % 2;
            yd = y % 2;
            x /= 2;
            y /= 2;
            row_size /= 2;
            depth--;
            cur_tree_node = GetIndex(x, y, row_size, depth);
            if (cur_tree_node == 0 && xd == 1 && yd == 1)
            {
                // we've expanded all the options and got back to the top
                return 0; // no more faces
            }
            // where does this split?
            bool sx = ((nodes[cur_tree_node].flags & split_flag_x) != 0);
            bool sy = ((nodes[cur_tree_node].flags & split_flag_y) != 0);
            // have we used all the split options?
            if (!(((sx && xd) || !sx) && ((sy && yd) || !sy)))
            {
                // we've found an unexpanded child, take the next one:
                done = true;
                x *= 2;
                y *= 2;
                row_size *= 2;
                depth++;
                if (sx && !xd && !yd) {
                    x++;
                } else if ((sx && xd && sy && !yd) || (!sx && sy && !yd)) {
                    y++;
                } else if (sx && sy && !xd && yd) {
                    x++; y++;
                }
                cur_tree_node = GetIndex(x, y, row_size, depth);
                // if we've made our way to the root node, we've run out out
                // of options.
                if (cur_tree_node == 0)
                {
                    return 0;
                }
            };
        }
    }
    // find the first leaf on this subtree, taking top left every time.
    while (nodes[cur_tree_node].flags & (split_flag_x | split_flag_y))
    {
        x *= 2;
        y *= 2;
        row_size *= 2;
        depth++;
        cur_tree_node = GetIndex(x, y, row_size, depth);
    }
    return cur_tree_node;
}

unsigned int VertexCoordRemapper::Tree::GetTransform(unsigned int src_x, unsigned int src_y,
                                      unsigned int max_depth,
                                      unsigned int stop_x, unsigned int stop_y,
                                      double &dest_x, double &dest_y)
{
    // we start at the top and take children, prefering the lowest numbered one,
    // until we either have no children, or we have found a child that knows the
    // exact place we are looking for.
    unsigned int no_x = 0, no_y = 0, row_size = 1, depth = 0,
                 rem_x = src_x, rem_y = src_y,
                 step_x = 1 << max_depth, step_y = 1 << max_depth,
                 node_id = GetIndex(no_x, no_y, row_size, depth);
    while  (   !(    (rem_x == 0 || rem_x == step_x)
                  && (rem_y == 0 || rem_y == step_y))
            &&  (nodes[node_id].flags & (split_flag_x | split_flag_y)))
    {
        // split, try and take earliest child node that leads towards goal
        depth++;
        no_x *= 2;
        no_y *= 2;
        row_size *= 2;
        if (nodes[node_id].flags & split_flag_x)
        {
            step_x /= 2;
            if (rem_x > step_x)
            {
                no_x ++;
                rem_x -= step_x;
            }
        }
        if (nodes[node_id].flags & split_flag_y)
        {
            step_y /= 2;
            if (rem_y > step_y)
            {
                no_y ++;
                rem_y -= step_y;
            }
        }
        // we want to stop if we have got a node at least as high as the
        // requested stopping node. Anything at a higher depth is after it.
        if (depth > max_depth) return 0;
        node_id = GetIndex(no_x, no_y, row_size, depth);
        if (depth == max_depth && no_x >= stop_x && no_y >= stop_y) return 0;
    }
    // if any of the vertices we are want to use are invalid (e.g. behind the
    // viewer in a rectilinear projection) we don't want to risk using them:
    if (nodes[node_id].flags & (transform_fail_flag * 15)) return 0;
    // if this face crosses a discontinuity, we should be using a point off
    // screen instead of in the middle. Refuse to use these faces
    if (nodes[node_id].flags & (vertex_side_flag_start * 15)) return 0;
    // linearly interpolate the node's corners.
    // most of the time we only use factors of 0 and 1, we don't want to make
    // points up except when trying to connect a point on a highly subdivided
    // face to a point that doesn't exist because it is on a less subdivided
    // face, in which case it needs to line up with the linear interpolation of
    // the less subdivided face. This is along one edge, so the other direction
    // should have a blending factor of 0 or 1.
    double xf = (double) rem_x / (double) step_x;
    double yf = (double) rem_y / (double) step_y;
    
    double top_x = (1.0 - xf) * nodes[node_id].verts[0][0][0]
              + xf * nodes[node_id].verts[1][0][0],
           bottom_x = (1-.0 - xf) * nodes[node_id].verts[0][1][0]
              + xf * nodes[node_id].verts[1][1][0],
           top_y = (1.0 - xf) * nodes[node_id].verts[0][0][1]
              + xf * nodes[node_id].verts[1][0][1],
           bottom_y = (1.0 - xf) * nodes[node_id].verts[0][1][1]
              + xf * nodes[node_id].verts[1][1][1];
    dest_x = top_x * (1.0 - yf) + bottom_x * yf;
    dest_y = top_y * (1.0 - yf) + bottom_y* yf;
    return node_id;
}

