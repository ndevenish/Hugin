// -*- c-basic-offset: 4 -*-

/** @file MeshRemapper.h
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

/* A MeshRemapper is an abstract base class for objects that calculate an
 * approximate remap specified by quadrilatrials. Each quadrilatrial has vertex
 * coordinates and texture coordinates. The texture coordinates specify for each
 * vertex a point on the image to be mapped to that vertex location, where the
 * range 0 to 1 in either direction is the entirity of the image. The vertex
 * locations are the coordinates in the output in pixels. (They are scaled down
 * to fit in the preview).
 *
 * A mesh manager then converts the faces specifed into a format sutible for the
 * whatever graphics system is being used to draw them.
 */

#ifndef _MESHREMAPPER_H
#define _MESHREMAPPER_H

#include <vector>
#include <panodata/Panorama.h>
#include <panotools/PanoToolsInterface.h>

class ViewState;

class MeshRemapper
{
public:
    MeshRemapper(HuginBase::Panorama *m_pano, unsigned int image_number,
                         ViewState *view_state);
    virtual ~MeshRemapper();
    virtual void UpdateAndResetIndex();
    class Coords
    {
    public:
        // Warning: these are pointers to arrays, and GetNextFaceCoordinates
        // is expected to set the pointers, they won't be pointing at an array
        // of the correct size when passed in. The idea is that the MeshRemapper
        // has some data structures already storing the values, and assignment
        // of a pointer is quicker than assignment of 8 doubles. The pointer is
        // used like an array to get the two different x positions though.
        // the first actual array is the the positions in the opposite direction
        // and the last array is x and y components of that vertex.
        // tex_c is the coordinate in the image ranging from 0 to 1
        // vertex_c is the coordinate in the panorama, in its pixel space.
        double (*tex_c)[2][2];
        double (*vertex_c)[2][2];
    };
    class ArrayCoords
    {
    public:
        // like above, but with it's own memory.
        double tex_c[2][2][2];
        double vertex_c[2][2][2];
    };    
    // get the texture and vertex coordinates for the next face. The coordinates
    //    are ordered [left / right][top / bottom][x coord / y coord].
    // returns false once specified all faces.
    virtual bool GetNextFaceCoordinates(Coords *result) = 0;
protected:
    ViewState *view_state;
    HuginBase::Panorama *m_pano;
    unsigned int image_number;
    // this is the number number of units the between vertex coorinates that
    // gives a pixel in the display.
    float scale;
    // these are the sizes of the input images in pixels. Children should use
    // this to scale their texture coordinates since the Tranform uses image
    // pixels rather than scaling each image to the range 0 to 1.
    double height, width;
    HuginBase::PTools::Transform transform;
    
    // cropping of the source image
    HuginBase::SrcPanoImage::CropMode crop_mode;
    double crop_x1, crop_x2, crop_y1, crop_y2,
           circle_crop_centre_x, circle_crop_centre_y,
           circle_crop_radius_x, circle_crop_radius_y;
    bool circle_crop;
    void SetCrop(); // fill the above values
    // crop a face to the source image, return true if there is anything left.
    // SetCrop() must have been called beforehand with up to date information.
    void ClipFace(Coords *face);
    // Give a face that was produced by ClipFace and return true, or if there
    // are no faces, return false. The pointers in result must be valid.
    bool GiveClipFaceResult(Coords * result);
private:
    // when clipping makes lots of faces, we need a list of them:
    std::vector<MeshRemapper::ArrayCoords> face_list;
};

#endif

