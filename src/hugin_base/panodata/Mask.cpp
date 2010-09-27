// -*- c-basic-offset: 4 -*-

/** @file Mask.h
 *
 *  @brief declaration of classes to work with mask
 *
 *  @author Thomas Modes
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

// for debugging
#include <iostream>
#include <stdio.h>

#include "Mask.h"

#include <iostream>
#include <vector>
//#include <vigra/diff2d.hxx>
//#include <vigra/imageinfo.hxx>
#include <panotools/PanoToolsInterface.h>


namespace HuginBase {

using namespace hugin_utils;

bool MaskPolygon::isInside(const FDiff2D p) const
{
    int wind=getWindingNumber(p);
    if(m_invert)
        return wind==0;
    else
        return wind!=0;
};

int MaskPolygon::getWindingNumber(const FDiff2D p) const
{
    if(m_polygon.size()<3)
        return 0;
    int wind=0;
    FDiff2D a=m_polygon[m_polygon.size()-1];
    for(unsigned int i=0;i<m_polygon.size();i++)
    {
        FDiff2D b=m_polygon[i];
        if(a.y<=p.y)
        {
            if(b.y>p.y)
                if((b.x-a.x)*(p.y-a.y)<(p.x-a.x)*(b.y-a.y))
                    wind++;
        }
        else
        {
            if(b.y<=p.y)
                if((b.x-a.x)*(p.y-a.y)>(p.x-a.x)*(b.y-a.y))
                    wind--;
        };
        a=b;
    };
    return wind;
};

int MaskPolygon::getTotalWindingNumber() const
{
    if(m_polygon.size()<2)
        return 0;
    MaskPolygon diffPoly;
    unsigned int count=m_polygon.size();
    for(unsigned int i=0;i<count;i++)
    {
        diffPoly.addPoint(m_polygon[(i+1)%count]-m_polygon[i]);
    };
    return diffPoly.getWindingNumber(FDiff2D(0,0));
};

void MaskPolygon::addPoint(const FDiff2D p) 
{
    m_polygon.push_back(p);
};

void MaskPolygon::insertPoint(const unsigned int index, const FDiff2D p)
{
    if(index<=m_polygon.size())
        m_polygon.insert(m_polygon.begin()+index,p);
};

void MaskPolygon::removePoint(const unsigned int index)
{
    if(index<m_polygon.size())
    {
        m_polygon.erase(m_polygon.begin()+index);
    };
};

void MaskPolygon::movePointTo(const unsigned int index, const hugin_utils::FDiff2D p)
{
    if(index<m_polygon.size())
    {
        m_polygon[index].x=p.x;
        m_polygon[index].y=p.y;
    };
};

void MaskPolygon::movePointBy(const unsigned int index, const hugin_utils::FDiff2D diff)
{
    if(index<m_polygon.size())
    {
        m_polygon[index].x+=diff.x;
        m_polygon[index].y+=diff.y;
    };
};

void MaskPolygon::scale(const double factorx,const double factory)
{
    for(unsigned int i=0;i<m_polygon.size();i++)
    {
        m_polygon[i].x*=factorx;
        m_polygon[i].y*=factory;
    };
};

void MaskPolygon::transformPolygon(const PTools::Transform &trans)
{
    double xnew,ynew;
    VectorPolygon newPoly;
    for(unsigned int i=0;i<m_polygon.size();i++)
    {
        if(trans.transformImgCoord(xnew,ynew,m_polygon[i].x,m_polygon[i].y))
        {
            newPoly.push_back(FDiff2D(xnew,ynew));
        };
    };
    m_polygon=newPoly;
};

void MaskPolygon::subSample(const double max_distance)
{
    if(m_polygon.size()<3)
        return;
    VectorPolygon oldPoly=m_polygon;
    unsigned int count=oldPoly.size();
    m_polygon.clear();
    for(unsigned int i=0;i<count;i++)
    {
        addPoint(oldPoly[i]);
        FDiff2D p1=oldPoly[i];
        FDiff2D p2=oldPoly[(i+1)%count];
        double distance=norm(p2-p1);
        if(distance>max_distance)
        {
            //add intermediate points
            double currentDistance=max_distance;
            while(currentDistance<distance)
            {
                FDiff2D p_new=p1+(p2-p1)*currentDistance/distance;
                addPoint(p_new);
                currentDistance+=max_distance;
            };
        };
    };
};

bool MaskPolygon::clipPolygon(const vigra::Rect2D rect)
{
    //clipping using Sutherland-Hodgman algorithm
    clip_onPlane(rect, clipLeft);
    clip_onPlane(rect, clipRight);
    clip_onPlane(rect, clipTop);
    clip_onPlane(rect, clipBottom);
    return (m_polygon.size()>2);
};

void MaskPolygon::rotate90(bool clockwise,unsigned int maskWidth,unsigned int maskHeight)
{
    for(unsigned int i=0;i<m_polygon.size();i++)
    {
        if(clockwise)
        {
            FDiff2D p=m_polygon[i];
            m_polygon[i].x=maskHeight-p.y;
            m_polygon[i].y=p.x;
        }
        else
        {
            FDiff2D p=m_polygon[i];
            m_polygon[i].x=p.y;
            m_polygon[i].y=maskWidth-p.x;
        };
    };
};

unsigned int MaskPolygon::FindPointNearPos(const FDiff2D p, const double tol)
{
    if(m_polygon.size()==0)
        return UINT_MAX;
    FDiff2D p1;
    unsigned int j=m_polygon.size()-1;
    FDiff2D p2=m_polygon[j];
    for(unsigned int i=0;i<m_polygon.size();i++)
    {
        p1=m_polygon[i];
        // find intersection of perpendicular through point p and line between point i and j 
        FDiff2D diff=p2-p1;
        if(norm(diff)<0.001)
            continue;
        double u=((p.x-p1.x)*(p2.x-p1.x)+(p.y-p1.y)*(p2.y-p1.y))/sqr(norm(diff));
        if((u>=0.1) && (u<=0.9))
        {
            // intersection is between p1 and p2
            FDiff2D footpoint=p1+diff*u;
            // now check distance between intersection and p
            if(norm(p-footpoint)<tol)
                return i==0 ? j+1 : i;
        };
        j=i;
        p2=p1;
    };
    return UINT_MAX;
};

MaskPolygon &MaskPolygon::operator=(const MaskPolygon otherPoly)
{
    if (this == &otherPoly)
        return *this;
    setMaskType(otherPoly.getMaskType());
    setMaskPolygon(otherPoly.getMaskPolygon());
    setImgNr(otherPoly.getImgNr());
    setInverted(otherPoly.isInverted());
    return *this;
};

const bool MaskPolygon::operator==(const MaskPolygon &otherPoly) const
{
    return ((m_maskType == otherPoly.getMaskType()) && (m_polygon == otherPoly.getMaskPolygon()));
};

bool MaskPolygon::parsePolygonString(const std::string polygonStr)
{
    m_polygon.clear();
    if(polygonStr.length()==0)
        return false;
    std::stringstream is(polygonStr);
    while(is.good())
    {
        double x;
        double y;
        if(is>>x)
            if(is>>y)
                m_polygon.push_back(FDiff2D(x,y));
    };
    return m_polygon.size()>2;
};

void MaskPolygon::printPolygonLine(std::ostream &o, const unsigned int newImgNr) const
{
    o<<"k i"<<newImgNr<<" ";
    switch(m_maskType)
    {
    case Mask_negative:
        o<<"t0 ";
        break;
    case Mask_positive:
        o<<"t1 ";
        break;
    };
    o<<"p\"";
    for(unsigned int i=0; i<m_polygon.size(); i++)
    {
        o<<m_polygon[i].x<<" "<<m_polygon[i].y;
        if((i+1)!=m_polygon.size())
            o<<" ";
    };
    o<<"\""<<std::endl;
};

//helper function for clipping
bool MaskPolygon::clip_isSide(const FDiff2D p, const vigra::Rect2D r, const clipSide side)
{
    switch(side){
        case clipLeft:
            return p.x>=r.left();
        case clipRight:
            return p.x<=r.right();
        case clipTop:
            return p.y>=r.top();
        case clipBottom:
            return p.y<=r.bottom();
    };
    //this should never happens
    return false;
}

FDiff2D MaskPolygon::clip_getIntersection(const FDiff2D p, const FDiff2D q, const vigra::Rect2D r, const clipSide side)
{
    double a;
    double b;
    double xnew;
    double ynew;
    if(q.x-p.x==0)
    {
        a=0;
        b=p.y;
    }
    else
    {
        a=(q.y-p.y)/(q.x-p.x);
        b=p.y-p.x*a;
    };
    switch(side){
        case clipLeft:
            xnew=r.left();
            ynew=xnew*a+b;
            break;
        case clipRight:
            xnew=r.right();
            ynew=xnew*a+b;
            break;
        case clipTop:
            ynew=r.top();
            if(a!=0)
                xnew=(ynew-b)/a;
            else
                xnew=p.x;
            break;
        case clipBottom:
            ynew=r.bottom();
            if(a!=0)
                xnew=(ynew-b)/a;
            else
                xnew=p.x;
            break;
    };
    return FDiff2D(xnew,ynew);
};

void MaskPolygon::clip_onPlane(const vigra::Rect2D r, const clipSide side)
{
    if(m_polygon.size()<3)
        return;
    FDiff2D s=m_polygon[m_polygon.size()-1];
    FDiff2D p;
    VectorPolygon newPolygon;
    for(unsigned int i=0;i<m_polygon.size();i++)
    {
        p=m_polygon[i];
        if(clip_isSide(p,r,side))
        {
            // point p is "inside"
            if(!clip_isSide(s,r,side))
                // and point s is "outside"
                newPolygon.push_back(clip_getIntersection(p,s,r,side));
            newPolygon.push_back(p);
        }
        else
        {
            //point p is "outside"
            if(clip_isSide(s,r,side))
                //ans point s is "inside"
                newPolygon.push_back(clip_getIntersection(s,p,r,side));
        };
        s=p;
    };
    m_polygon=newPolygon;
};

} // namespace
