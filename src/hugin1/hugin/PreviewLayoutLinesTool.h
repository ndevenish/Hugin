// -*- c-basic-offset: 4 -*-
/** @file PreviewLayoutLinesTool.h
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

#ifndef PREVIEW_LAYOUT_LINES_TOOL_H
#define PREVIEW_LAYOUT_LINES_TOOL_H

#include "Tool.h"
#include <vector>
#include <hugin_math/hugin_math.h>
#include "GreatCircles.h"

class GLPreviewFrame;

/** The PreviewLayoutLinesTool handles the lines connecting images in the layout
 * view of the fast preview.
 * 
 * These are its functions:
 * -# Draw the coloured lines indicating control point error behind the images.
 * -# Draw the grey lines for images overlapping that share no control points.
 * -# Interprate clicks on lines and load up the relevant pair in the control
 *    point tab.
 * .
 * 
 * @todo Update line information when control point information changes, and
 * only when it changes. The main preview system doesn't redraw when control
 * points change. This should save on the draw time, since we don't always
 * need to examine the control points to draw the preview.
 */
class PreviewLayoutLinesTool : public Tool, public HuginBase::PanoramaObserver
{
public:
    explicit PreviewLayoutLinesTool(ToolHelper *helper);
    ~PreviewLayoutLinesTool();
    
    /** This just sets a flag when the panorama is changed.
     * When we next redraw, we recalculate the statistics for the lines if the
     * flag is set.
     */
    void panoramaChanged(HuginBase::Panorama &pano);
    void panoramaImagesChanged(HuginBase::Panorama&, const HuginBase::UIntSet&);
    
    /// Start using the PreviewLayoutLinesTool.
    void Activate();
    
    /** Revaluate the lines under the mouse pointer when it moves.
     */
    virtual void MouseMoveEvent(double x, double y, wxMouseEvent & e);
    
    /** Capture clicks on lines, and load up the relavent images in the control
     * point tab.
     */
    void MouseButtonEvent(wxMouseEvent & e);

    /** Draw all the lines between images that indicate the quality and quantity
     * of control points.
     */
    void BeforeDrawImagesEvent();
    
    bool BeforeDrawImageEvent(unsigned int image);
    
    /** Draw a border over the images when a line is hilighted, similar to the
     * identify tool.
     */
    void AfterDrawImagesEvent();
private:

    //user has clicked and is holding left button while near a line
    bool m_holdOnNear;

    /// Flag set to true to update statistics next redraw
    bool m_updateStatistics;
    /// OpenGL texture names for the border highlight.
    unsigned int m_rectangleBorderTex;
    
    /// The location on the panorama that the middle of the images map to.
    std::vector<hugin_utils::FDiff2D> m_imageCentres;
    
    /// The spherical coordinates of the middle of the images.
    std::vector<hugin_utils::FDiff2D> m_imageCentresSpherical;
    
    /// The transformations used to make the image centres.
    std::vector<HuginBase::PTools::Transform *> m_transforms;
    
    
    /** A class to store information about each line that will be drawn.
     */
    class LineDetails
    {
    public:
        /// index of images for this line
        unsigned int image1, image2;
        LineDetails();
        /// the number of control points between these images
        unsigned int numberOfControlPoints;
        /// the error of the control point with the greatest error between these images
        double worstError;
        /// the total of all the control point errors between these images
        double totalError;
        /** false if the line should be used, true if it shouldn't.
         * A dud image is not drawn, and should never be the nearest line to the
         * mouse.
         */
        bool dud;
        
        GreatCircleArc arc;
        
        /** Draw a line in the preview for this pair of images.
         * 
         * If the line is a dud, no line is drawn.
         * 
         * - It is coloured grey and 1 pixel thick if there are no control points.
         * - The width of the line is max {1 px + NumberOfControlPoints / 5, 5}.
         * - The colour of a line with control points is dependant on the maximum
         * error and selection:
         *   - Red if the error is greater than or equal to 20.
         *   - Red - Orange - Yellow if the error is between 5 and 20
         *   - Yellow - Green if the error is less than 5.
         *   - All lines are drawn white instead if it is the selected line.
         * @param highlight Prelight for nearest line. Draws the line white when
         * true, otherwise use a colour based on error.
         */
        void draw(bool highlight);
        
        /** Get the square of the distance from the arc to some panorama coordinate.
         * The point is normally the mouse location in the panorama.
         */
        float getDistance(hugin_utils::FDiff2D point);
    };
    
    /// A container for the line information.
    std::vector<LineDetails> m_lines;
    
    /// The index of nearest line to the mouse position
    unsigned int m_nearestLine;
    
    /// True if we should highlight the nearest line to the mouse position.
    bool m_useNearestLine;
    
    /// Update the line information
    void updateLineInformation();
    
    /// Update the locations of the image centres.
    void updateImageCentres();
    
    /// Draw a border for an image.
    void drawIdentificationBorder(unsigned int image);
};

#endif

