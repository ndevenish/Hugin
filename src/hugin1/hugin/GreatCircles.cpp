// -*- c-basic-offset: 4 -*-
/** @file GreatCircles.cpp
 *
 *  @author James Legg
 *  @author Darko Makreshanski
 *
 *  @brief Implement GreatCircles class.
 *
 *  Copyright 2009 James Legg.
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
 */

#ifdef __WXMAC__
#include "panoinc_WX.h"
#include "panoinc.h"
#endif

#include "GreatCircles.h"

// for some reason #include <GL/gl.h> isn't portable enough.
#include <wx/platform.h>
#ifdef __WXMAC__
#include <OpenGL/gl.h>
#else
#ifdef __WXMSW__
#include <vigra/windows.h>
#endif
#include <GL/gl.h>
#endif

// we use the trigonometric functions.
#include <cmath>
#include <limits>

// The number of segments to use in subdivision of the line.
// Higher numbers increase the accuracy of the line, but it is slower.
// Must be at least two. More is much better.
const unsigned int segments = 48;

void GreatCircles::setVisualizationState(VisualizationState * visualizationStateIn)
{
    m_visualizationState = visualizationStateIn;
}

void GreatCircles::drawLineFromSpherical(double startLat, double startLong,
                                         double endLat, double endLong, double width)
{
    DEBUG_ASSERT(m_visualizationState); 
    GreatCircleArc(startLat, startLong, endLat, endLong, *m_visualizationState).draw(true, width);
}

GreatCircleArc::GreatCircleArc()
{
}

GreatCircleArc::GreatCircleArc(double startLat, double startLong,
                       double endLat, double endLong,
                       VisualizationState & visualizationState)

{
    m_visualizationState = &visualizationState;
    // get the output projection
    const HuginBase::PanoramaOptions & options = *(visualizationState.GetOptions());
    // make an image to transform spherical coordinates into the output projection
    HuginBase::SrcPanoImage equirectangularImage;
    equirectangularImage.setProjection(HuginBase::SrcPanoImage::EQUIRECTANGULAR);
    equirectangularImage.setHFOV(360.0);
    equirectangularImage.setSize(vigra::Size2D(360.0, 180.0));
    
    // make a transformation from spherical coordinates to the output projection
    HuginBase::PTools::Transform transform;
    transform.createInvTransform(equirectangularImage, options);
    
	m_xscale = visualizationState.GetScale();

    /**Handle case where the points are opposite sides of the sphere
     * (i.e. The angle startLat is -endLat and startLong is -endLong.)
     * There are infinetly many great circles in this case, we pick one going
     * through (180, 90), by splitting the problem in two.
     */
    if (startLat == 360.0 - endLat && startLong == 180.0 - endLong)
    {
        // we should probably check to see if we already go through (180, 90).
        if (startLat == 180.0 && startLong == 90.0)
        {
            // foiled again: pick one going through (180, 0) instead.
            *this = GreatCircleArc(startLat, startLong, 180.0, 0.0, visualizationState);
            GreatCircleArc other(180.0, 0.0, endLat, endLong, visualizationState);
            m_lines.insert(m_lines.end(), other.m_lines.begin(), other.m_lines.end());
            return;
        }
        *this = GreatCircleArc(startLat, startLong, 180.0, 90.0, visualizationState);
        GreatCircleArc other(180.0, 90.0, endLat, endLong, visualizationState);
        m_lines.insert(m_lines.end(), other.m_lines.begin(), other.m_lines.end());
        return;
    }
    
    // convert start and end positions so that they don't go across the +/-180
    // degree seam
    if (startLat < 90.0 && endLat > 270.0)
    {
        endLat -= 360.0;
    }
    // convert to radians
    startLat *= (M_PI / 180.0);
    startLong *= (M_PI / 180.0);
    endLat *= (M_PI / 180.0);
    endLong *= (M_PI / 180.0);
    
    // find sines and cosines, they are used multiple times.
    double sineStartLat = std::sin(startLat);
    double sineStartLong = std::sin(startLong);
    double sineEndLat = std::sin(endLat);
    double sineEndLong = std::sin(endLong);
    double cosineStartLat = std::cos(startLat);
    double cosineStartLong = std::cos(startLong);
    double cosineEndLat = std::cos(endLat);
    double cosineEndLong = std::cos(endLong);
    
    /* to get points on the great circle, we linearly interpolate between the
     * two 3D coordinates for the given spherical coordinates, then normalise
     * the vector to get back on the sphere. This works everywhere except exact
     * opposite points, where we'll get the original points repeated several
     * times (and if we are even more unlucky we will hit the origin where the
     * normal isn't defined.)
     */
    // convert locations to 3d coordinates.
    double p1[3] = {cosineStartLat * sineStartLong, sineStartLat * sineStartLong, cosineStartLong};
    double p2[3] = {cosineEndLat * sineEndLong, sineEndLat * sineEndLong, cosineEndLong};
    
    ///@todo don't check the +/- 180 degree boundary when projection does not break there.
    bool hasSeam = true;
    // draw a line strip and transform the coordinates as we go.
    double b1 = 0.0;
    double b2 = 1.0;
    const double bDifference = 1.0 / double(segments);
    // for discontinuity detection.
    int lastSegment = 1;
    // The last processed vertex's position.
    hugin_utils::FDiff2D last_vertex;
    /* true if we shouldn't use last_vertex to make a line segment
     * i.e. We just crossed a discontinuity. */
    bool skip = true;
    for (unsigned int segment_index = 0; segment_index < segments;
         segment_index++, b1 += bDifference, b2 -= bDifference)
    {
        // linearly interpolate positions
        double v[3] = {p1[0] * b1 + p2[0] * b2, p1[1] * b1 + p2[1] * b2, p1[2] * b1 + p2[2] * b2};
        // normalise
        double length = sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
        v[0] /= length;
        v[1] /= length;
        v[2] /= length;
        /*double longitude = atan2(numerator,
                                 cosineStartLong * (c1 * std::sin(latitude) -
                                                    c2 * std::cos(latitude)));*/
        double longitude = std::acos(v[2]);
        // acos returns values between 0 and M_PI. The other
        // latitudes are on the back of the sphere, so check y coordinate (v1)
        double latitude = std::acos(v[0] / std::sin(longitude));
        if (v[1] < 0.0)
        {
            // on the back.
            latitude = -latitude + 2 * M_PI;
        }
        
        double vx, vy;
        bool infront =  transform.transformImgCoord(vx,
                                                    vy,
                                                    latitude  * 180.0 / M_PI,
                                                    longitude  * 180.0 / M_PI);
        // don't draw across +/- 180 degree seems.
        if (hasSeam)
        {
            // we divide the width of the panorama into 3 segments. If we jump
            // across the middle section, we split the line into two.
            int newSegment = vx / (options.getWidth() / 3);
            if ((newSegment < 1 && lastSegment > 1) ||
                (newSegment > 1 && lastSegment < 1))
            {
                skip = true;
            }
            lastSegment = newSegment;
        }
        if (infront)
        {
            if (!skip)
            {
                LineSegment line;
                line.vertices[0] = last_vertex;
                line.vertices[1] = hugin_utils::FDiff2D(vx, vy);
                m_lines.push_back(line);
            }
            // The next line segment should be a valid one.
            last_vertex = hugin_utils::FDiff2D(vx, vy);
            skip = false;
        } else {
            skip = true;
        }
    }
}
                       
