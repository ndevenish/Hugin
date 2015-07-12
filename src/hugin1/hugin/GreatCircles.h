// -*- c-basic-offset: 4 -*-
/** @file GreatCircles.h
 *
 *  @author James Legg
 *
 *  @brief Declare GreatCircles class
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
 */

#ifndef GREATCIRCLES_H
#define GREATCIRCLES_H

#include "ViewState.h"
#include <vector>
#include <hugin_math/hugin_math.h>

/** Draw great circle arcs in the fast preview.
 *
 *  A great circle is the spherical equivalent of a straight line, but is often
 *  drawn curved, depending on the start and end positions and the projection.
 */
class GreatCircles
{
public:
    /** constructor */
    GreatCircles();
    /** Set the ViewState to use for information on output projection and
     * preview display.
     * @param viewState a pointer to the ViewState the preview is using.
     */
    void setVisualizationState(VisualizationState * visualizationState);
    /** Draw the shortest segment of the great circle crossing two spherical
     * coordinates.
     *
     * Actually, the line is approximated since OpenGL doesn't do curves.
     *
     * Preconditions: The OpenGL state must be ready to draw lines the required
     * width and colour, and outside of a glBegin/glEnd pair. setViewStatePtr
     * must have been called with a valid (and still valid9 ViewState pointer to
     * the ViewState used by the preview.
     * 
     * @param startLat lattiude of the first point in degrees.
     * @param startLong longitude of the first point in degrees.
     * @param endLat lattide of the second point in degrees.
     * @param endLong longitude of the second point in degrees.
     * @param width width of the line in pixels.
     */
    void drawLineFromSpherical(double startLat, double startLong,
                               double endLat, double endLong, double width = 1.0);
private:
    VisualizationState * m_visualizationState;
};

class GreatCircleArc
{
    public:
        /** Create a bad great circle arc.
         * draw() won't do anything and squareDistance() will return the
         * maximum float.
         */
        GreatCircleArc();
        /** Create a great circle arc.
         * @param startLat lattiude of the first point in degrees.
         * @param startLong longitude of the first point in degrees.
         * @param endLat lattide of the second point in degrees.
         * @param endLong longitude of the second point in degrees.
         */
        GreatCircleArc(double startLat, double startLong,
                       double endLat, double endLong,
                       VisualizationState & m_visualizationState);
        /// Draw the great circle arc on the fast preview
        void draw(bool withCross=true, double width = 1.0) const;
        /** Return the square of the minimal distance between the great circle arc and a coorinate on the panorama.
         * This is an approximation, but should be reasonable.
         */
        float squareDistance(hugin_utils::FDiff2D point) const;
		double m_xscale ;
		double getxscale() const;

        struct LineSegment
        {
            hugin_utils::FDiff2D vertices[2];
            /// Get the square of the minimal distance to a point.
            float squareDistance(hugin_utils::FDiff2D point) const;
            /// Specify the line to OpenGL. Must be within a glBegin/glEnd pair.
			void doGLcross(int point, double cscale, VisualizationState *state) const;
            /**
             * Draw a meshed line
             * @param width the width of the line
             * @param state The visualization state needed to obtain final 3D coordinates and the scale for the visualization
             * @param preceding the line segment before this line segment, needed to calculate the right begining slope
             * @param proceeding the line segment after this line segment, needed to calculate the right ending slope
             */
            void doGL(double width, VisualizationState *state, LineSegment * preceding = NULL, LineSegment * proceeding = NULL) const;
        };

    protected:
        std::vector<LineSegment> m_lines;
        VisualizationState * m_visualizationState;
		
};

#endif

