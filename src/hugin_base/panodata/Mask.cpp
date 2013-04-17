// -*- c-basic-offset: 4 -*-

/** @file Mask.h
 *
 *  @brief declaration of classes to work with mask
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

// for debugging
#include <iostream>
#include <stdio.h>

#include "Mask.h"

#include <iostream>
#include <vector>
#include <panotools/PanoToolsInterface.h>
#include <panodata/PTScriptParsing.h>

namespace HuginBase {

using namespace hugin_utils;

bool MaskPolygon::isInside(const FDiff2D p) const
{
    if(m_polygon.size()<3)
        return false;
    if(!m_boundingBox.contains(vigra::Point2D(p.x,p.y)))
        return false;
    int wind=getWindingNumber(p);
    if(m_invert)
        return wind==0;
    else
        return wind!=0;
};

int MaskPolygon::getWindingNumber(const FDiff2D p) const
{
    // algorithm is modified version of winding number method
    // described at http://www.softsurfer.com/Archive/algorithm_0103/algorithm_0103.htm
    // Copyright 2001, softSurfer (www.softsurfer.com)
    // This code may be freely used and modified for any purpose
    // providing that this copyright notice is included with it.
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

bool MaskPolygon::isPositive() const
{
    return (m_maskType==Mask_positive) || 
           (m_maskType==Mask_Stack_positive);
};

void MaskPolygon::setMaskPolygon(const VectorPolygon newMask)
{
    m_polygon=newMask;
    calcBoundingBox();
};

void MaskPolygon::addPoint(const FDiff2D p) 
{
    m_polygon.push_back(p);
    calcBoundingBox();
};

void MaskPolygon::insertPoint(const unsigned int index, const FDiff2D p)
{
    if(index<=m_polygon.size())
    {
        m_polygon.insert(m_polygon.begin()+index,p);
        calcBoundingBox();
    };
};


void MaskPolygon::removePoint(const unsigned int index)
{
    if(index<m_polygon.size())
    {
        m_polygon.erase(m_polygon.begin()+index);
        calcBoundingBox();
    };
};

void MaskPolygon::movePointTo(const unsigned int index, const hugin_utils::FDiff2D p)
{
    if(index<m_polygon.size())
    {
        m_polygon[index].x=p.x;
        m_polygon[index].y=p.y;
        calcBoundingBox();
    };
};

void MaskPolygon::movePointBy(const unsigned int index, const hugin_utils::FDiff2D diff)
{
    if(index<m_polygon.size())
    {
        m_polygon[index].x+=diff.x;
        m_polygon[index].y+=diff.y;
        calcBoundingBox();
    };
};

void MaskPolygon::scale(const double factorx,const double factory)
{
    for(unsigned int i=0;i<m_polygon.size();i++)
    {
        m_polygon[i].x*=factorx;
        m_polygon[i].y*=factory;
    };
    calcBoundingBox();
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
    calcBoundingBox();
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

void MaskPolygon::calcBoundingBox()
{
    if(m_polygon.size()>0)
    {
        m_boundingBox.setUpperLeft(vigra::Point2D(m_polygon[0].x,m_polygon[0].y));
        m_boundingBox.setLowerRight(vigra::Point2D(m_polygon[0].x+1,m_polygon[0].y+1));
        if(m_polygon.size()>1)
        {
            for(unsigned int i=1;i<m_polygon.size();i++)
            {
                m_boundingBox|=vigra::Point2D(m_polygon[i].x,m_polygon[i].y);
            };
        };
        //adding a small border to get no rounding error because polygon coordinates are float
        //numbers, but bounding box has integer coordinates
        m_boundingBox.addBorder(2);
    };
};

//helper function for clipping
enum clipSide
{
    clipLeft=0,
    clipRight,
    clipTop,
    clipBottom
};

bool clip_isSide(const FDiff2D p, const vigra::Rect2D r, const clipSide side)
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

FDiff2D clip_getIntersection(const FDiff2D p, const FDiff2D q, const vigra::Rect2D r, const clipSide side)
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

VectorPolygon clip_onPlane(const VectorPolygon polygon, const vigra::Rect2D r, const clipSide side)
{
    if(polygon.size()<3)
    {
        return polygon;
    };
    FDiff2D s=polygon[polygon.size()-1];
    FDiff2D p;
    VectorPolygon newPolygon;
    for(unsigned int i=0;i<polygon.size();i++)
    {
        p=polygon[i];
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
    return newPolygon;
};

bool MaskPolygon::clipPolygon(const vigra::Rect2D rect)
{
    //clipping using Sutherland-Hodgman algorithm
    m_polygon=clip_onPlane(m_polygon, rect, clipLeft);
    m_polygon=clip_onPlane(m_polygon, rect, clipRight);
    m_polygon=clip_onPlane(m_polygon, rect, clipTop);
    m_polygon=clip_onPlane(m_polygon, rect, clipBottom);
    return (m_polygon.size()>2);
};

//helper function for clipping
/** check if point is inside circle
    @returns true, if point p is inside circle given by center and radius
    @param p point to test
    @param center center point of circle test
    @param radius radius of circle to test
*/
bool clip_insideCircle(const FDiff2D p, const FDiff2D center, const double radius)
{
    return p.squareDistance(center)<=radius*radius;
};