void GreatCircleArc::draw(bool withCross, double width) const
{
    // Just draw all the previously worked out line segments
    /** @todo It is probably more apropriate to use thin rectangles than lines.
     * There are hardware defined limits on what width a line can be, and the
     * worst case is the hardware only alows lines 1 pixel thick.
     */
        LineSegment * pre;
        LineSegment * pro;
        for (unsigned int i = 0 ; i < m_lines.size() ; i++) {
            if (i != 0) {
                pre = (LineSegment*) &(m_lines[i-1]);
            } else {
                pre = NULL;
            }
            if (i != m_lines.size() - 1) {
                pro = (LineSegment*)&(m_lines[i+1]);
            } else {
                pro = NULL;
            }
            m_lines[i].doGL(width, m_visualizationState,pre,pro);
        }
    glEnd();
    // The arc might contain no line segments in some circumstances,
    // so check for this before drawing crosses at the ends.
    if(withCross && !m_lines.empty())
    {
        double scale = 4 / getxscale();
        // The scale to draw them: this is 5 pixels outside in every direction.
        {
            std::vector<GreatCircleArc::LineSegment>::const_iterator it;
            it = m_lines.begin();
            it->doGLcross(0,scale, m_visualizationState);

            it = m_lines.end();
            it--;	//.end points beyond last point.
            it->doGLcross(1,scale, m_visualizationState);
        }
    };
}

