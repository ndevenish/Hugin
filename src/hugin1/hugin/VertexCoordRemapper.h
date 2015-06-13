// -*- c-basic-offset: 4 -*-

/** @file VertexCoordRemapper.h
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

/* A VertexCoordRemapper uses the reamapping transformations to create a set of
 * quadrilatrials that approximate the remapping. Each quad represents a small
 * axis-aligned rectangle of the input image.
 * This is created by an adaptive subdivision method:
 *  - Each corner is mapped to the correct position, to give a quadrilatial.
 *  - Each edge is tested against some function
 *  - The edges that fail the test are split into two edges, the middle
 *        of the original edge is mapped to the correct position, the new lines
 *        meet at this point, and use the end points of the original line.
 *  - The new edges are tested and optionally split, etc.
 *
 * The testing function:
 *  - fails for any edge under minimum depth
 *  - passes for any edge at maximum depth
 *  - passes for any edge not affected by the above conditions and is {shorter
 *        than minimum length | makes an angle of less than a minimum angle}
 *  ? passes for any edge that is not affected by the above conditions, and is
 *        deemed to cross a discontinuity, and has each end point closer than
 *        minimum length to the panorama's edge
 *
 * Edges are deemed to span across a discontinuity if they are longer than the
 * two edges they are created from, and make a sharp turn at each end. This is
 * not entierly accurate. Since remappings are mostly quite smooth, we let the
 * minimum depth test avoid most mistakes that can be caused by this assumption.
 * 
 * The meshes can then be caclulated from these edges. Starting with the initial
 * quadrilatrial, we can check if the edges have been subdivided in each
 * direction. Where a subdivided edge and a non subdivided edge are oposite
 * each other in a quad, the non subdivided edge gets subdivided linearly, so
 * it lines up with the quad on the other side (this subdivide only
 * applies to one side of the edge, and adds no more actual geometric detail).
 */

#ifndef __VERTEXCOORDREMAPPER_H
#define __VERTEXCOORDREMAPPER_H

#include "MeshRemapper.h"
#include <panodata/Panorama.h>
#include <panodata/PanoramaOptions.h>
#include <panotools/PanoToolsInterface.h>

class VertexCoordRemapper : public MeshRemapper
{
public:
    VertexCoordRemapper(HuginBase::Panorama *m_pano, HuginBase::SrcPanoImage * image,
                       VisualizationState *visualization_state);
    virtual void UpdateAndResetIndex();
    // get the texture and vertex coordinates for the next face. The coordinates
    //    are ordered [left / right][top / bottom][x coord / y coord].
    virtual bool GetNextFaceCoordinates(Coords *result);
private:
           // texture coordinates for passing back in GetNextFaceCoordinates.
    double tex_coords[2][2][2],
           // spare vertex coordinates space used for copies we will change.
           s_vertex_coords[2][2][2],
           // the size of the output panorama.
           o_width, o_height,
           // extents in +/- 180 degree boundary crossing checking.
           radius, lower_bound, upper_bound, lower_bound_h, upper_bound_h,
           x_midpoint, y_midpoint,
           // values used to flip vertices across the 180 boundary
           x_add_360, y_add_360;
    
    // when we cross the 180 degree split there are two faces to give for every
    // face on the tree. Set this false when we should give the second in a pair
    bool done_node;
    unsigned int tree_node_id;
    // this stores what vertices need flipping.
    unsigned short int discontinuity_flags;
    
    // The output projection format we are using
    HuginBase::PanoramaOptions::ProjectionFormat output_projection;
    
    // set the coordinates of node node_id, decided which directions to
    //   subdivide in and then call recursively on any new faces.
    //   the 'stretch' arguments are needed if the parents of this node have
    //   subdivided in only one direction.
    void RecursiveUpdate(unsigned int node_id, unsigned int stretch_x,
                         unsigned int stretch_y);
    // decides when to subdivide. Stores the result in the node.
    void TestSubdivide(unsigned int node_id);
    
    // Where a face crosses a discontinuity the vertices end up on different
    // sides. Flip a vertex to the other side of the 180 degree seam.
    void DiscontinuityFlip(double vertex_c[2]);
        
    /* We make a quad tree of faces.
     * Each face has a copy of all the vertices at its corners, and knows if it
     * has been subdivided in each direction. If it has been subdivided, it is
     * not actually used for drawing, but the corners can be useful for children
     */
    class TreeNode
    {
    public:
        // the edges of the faces: [left/right][top/bottom][x coord / y coord]
        double verts[2][2][2];
        unsigned short int flags;
        // angle of the gradients of the left and top edge.
        float angle_x, angle_y;
        // the lengths of the left and top edge.
        float length_x, length_y;
    };
    /* the actual tree stores each tree in constant space, however the space is
     * quite large. This is so we allocate memory only on creation and have
     * very cheep lookup operations.
     */
    class Tree
    {
    public:
        void GetChildrenIds(const unsigned int node_num,
                            unsigned int children[4],
                            unsigned int &num_children);
        // The leaf nodes are the ones we want to be drawn
        // Select the first leaf node
        void ResetIndex();
        // return the node for the selected leaf and move on to the next.
        // returns 0 after they have all been used. (node 0 always subdivides
        // so cannot be a leaf node).
        unsigned int GetNext();
        // get the face that was subdivided to get the requested one.
        unsigned int GetParentId(const unsigned int nodenum);
        unsigned int GetParentId(unsigned int x, unsigned int y,
                                 unsigned int row_size, unsigned depth);
        unsigned int GetDepth(const unsigned int nodenum);
        // find the position of a node given its index
        void GetPosition(const unsigned int nodenum, unsigned int &x,
                         unsigned int &y, unsigned int &row_size,
                         unsigned int &depth);
        // find the index of a node, given information about its position.
        unsigned int GetIndex(const unsigned int x, const unsigned int y,
                              const unsigned int row_size,
                              unsigned int depth);
        // the scale and offset reduce the texture mapped region to the
        // rectangle the user has cropped the source image to.
        double x_crop_scale, y_crop_scale, x_crop_offs, y_crop_offs;
        void GetInputCoordinates(unsigned int node_num, double coords[2][2][2]);
        
        // tries to find a transformed coordinate in the tree that would have
        // been written before stop. If there are none, return 0, otherwise
        // set dest_x and dest_y and return the number of the node it used to
        // work that out. This takes into account where the subdivision has
        // stopped and performs linear interpolation in that case.
        unsigned int GetTransform(unsigned int src_x,   unsigned int src_y,
                                  unsigned int depth,
                                  unsigned int stop_x, unsigned int stop_y,
                                  double &dest_x, double &dest_y);
        // Array of as many nodes necessary to store a full tree with subdivions
        // up to max_depth (defined in VertexCoordRemapper.cpp) times.
        TreeNode nodes[1+4+16+64+256+1024+4096];
    private:
        unsigned int cur_tree_node;
    };
    Tree tree;
    void SetLengthAndAngle(TreeNode *node);
};

#endif

