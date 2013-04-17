// -*- c-basic-offset: 4 -*-

/** @file Mask.h
 *
 *  @brief definitions of classes to work with mask
 *
 *  @author Thomas Modes
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

#include <hugin_shared.h>
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

/** \brief base class, which stores one mask polygon 
 *  
 *  note: the mask handling (e.g. propagating of positive masks) happens in 
 *  HuginBase::Panorama::updateMasks which is automatic called after a 
 *  change to the panorama object by HuginBase::Panorama::changedFinished
 */
class IMPEX MaskPolygon
{
public:
    /** enumeration with type of possible masks */
    enum MaskType 
    {
        Mask_negative=0,
        Mask_positive=1,
        Mask_Stack_negative=2,
        Mask_Stack_positive=3,
        Mask_negative_lens=4
    };
    /** constructor */
    MaskPolygon() : m_maskType(Mask_negative), m_imgNr(0), m_invert(false) {};
    /** checks if given point is inside of the stored polygon */
    bool isInside(const FDiff2D p) const;
    /** returns the winding number of the polygon around point p */
    int getWindingNumber(const FDiff2D p) const;
    /** returns the total winding number of the polygon*/
    int getTotalWindingNumber() const;

    // access functions
    /** returns mask type */
    MaskType getMaskType() const { return m_maskType; };
    /** sets mask type */
    void setMaskType(const MaskType newType) { m_maskType=newType; };
    /** returns true, if mask type is positive */
    bool isPositive() const;
    /** returns vector with coordinates of the polygon */
    VectorPolygon getMaskPolygon() const { return m_polygon; };
    /** set complete vector with all corrdinates of the polygon */
    void setMaskPolygon(const VectorPolygon newMask);
    /** returns the associated image number, only used when loading a project, otherwise discarded */
    unsigned int getImgNr() const { return m_imgNr; };
    /** sets the associated image number, only used when loading a project, otherwise discarded */
    void setImgNr(const unsigned int newImgNr) { m_imgNr=newImgNr; };
    /** set mask to normal or inverted */
    void setInverted(const bool inverted) { m_invert = inverted; };
    /** returns if mask is inverted */
    bool isInverted() const { return m_invert; };

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
    /** clips the polygon to the circle with center and radius */
    bool clipPolygon(const FDiff2D center, const double radius);
    /** rotate the polygon by 90 degrees */
    void rotate90(bool clockwise,unsigned int maskWidth,unsigned int maskHeight);
    /** subsamples the polygon, so that the longest distance between 2 points is max_distance */
    void subSample(const double max_distance);

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
    /** calculates the bounding box of the polygon to speed up tests */
    void calcBoundingBox();
    //variables for internal storage of Mask type, polygon and assigned image number
    MaskType m_maskType;
    VectorPolygon m_polygon;
    unsigned int m_imgNr;
    bool m_invert;
    vigra::Rect2D m_boundingBox;
};

typedef std::vector<MaskPolygon> MaskPolygonVector;

/** load the mask from stream */
IMPEX void LoadMaskFromStream(std::istream& stream, vigra::Size2D& imageSize, MaskPolygonVector &newMasks, size_t imgNr);
/** save the mask into stream */
IMPEX void SaveMaskToStream(std::ostream& stream, vigra::Size2D imageSize, MaskPolygon &maskToWrite, size_t imgNr);

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
#endif // _PANODATA_MASK_H