void GreatCircleArc::LineSegment::doGL(double width, VisualizationState * state, GreatCircleArc::LineSegment * preceding, GreatCircleArc::LineSegment * proceeding) const
{
    double m = (vertices[1].y - vertices[0].y) / (vertices[1].x - vertices[0].x);
    double b = vertices[1].y - m * vertices[1].x;
    hugin_utils::FDiff2D rect[4];
    double d = width / state->GetScale() / 2.0;
//DEBUG_DEBUG("drawing lines " << d);
    double yd = d * sqrt(m*m + 1);
    double xd =  d * sin(atan(m));

    if (vertices[1].x < vertices[0].x) {
        yd *= -1;
    } else {
        xd *= -1;
    }

    //first find out if it is a special case

    bool vertical = false;
    if (abs(vertices[1].x - vertices[0].x) < 0.00001) {
        xd = d;
        if (vertices[1].y > vertices[0].y) {
            xd *= -1; 
        }
        yd = 0;
        vertical = true;
    }

    bool horizontal = false;
    if(abs(vertices[1].y - vertices[0].y) < 0.00001) {
        xd = 0;
        yd = d;
        if (vertices[1].x > vertices[0].x) {
            yd *= -1; 
        }
        m = 0;
        b = vertices[0].y;
        horizontal = true;
    }

    //TODO: line segment ends of special cases correspondend to preceding and proceeding segments
    if (vertical || horizontal) {
        rect[0].x = vertices[0].x - xd;
        rect[0].y = vertices[0].y + yd;
        rect[1].x = vertices[0].x + xd;
        rect[1].y = vertices[0].y - yd;
        rect[2].x = vertices[1].x + xd;
        rect[2].y = vertices[1].y - yd;
        rect[3].x = vertices[1].x - xd;
        rect[3].y = vertices[1].y + yd;
    } else {

        //The mathematics behind this is to get the equation for the line of the segment in the form of y = m * x + b
        //then shift the line + - in the y direction to get two lines, and finally do this for the preceding and proceeding line segments and find the intersection
        //or just make the ends perpendicular
        bool def = false;
        if (preceding != NULL) {

            double m_pre = (preceding->vertices[1].y - preceding->vertices[0].y) / (preceding->vertices[1].x - preceding->vertices[0].x);
            double b_pre = preceding->vertices[1].y - m_pre * preceding->vertices[1].x;

            if (m_pre == m) {
                def = true;
            } else {

                double yd_pre = d * sqrt(m_pre*m_pre + 1);
                if (preceding->vertices[1].x < preceding->vertices[0].x) {
                    yd_pre *= -1;
                }


                rect[1].x = ((b_pre + yd_pre) - (b + yd)) / (m - m_pre);
                rect[1].y = m_pre * rect[1].x + b_pre + yd_pre;

                rect[0].x = ((b_pre - yd_pre) - (b - yd)) / (m - m_pre);
                rect[0].y = m_pre * rect[0].x + b_pre - yd_pre;
            }
            
        } else {
            def = true;
        }

        if (def) {

            //in this case return just a proper rectangle
            rect[1].x = vertices[0].x     + xd;
            rect[1].y = m * rect[1].x + b + yd;

            rect[0].x = vertices[0].x     - xd;
            rect[0].y = m * rect[0].x + b - yd;

        }

//        DEBUG_DEBUG("");
//        DEBUG_DEBUG("drawing line " << vertices[0].x << " " << vertices[0].y << " ; " << vertices[1].x << " " << vertices[1].y);
//        DEBUG_DEBUG("drawing line r " << rect[0].x << " " << rect[0].y << " ; " << rect[1].x << " " << rect[1].y);
//        DEBUG_DEBUG("drawing line a " << m << " " << b << " yd: " << yd << " xd: " << xd << " ; " << vertices[1].x - vertices[0].x);

        def = false;
        if (proceeding != NULL) {


            double m_pro = (proceeding->vertices[1].y - proceeding->vertices[0].y) / (proceeding->vertices[1].x - proceeding->vertices[0].x);
            double b_pro = proceeding->vertices[1].y - m_pro * proceeding->vertices[1].x;

            if (m_pro == m) {
                def = true;
            } else {

                double yd_pro = d * sqrt(m_pro*m_pro + 1);
                if (proceeding->vertices[1].x < proceeding->vertices[0].x) {
                    yd_pro *= -1;
                }


                rect[3].x = ((b_pro - yd_pro) - (b - yd)) / (m - m_pro);
                rect[3].y = m_pro * rect[3].x + b_pro - yd_pro;

                rect[2].x = ((b_pro + yd_pro) - (b + yd)) / (m - m_pro);
                rect[2].y = m_pro * rect[2].x + b_pro + yd_pro;

            }

        } else {
            def = true;
        }

        if (def) {

            rect[3].x = vertices[1].x     - xd;
            rect[3].y = m * rect[3].x + b - yd;

            rect[2].x = vertices[1].x     + xd;
            rect[2].y = m * rect[2].x + b + yd;
        }

//    DEBUG_DEBUG("");
//    DEBUG_DEBUG("drawing line " << vertices[0].x << " " << vertices[0].y << " ; " << vertices[1].x << " " << vertices[1].y);
//    DEBUG_DEBUG("drawing line r " << rect[0].x << " " << rect[0].y << " ; " << rect[1].x << " " << rect[1].y);
//    DEBUG_DEBUG("drawing line r " << rect[2].x << " " << rect[2].y << " ; " << rect[3].x << " " << rect[3].y);

    }
    


    MeshManager::MeshInfo::Coord3D res1 = state->GetMeshManager()->GetCoord3D((hugin_utils::FDiff2D&)rect[0]);
    MeshManager::MeshInfo::Coord3D res2 = state->GetMeshManager()->GetCoord3D((hugin_utils::FDiff2D&)rect[1]);
    MeshManager::MeshInfo::Coord3D res3 = state->GetMeshManager()->GetCoord3D((hugin_utils::FDiff2D&)rect[2]);
    MeshManager::MeshInfo::Coord3D res4 = state->GetMeshManager()->GetCoord3D((hugin_utils::FDiff2D&)rect[3]);
    glBegin(GL_QUADS);
    glVertex3d(res1.x,res1.y,res1.z);
    glVertex3d(res2.x,res2.y,res2.z);
    glVertex3d(res3.x,res3.y,res3.z);
    glVertex3d(res4.x,res4.y,res4.z);
    glEnd();

//    glBegin(GL_QUADS);
//    glVertex3d(rect[0].x,rect[0].y,0);
//    glVertex3d(rect[1].x,rect[1].y,0);
//    glVertex3d(rect[2].x,rect[2].y,0);
//    glVertex3d(rect[3].x,rect[3].y,0);
//    glEnd();

//    MeshManager::MeshInfo::Coord3D res1 = state->GetMeshManager()->GetCoord3D((hugin_utils::FDiff2D&)vertices[0]);
//    MeshManager::MeshInfo::Coord3D res2 = state->GetMeshManager()->GetCoord3D((hugin_utils::FDiff2D&)vertices[1]);
//    glVertex3d(res1.x,res1.y,res1.z);
//    glVertex3d(res2.x,res2.y,res2.z);
}