/** returns intersection of line and circle
    @param p fist point of line segment
    @param s seconst point of line segment
    @param center center of circle
    @param radius radius of circle
    @returns vector with all intersection of line between p and s and given circle
*/
std::vector<FDiff2D> clip_getIntersectionCircle(const FDiff2D p, const FDiff2D s, const FDiff2D center, const double radius)
{
    std::vector<FDiff2D> intersections;
    FDiff2D slope=s-p;
    if(slope.squareLength()<1e-5)
    {
        return intersections;
    };
    FDiff2D p2=p-center;
    double dotproduct=p2.x*slope.x+p2.y*slope.y;
    double root=sqrt(dotproduct*dotproduct-slope.squareLength()*(p2.squareLength()-radius*radius));
    double t1=(-dotproduct+root)/slope.squareLength();
    double t2=(-dotproduct-root)/slope.squareLength();
    std::set<double> t;
    if(t1>0 && t1<1)
    {
        t.insert(t1);
    };
    if(t2>0 && t2<1)
    {
        if(fabs(t2-t1)>1e-5)
        {
            t.insert(t2);
        };
    };
    if(t.size()>0)
    {
        for(std::set<double>::const_iterator it=t.begin();it!=t.end();it++)
        {
            intersections.push_back(p+slope*(*it));
        };
    };
    return intersections;
};

/** calculates angle between vector a and b in radians */
double angle_between(const FDiff2D a, const FDiff2D b)
{
    return asin((a.x*b.y-a.y*b.x)/(sqrt(a.squareLength())*sqrt(b.squareLength())));
};

/** adds an arc with given radius at the end of the polygon, the point is not added to the arc
    @param poly polygon to which the arc should added
    @param s point to which the arc should go
    @param center center of arc
    @param radius radius of arc
    @param clockwise true, if arc should go clockwise; else it goes anti-clockwise
*/
void generateArc(VectorPolygon& poly, const FDiff2D s, const FDiff2D center, const double radius, const bool clockwise)
{
    if(poly.size()==0)
    {
        return;
    };
    FDiff2D p=poly[poly.size()-1];
    double maxDistance=5.0;
    if(p.squareDistance(s)<maxDistance*maxDistance)
    {
        return;
    };
    double angle=atan2(p.y-center.y,p.x-center.x);
    double final_angle=atan2(s.y-center.y,s.x-center.x);
    //step 1 degree or less, so that max distance between 2 points is smaller than maxDistance
    double step=std::min<double>(PI/180,atan2(maxDistance,radius));
    if(!clockwise)
    {
        while(final_angle<angle)
        {
            final_angle+=2*PI;
        };
        angle+=step;
        while(angle<final_angle)
        {
            poly.push_back(FDiff2D(cos(angle)*radius+center.x,sin(angle)*radius+center.y));
            angle+=step;
        };
    }
    else
    {
        while(final_angle>angle)
        {
            final_angle-=2*PI;
        };
        angle-=step;
        while(angle>final_angle)
        {
            poly.push_back(FDiff2D(cos(angle)*radius+center.x,sin(angle)*radius+center.y));
            angle-=step;
        };
    };
};

