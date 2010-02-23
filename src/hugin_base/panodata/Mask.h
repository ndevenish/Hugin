// -*- c-basic-offset: 4 -*-

/** @file Mask.h
 *
 *  @brief definitions of classes to work with mask
 *
 *  @author Thomas Modes
 *
 *
 *  $Id$
 *
 */

/*  This program is free software; you can redistribute it and/or
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

#ifndef _PANODATA_MASK_H
#define _PANODATA_MASK_H

#include "hugin_utils/utils.h"
#include "hugin_math/hugin_math.h"

namespace HuginBase 
{
namespace PTools { class Transform; }

using namespace hugin_utils;

/** vector, which stores coordinates of one polygon */
typedef std::vector<FDiff2D> VectorPolygon;

/** polygon can exceed the image maximal maskOffset pixels in each direction
 *  bigger polygons will be clipped after loading 
 */
const int maskOffset=100;

/** base class, which stores one polygon */
class MaskPolygon
{
public:
    /** enumeration with type of possible masks */
    enum MaskType 
    {
        Mask_negative=0,
        Mask_positive=1
    };
    /** constructor */
    MaskPolygon()
    {
        m_maskType=Mask_negative;
    };
    /** checks if given point is inside of the stored polygon */
    bool isInside(const FDiff2D p) const;

    // access functions
    /** returns mask type */
    MaskType getMaskType() const { return m_maskType; };
    /** sets mask type */
    void setMaskType(const MaskType newType) { m_maskType=newType; };
    /** returns vector with coordinates of the polygon */
    VectorPolygon getMaskPolygon() const { return m_polygon; };
    /** set complete vector wit all corrdinates of the polygon */
    void setMaskPolygon(const VectorPolygon newMask) { m_polygon = newMask; };
    /** returns the associated image number, only used when loading a project, otherwise discarded */
    unsigned int getImgNr() const { return m_imgNr; };
    /** sets the associated image number, only used when loading a project, otherwise discarded */
    void setImgNr(const unsigned int newImgNr) { m_imgNr=newImgNr; };

    // polygon modifier
    /** adds point at the end to the polygon */
    void addPoint(const FDiff2D p);
    /** insert point at the position index into the polygon */
    void insertPoint(const unsigned int index, const FDiff2D p);
    /** removes point at the position index from the polygon */
    void removePoint(const unsigned int index);
    /** moves the point at position index to the new absolute position p */
    void movePointTo(const unsigned int index, const FDiff2D p);
    /** relativ moves the point at position index by diff */
    void movePointBy(const unsigned int index, const FDiff2D diff);
    /** scales all polygon coordinates by factorx for x position and factory for y position */
    void scale(const double factorx, const double factory);
    /** scales x and y axis equally by factor */
    void scale(const double factor) { scale(factor,factor);} ;
    /** transforms the polygon coordinates by the given transformation */
    void transformPolygon(const PTools::Transform &trans);
    /** clips the polygon to the given rectangle */
    bool clipPolygon(const vigra::Rect2D rect);
    /** rotate the polygon by 90 degrees */
    void rotate90(bool clockwise,unsigned int maskWidth,unsigned int maskHeight);
    /** search a point which lies near the polygon line and return the index for inserting the new point */
    unsigned int FindPointNearPos(const FDiff2D p, const double tol);

    //operators
    /** assign operator */
    MaskPolygon &operator=(const MaskPolygon otherPoly);
    /** comparision operator */
    const bool operator==(const MaskPolygon &otherPoly) const;

    //input/output functions
    /** parses the x and y positions from the given string */
    bool parsePolygonString(const std::string polygonStr);
    /** writes the complete k line which describes the mask to the stream, using the given
     *  newImgNr for the i parameter
     */
    void printPolygonLine(std::ostream & o, const unsigned int newImgNr) const;

private:
    // helper functions for clipping using Sutherland-Hodgeman Clipping Algorithm
    enum clipSide
    {
        clipLeft=0,
        clipRight,
        clipTop,
        clipBottom
    };
    bool clip_isSide(const FDiff2D p, const vigra::Rect2D r, const clipSide side);
    FDiff2D clip_getIntersection(const FDiff2D p, const FDiff2D q, const vigra::Rect2D r, const clipSide side);
    void clip_onPlane(const vigra::Rect2D r, clipSide side);

    //variables for internal storage of Mask type, polygon and assigned image number
    MaskType m_maskType;
    VectorPolygon m_polygon;
    unsigned int m_imgNr;
};

typedef std::vector<MaskPolygon> MaskPolygonVector;

}; //namespace

namespace vigra_ext 
{

template <class SrcImageIterator, class SrcAccessor>
void applyMask(vigra::triple<SrcImageIterator, SrcImageIterator, SrcAccessor> img, HuginBase::MaskPolygonVector masks)
{
    vigra::Diff2D imgSize = img.second - img.first;

    if(masks.size()<1)
        return;
    // create dest y iterator
    SrcImageIterator yd(img.first);
    // loop over the image and transform
    for(int y=0; y < imgSize.y; ++y, ++yd.y)
    {
        // create x iterators
        SrcImageIterator xd(yd);
        for(int x=0; x < imgSize.x; ++x, ++xd.x)
        {
            HuginBase::FDiff2D newPoint(x,y);
            bool insideMasks=false;
            unsigned int i=0;
            while(!insideMasks && (i<masks.size()))
            {
                insideMasks=masks[i].isInside(newPoint);
                i++;
            };
            if(insideMasks)
                *xd=0;
        }
    }
}

} //namespace
#endif // PANOIMAGE_H