double GreatCircleArc::getxscale(void) const
{
	return m_xscale;
}


void GreatCircleArc::LineSegment::doGLcross(int point, double xscale, VisualizationState * state) const
{
    //TODO: cross with polygons instead of lines
    double vx, vy;

	vx = vertices[point].x;
	vy = vertices[point].y;

    hugin_utils::FDiff2D p1(vx - xscale, vy - xscale);
    hugin_utils::FDiff2D p2(vx + xscale, vy + xscale);

    hugin_utils::FDiff2D p3(vx - xscale, vy + xscale);
    hugin_utils::FDiff2D p4(vx + xscale, vy - xscale);

    MeshManager::MeshInfo::Coord3D res1 = state->GetMeshManager()->GetCoord3D(p1);
    MeshManager::MeshInfo::Coord3D res2 = state->GetMeshManager()->GetCoord3D(p2);

    MeshManager::MeshInfo::Coord3D res3 = state->GetMeshManager()->GetCoord3D(p3);
    MeshManager::MeshInfo::Coord3D res4 = state->GetMeshManager()->GetCoord3D(p4);

    glBegin(GL_LINES);

        // main diagonal
        glVertex3f(res1.x,res1.y,res1.z);
        glVertex3f(res2.x,res2.y,res2.z);
        // second diagonal
        glVertex3f(res3.x,res3.y,res3.z);
        glVertex3f(res4.x,res4.y,res4.z);

    glEnd();
}


float GreatCircleArc::squareDistance(hugin_utils::FDiff2D point) const
{
    // find the minimal distance.
    float distance = std::numeric_limits<float>::max();
    for (std::vector<GreatCircleArc::LineSegment>::const_iterator it = m_lines.begin();
         it != m_lines.end();
         it++)
    {
        float this_distance = it->squareDistance(point);
        if (this_distance < distance)
        {
            distance = this_distance;
        }
    }
    return distance;
}

float GreatCircleArc::LineSegment::squareDistance(hugin_utils::FDiff2D point) const
{
    // minimal distance between a point and a line segment

    // Does the line segment start and end in the same place?
    if (vertices[0]  == vertices[1])    

    {
        // yes, so return the distance to the point where the 'line' segment is.
        return point.squareDistance(vertices[0]);
    }
    // the vector along the line segment.
    hugin_utils::FDiff2D line_vector = vertices[1] - vertices[0];
    // the parameter indicating the point's nearest position along the line.
    float t = line_vector.x == 0 ?
                    (point.y - vertices[0].y) / line_vector.y
                  : (point.x - vertices[0].x) / line_vector.x;
    // check if the point lies along the line segment.
    if (t <= 0.0)
    {
        // no, nearest vertices[0].
        return vertices[0].squareDistance(point);
    }
    if (t >= 1.0)
    {
        // no, nearest vertices[1].
        return vertices[1].squareDistance(point);
    }
    
    // Find the intersection with the line segment and the tangent line.
    hugin_utils::FDiff2D intersection(vertices[0] + line_vector * t);
    // Finally, find the distance between the intersection and the point.
    return intersection.squareDistance(point);
}