bool MaskPolygon::clipPolygon(const FDiff2D center,const double radius)
{
    if(radius<=0 || m_polygon.size()<3)
    {
        return false;
    };
    FDiff2D s=m_polygon[m_polygon.size()-1];
    bool s_inside=clip_insideCircle(s,center,radius);
    FDiff2D p;
    VectorPolygon newPolygon;
    bool needsFinalArc=false;
    double angleCovered=0;
    double angleCoveredOffset=0;
    for(unsigned int i=0;i<m_polygon.size();i++)
    {
        p=m_polygon[i];
        bool p_inside=clip_insideCircle(p,center,radius);
        if(p_inside)
        {
            if(s_inside)
            {
                //both points inside
                newPolygon.push_back(p);
            }
            else
            {
                //line crosses circles from outside
                std::vector<FDiff2D> points=clip_getIntersectionCircle(p,s,center,radius);
                DEBUG_ASSERT(points.size()==1);
                angleCovered+=angle_between(s-center,points[0]-center);
                if(newPolygon.size()==0)
                {
                    needsFinalArc=true;
                    angleCoveredOffset=angleCovered;
                }
                else
                {
                    generateArc(newPolygon,points[0],center,radius,angleCovered<0);
                };
                newPolygon.push_back(points[0]);
                newPolygon.push_back(p);
            };
        }
        else
        {
            if(!s_inside)
            {
                //both points outside of circle
                std::vector<FDiff2D> points=clip_getIntersectionCircle(s,p,center,radius);
                //intersection can only be zero points or 2 points
                if(points.size()>1)
                {
                    angleCovered+=angle_between(s-center,points[0]-center);
                    if(newPolygon.size()==0)
                    {
                        needsFinalArc=true;
                        angleCoveredOffset=angleCovered;
                    }
                    else
                    {
                        generateArc(newPolygon,points[0],center,radius,angleCovered<0);
                    };
                    newPolygon.push_back(points[0]);
                    newPolygon.push_back(points[1]);
                    angleCovered=angle_between(points[1]-center,p-center);
                }
                else
                {
                    angleCovered+=angle_between(s-center,p-center);
                };
            }
            else
            {
                //line segment intersects circle from inside
                std::vector<FDiff2D> points=clip_getIntersectionCircle(s,p,center,radius);
                angleCovered=0;
                DEBUG_ASSERT(points.size()==1);
                newPolygon.push_back(points[0]);
            };
        };
        s=p;
        s_inside=p_inside;
    };
    if(needsFinalArc && newPolygon.size()>1)
    {
        generateArc(newPolygon,newPolygon[0],center,radius,(angleCovered+angleCoveredOffset)<0);
    };        
    m_polygon=newPolygon;
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
    o<<"t"<<(int)m_maskType<<" ";
    o<<"p\"";
    for(unsigned int i=0; i<m_polygon.size(); i++)
    {
        o<<m_polygon[i].x<<" "<<m_polygon[i].y;
        if((i+1)!=m_polygon.size())
            o<<" ";
    };
    o<<"\""<<std::endl;
};

void LoadMaskFromStream(std::istream& stream,vigra::Size2D& imageSize, MaskPolygonVector &newMasks, size_t imgNr)
{
    while (stream.good()) 
    {
        std::string line;
        std::getline(stream,line);
        switch (line[0]) 
        {
            case '#':
            {
                unsigned int w;
                if (PTScriptParsing::getIntParam(w, line, "w"))
                    imageSize.setWidth(w);
                unsigned int h;
                if (PTScriptParsing::getIntParam(h, line, "h"))
                    imageSize.setHeight(h);
                break;
            }
            case 'k':
            {
                HuginBase::MaskPolygon newPolygon;
                //Ignore image number set in mask
                newPolygon.setImgNr(imgNr);
                unsigned int param;
                if (PTScriptParsing::getIntParam(param,line,"t"))
                {
                    newPolygon.setMaskType((HuginBase::MaskPolygon::MaskType)param);
                }
                std::string format;
                if (PTScriptParsing::getPTParam(format,line,"p"))
                {
                    if(newPolygon.parsePolygonString(format)) {
                        newMasks.push_back(newPolygon);
                    } 
                }
                break;
            }
            default:
            {
                break;
            }
        }
    }
};

void SaveMaskToStream(std::ostream& stream, vigra::Size2D imageSize, MaskPolygon &maskToWrite, size_t imgNr)
{
    stream << "# w" << imageSize.width() << " h" << imageSize.height() << std::endl;
    maskToWrite.printPolygonLine(stream, imgNr);
};

} // namespace
