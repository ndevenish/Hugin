// -*- c-basic-offset: 4 -*-
/** @file PreviewILayoutLineTool.cpp
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

#include "panoinc_WX.h"
#include "panoinc.h"

#include "PreviewLayoutLinesTool.h"
#include <config.h>

#include "base_wx/platform.h"
#include "MainFrame.h"

#include <wx/platform.h>
#ifdef __WXMAC__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#include <cfloat>
#include <cmath>

// the size of the rectangular texture. Must be a power of two, and at least 8.
#define rect_ts 64

// The resolution to use to sample overlaps.
/* The time it takes to sample it is a multiple of this squared + a constant.
 * So keep it low to make the redraws faster, set it higher to make the grey
 * lines' appearence more accurate. Make sure to adjust MIN_SAMPLE_OVERLAPS
 * below to keep the proportion that need to overlap the same.
 */
#define SAMPLE_FREQUENCY 12

/* The amount of samples that are required to be within the other image for it
 * to count as a large enough overlap to warrent a grey line. It is affected by
 * the SAMPLE_FREQUENCY. An image with SAMPLE_FREQUENCY squared overlaps is 
 * entirely within the other image. Must be > 0.
 * 
 * 12*12 = 144 samples. 16 / 144 = 1/9, so a 9th of the image must overlap for
 * the grey line to appear. If just the corners overlap, the overlap must be a
 * third in both directions:
 * |||
 * |||
 * ||$SS
 *   SSS
 *   SSS
 */
#define MIN_SAMPLE_OVERLAPS 16

/// A map from image positions to panorama positions
class PosMap
{
public:
    hugin_utils::FDiff2D data[SAMPLE_FREQUENCY][SAMPLE_FREQUENCY];
    hugin_utils::FDiff2D * operator[](std::size_t index)
    {
        return data[index];
    }
};

PreviewLayoutLinesTool::PreviewLayoutLinesTool(PreviewToolHelper *helper)
    : PreviewTool(helper),
      m_updateStatistics(true),
      m_useNearestLine(false)
{
    helper->GetPanoramaPtr()->addObserver(this);
    // make the textures. We have a circle border and a square one.
    // the textures are white with a the alpha chanel forming a border.
    glGenTextures(1, (GLuint*) &m_rectangleBorderTex);
    // we only want to specify alpha, but using just alpha in opengl attaches 0
    // for the luminosity. I tried biasing the red green and blue values to get
    // them to 1.0, but it didn't work under OS X for some reason. Instead we
    // use a luminance alpha pair, and use 1.0 for luminance all the time.

    // In the rectangle texture, the middle is 1/8 opaque, the outer pixels
    // are completely transparent, and one pixel in from the edges is
    // a completly opaque line.
    unsigned char rect_tex_data[rect_ts][rect_ts][2];
    // make everything white
    for (unsigned int x = 0; x < rect_ts; x++)
    {
        for (unsigned int y = 0; y < rect_ts; y++)
        {
            rect_tex_data[x][y][0] = 255;
        }
    }
    // now set the middle of the mask semi transparent
    for (unsigned int x = 2; x < rect_ts - 2; x++)
    {
        for (unsigned int y = 2; y < rect_ts - 2; y++)
        {
            rect_tex_data[x][y][1] = 31;
        }
    }
    // make an opaque border
    for (unsigned int d = 1; d < rect_ts - 1; d++)
    {
        rect_tex_data[d][1][1] = 255;
        rect_tex_data[d][rect_ts - 2][1] = 255;
        rect_tex_data[1][d][1] = 255;
        rect_tex_data[rect_ts - 2][d][1] = 255;
    }
    // make a transparent border around that
    for (unsigned int d = 0; d < rect_ts; d++)
    {
        rect_tex_data[d][0][1] = 0;
        rect_tex_data[d][rect_ts - 1][1] = 0;
        rect_tex_data[0][d][1] = 0;
        rect_tex_data[rect_ts - 1][d][1] = 0;
    }
    glBindTexture(GL_TEXTURE_2D, m_rectangleBorderTex);
    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_LUMINANCE_ALPHA, rect_ts, rect_ts,
                      GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, rect_tex_data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR_MIPMAP_LINEAR);
    // clamp texture so it won't wrap over the border of the cropped region.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
}
PreviewLayoutLinesTool::~PreviewLayoutLinesTool()
{
    // free the textures
    glDeleteTextures(1, (GLuint*) &m_rectangleBorderTex);
    
    // delete all the transforms.
    while (!m_transforms.empty())
    {
        delete m_transforms.back();
        m_transforms.pop_back();
    }
    
    // stop observing the panorama
    helper->GetPanoramaPtr()->removeObserver(this);
}

void PreviewLayoutLinesTool::panoramaChanged(HuginBase::PanoramaData &pano)
{
    m_updateStatistics = true;
}
void PreviewLayoutLinesTool::panoramaImagesChanged(HuginBase::PanoramaData&,
                                                   const HuginBase::UIntSet&)
{
    m_updateStatistics = true;
}

void PreviewLayoutLinesTool::Activate()
{
    // register notifications
    helper->NotifyMe(PreviewToolHelper::MOUSE_MOVE, this);
    helper->NotifyMe(PreviewToolHelper::DRAW_UNDER_IMAGES, this);
    helper->NotifyMe(PreviewToolHelper::DRAW_OVER_IMAGES, this);
    helper->NotifyMe(PreviewToolHelper::MOUSE_PRESS, this);
    
    helper->SetStatusMessage(_("Click a connection to edit control points."));
}

void PreviewLayoutLinesTool::MouseMoveEvent(double x, double y, wxMouseEvent & e)
{
    // Try to find the nearest line to the mouse pointer.
    // ...Unless there are no lines.
    if (m_lines.empty())
    {
        return;
    }
    // Check each line in turn.
    double minDistance = DBL_MAX;
    unsigned int nearestLineOld = m_nearestLine;
    for (unsigned int i = 0; i < m_lines.size(); i++)
    {
        if (m_lines[i].dud) continue;
        double lineDistance = m_lines[i].getDistance(m_imageCentres, x, y);
        if (lineDistance < minDistance)
        {
            // found a new minimum.
            minDistance = lineDistance;
            m_nearestLine = i;
        }
    }
    
    // Work out if it is close enough to highlight it.
    bool oldUseNearestLine = m_useNearestLine;
    // The limit is 70 pixels from the line.
    // Coordinates are panorama pixels, so we'll need to scale it.
    m_useNearestLine = minDistance < 70.0 / helper->GetViewStatePtr()->GetScale();
    
    if (oldUseNearestLine != m_useNearestLine || m_nearestLine != nearestLineOld)
    {
        LineDetails & line = m_lines[m_nearestLine];
        // get notification of when the connected images are drawn so we can
        // draw them on top with a highlight.
        helper->NotifyMeBeforeDrawing(line.image1, this);
        helper->NotifyMeBeforeDrawing(line.image2, this);
        
        // Redraw with new indicators. Since the indicators aren't part of the
        // panorama, we have to persuade the viewstate that a redraw is required.
        helper->GetViewStatePtr()->ForceRequireRedraw();
        helper->GetViewStatePtr()->Redraw();
    }
}

void PreviewLayoutLinesTool::BeforeDrawImagesEvent()
{
    // We should check if the lines or image centers have changed.
    if (m_updateStatistics)
    {
        m_updateStatistics = false;
        // Must be done in this order since the transforms updateImageCentres
        // uses are also used by updateLineInformation.
        updateImageCentres();
        updateLineInformation();
        
        if (m_nearestLine >= m_lines.size())
        {
            // The line we had selected no longer exists. Use the first line
            // until the mouse moves again.
            m_nearestLine = 0;
        }
    }
    
    // now draw each line.
    glEnable(GL_LINE_SMOOTH);
    glDisable(GL_TEXTURE_2D);
    for (unsigned int i = 0; i < m_lines.size(); i++)
    {
        m_lines[i].draw(m_imageCentres, m_useNearestLine && i == m_nearestLine);
    }
    glDisable(GL_LINE_SMOOTH);
    glEnable(GL_TEXTURE_2D);
    // reset some openGL state back to its normal value.
    glColor3ub(255, 255, 255);
    glLineWidth(1.0);
}

void PreviewLayoutLinesTool::AfterDrawImagesEvent()
{
    // we draw the partly transparent identification boxes over the top of the
    // two images connecting the nearest line to the mouse pointer.
    
    // If there are no lines, there isn't a nearest one. Also check if we use it
    if (m_lines.empty() || !m_useNearestLine) return;
    
    // draw the actual images
    unsigned int image1 = m_lines[m_nearestLine].image1;
    unsigned int image2 = m_lines[m_nearestLine].image2;
    helper->GetViewStatePtr()->GetTextureManager()->DrawImage(image1,
                        helper->GetViewStatePtr()->GetMeshDisplayList(image1));
    helper->GetViewStatePtr()->GetTextureManager()->DrawImage(image2,
                        helper->GetViewStatePtr()->GetMeshDisplayList(image2));
                        
    // Setup OpenGL blending state for identification borders.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // use the border texture.
    glBindTexture(GL_TEXTURE_2D, m_rectangleBorderTex);
    // We use the texture matrix to align the texture with the cropping region.
    glMatrixMode(GL_TEXTURE);
    
    // now draw the identification boxes
    drawIdentificationBorder(image1);
    drawIdentificationBorder(image2);
    
    // set OpenGL state back how we found it.
    glMatrixMode(GL_MODELVIEW);
    glDisable(GL_BLEND);
}

void PreviewLayoutLinesTool::drawIdentificationBorder(unsigned int image)
{
    // we want to shift the texture so it lines up with the cropped region.
    glPushMatrix();
    const HuginBase::SrcPanoImage & src = *(helper->GetViewStatePtr()->
                                                            GetSrcImage(image));
    int width = src.getSize().width(), height = src.getSize().height();
    vigra::Rect2D crop_region = src.getCropRect();
    // pick a texture depending on crop mode and move it to the cropped area
    switch (src.getCropMode())
    {
        case HuginBase::SrcPanoImage::CROP_CIRCLE:
        case HuginBase::SrcPanoImage::CROP_RECTANGLE:
            // get the biggest rectangle contained by both the image  and the
            // cropped area.
            crop_region &= vigra::Rect2D(src.getSize());
            glScalef((float) width / (float) crop_region.width(),
                     (float) height / (float) crop_region.height(),
                     1.0);
            glTranslatef(-(float) crop_region.left() / (float) width,
                         -(float) crop_region.top() / (float) height,
                         0.0);
            break;
        case HuginBase::SrcPanoImage::NO_CROP:
            break;
    }
    // draw the image with the border texture.
    glCallList(helper->GetViewStatePtr()->GetMeshDisplayList(image));
    // reset the texture matrix.
    glPopMatrix();
}

bool PreviewLayoutLinesTool::BeforeDrawImageEvent(unsigned int image)
{
    if (m_lines.empty() || !m_useNearestLine)
    {
        // there are no lines, so none should be highlighted, therefore every
        // image must be drawn. Don't notify us any longer and draw the image.
        helper->DoNotNotifyMeBeforeDrawing(image, this);
        return true;
    }
    // Delay drawing of images, so we can show them on top of the others.
    LineDetails & line = m_lines[m_nearestLine];
    if (image == line.image1 || image == line.image2) return false;
    // We must be done with this event, so don't notify us any longer:
    helper->DoNotNotifyMeBeforeDrawing(image, this);
    return true;
}

void PreviewLayoutLinesTool::MouseButtonEvent(wxMouseEvent & e)
{
    // If it was a left click and we have at least one line, bring up the images
    // in that line in the control point editor.
    if ( e.GetButton() == wxMOUSE_BTN_LEFT && !m_lines.empty())
    {
        LineDetails & line = m_lines[m_nearestLine];
        MainFrame::Get()->ShowCtrlPointEditor(line.image1, line.image2);
        MainFrame::Get()->Raise();
    }
}

void PreviewLayoutLinesTool::updateLineInformation()
{
    m_lines.clear();
    const PT::Panorama & pano = *(helper->GetPanoramaPtr());
    unsigned int numberOfImages = pano.getNrOfImages();
    // make a line for every image pair, but set the unneeded ones as dud.
    // This is for constant look up times when we scan control points.
    m_lines.resize(numberOfImages * numberOfImages);
    unsigned int numberOfControlPoints = pano.getNrOfCtrlPoints();
    // loop over all control points to count them and get error statistics.
    for (unsigned int cpi = 0 ; cpi < numberOfControlPoints ; cpi++)
    {
        const ControlPoint & cp = pano.getCtrlPoint(cpi);
        unsigned int low_index, high_index;
        if (cp.image1Nr < cp.image2Nr)
        {
            low_index = cp.image1Nr;
            high_index = cp.image2Nr;
        } else {
            low_index = cp.image2Nr;
            high_index = cp.image1Nr;
        }
        // find the line.
        // We use the formula in the line below to record image numbers to each
        // line later.
        LineDetails & line = m_lines[low_index * numberOfImages + high_index];
        // update control point count.
        line.numberOfControlPoints++;
        // update error statistics
        line.totalError += cp.error;
        if (cp.error > line.worstError)
        {
            line.worstError = cp.error;
        }
    }
    
    /* Find some locations of the images. We will test if they overlap using
     *  these locations. We don't need the last image as we can always take the
     * smallest numbered image as the source of points.
     */
    /** @todo Check both ways around.
     * This only checks if points from the smallest numbered image are
     * within the largest numbered image. If the first image is huge compared to
     * the second, then it many points will miss and we might not reach the
     * target even if the second image is contained within the first.
     */
    std::vector<PosMap>  positions(pano.getNrOfImages() - 1);
    for (unsigned int i = 0; i < numberOfImages - 1; i++)
    {
        const HuginBase::SrcPanoImage & img = pano.getImage(i);
        for (unsigned int x = 0; x < SAMPLE_FREQUENCY; x++)
        {
            for (unsigned int y = 0; y < SAMPLE_FREQUENCY; y++)
            {
                // scale (x, y) so it is always within the cropped region of the
                // image.
                vigra::Rect2D c = img.getCropRect();
                /** @todo Use only points inside the circle when circular crop
                 * is used.
                 */
                double xc = double (x) / double (SAMPLE_FREQUENCY)
                                        * double(c.width()) + c.left();
                double yc = double (y) / double (SAMPLE_FREQUENCY)
                                        * double(c.height()) + c.top();
                // now look up (xc, yc) in the image, find where in the panorama
                // it ends up.
                m_transforms[i]->transformImgCoord  (
                            positions[i][x][y].x, positions[i][x][y].y,
                            xc, yc                  );
            }
        }
    }
    
    // write other line data.
    for (unsigned int i = 0; i < numberOfImages; i++)
    {
        for (unsigned int j = 0; j < numberOfImages; j++)
        {
            LineDetails & line = m_lines[i * numberOfImages + j];
            line.image1 = i;
            line.image2 = j;
            /// test if images overlap.
            if (line.numberOfControlPoints > 0)
            {
                line.dud = false;
            } else if (i >= j) {
                // We only use lines where image1 is the lowest numbered image.
                // We don't bother with lines from one image to the same one.
                line.dud = true;
            } else {
                // test overlapping regions.
                HuginBase::PTools::Transform transform;
                ViewState & viewState = *helper->GetViewStatePtr();
                HuginBase::SrcPanoImage & src = *viewState.GetSrcImage(j);
                transform.createTransform(src, *(viewState.GetOptions()));
                unsigned int overlapingSamples = 0;
                for (unsigned int x = 0; x < SAMPLE_FREQUENCY; x++)
                {
                    for (unsigned int y = 0; y < SAMPLE_FREQUENCY; y++)
                    {
                        // check if mapping a point that was found earilier to
                        // be inside an image in panorama space is inside the
                        // other image when transformed from panorama to image.
                        double dx, dy;
                        transform.transformImgCoord (
                                        dx, dy,
                                        positions[i][x][y].x,
                                        positions[i][x][y].y
                                                    );
                        if (src.isInside(vigra::Point2D((int) dx, (int) dy)))
                        {
                            // they overlap
                            overlapingSamples++;
                        }
                    }
                }
                // If the overlap isn't big enough, the line isn't used.
                line.dud = (overlapingSamples < MIN_SAMPLE_OVERLAPS);
            }
        }
    }
}

void PreviewLayoutLinesTool::updateImageCentres()
{
    unsigned int numberOfImages = helper->GetPanoramaPtr()->getNrOfImages();
    // The transforms have no copy constructor, so we can't have a direct vector
    // and use resize. Instead we have a vector of pointers and we create and
    // delete transforms as they are needed.
    while (m_transforms.size() > numberOfImages)
    {
        delete m_transforms.back();
        m_transforms.pop_back();
    }
    m_transforms.reserve(numberOfImages);
    while (m_transforms.size() < numberOfImages)
    {
        m_transforms.insert(m_transforms.end(), new HuginBase::PTools::Transform);
    }
    m_imageCentres.resize(helper->GetPanoramaPtr()->getNrOfImages());
    for (unsigned int image_number = 0;
         image_number < helper->GetPanoramaPtr()->getNrOfImages();
         image_number++)
    {
        // transforming image coordinates to panorama coordinates.
        m_transforms[image_number]->createInvTransform  (
            *(helper->GetViewStatePtr()->GetSrcImage(image_number)),
            *(helper->GetViewStatePtr()->GetOptions())
                                                        );
        const vigra::Size2D & s = helper->GetViewStatePtr()->GetSrcImage(image_number)->getSize();
        // find where the middle of the image maps to.
        m_transforms[image_number]->transformImgCoord   (
                                    m_imageCentres[image_number].x,
                                    m_imageCentres[image_number].y,
                                    double(s.x) / 2.0, double(s.y) / 2.0
                                                        );
    }
}

void PreviewLayoutLinesTool::LineDetails::draw(
                        const std::vector<hugin_utils::FDiff2D>  & imageCentres,
                        bool highlight
                                              )
{
    if (dud)
    {
        // The line isn't real, don't draw it.
        return;
    }
    // work out the colour
    if (highlight)
    {
        glColor3ub(255, 255, 255);
    } else if (numberOfControlPoints == 0) {
        // no control points, use a grey line
        glColor3ub(170, 170, 170);
    }else if (worstError <= 5.0) {
        // low error.
        glColor3f(worstError / 5.0, 1.0, 0.0);
    } else if (worstError < 20.0) {
        // medium error
        glColor3f(1.0, 1.0 - ((worstError - 5.0) / 15.0), 0.0);
    } else {
        // big error
        glColor3ub(255, 0, 0);
    }
    double lineWidth = numberOfControlPoints / 5.0 + 1.0;
    if (lineWidth > 5.0) lineWidth = 5.0;
    /** @todo It is probably more apropriate to use thin rectangles than lines.
     * There are hardware defined limits on what width a line can be, and the
     * worst case is the hardware only alows lines 1 pixel thick.
     */
    /// @todo calculate how the line behaves on the great circle
    glLineWidth(lineWidth);
    glBegin(GL_LINES);
        glVertex2d(imageCentres[image1].x, imageCentres[image1].y);
        glVertex2d(imageCentres[image2].x, imageCentres[image2].y);
    glEnd();
}

PreviewLayoutLinesTool::LineDetails::LineDetails()
    :   numberOfControlPoints(0),
        worstError(0),
        totalError(0)
{
    
}

double PreviewLayoutLinesTool::LineDetails::getDistance(
                        const std::vector<hugin_utils::FDiff2D>  & imageCentres,
                        const double x,
                        const double y
                                                       )
{
    if (dud)
    {
        // This isn't a real line, so return the maximum distance to it possible.
        return DBL_MAX;
    }
    double x1 = imageCentres[image1].x;
    double x2 = imageCentres[image2].x;
    double y1 = imageCentres[image1].y;
    double y2 = imageCentres[image2].y;
    // find the shortest distance between point (x,y) and the line segment
    // between (x1, y1) and (y1, y2).
    // Does the line start and end in the same place?
    if (x1 == x2 && y1 == y2)
    {
        // yes, so return the distance to the point where the 'line' is.
        double dx = x1 - x,
               dy = y1 - y;
        return std::sqrt(dx * dx + dy * dy);
    }
    /* We are trying to find something like this:
     *      |<-----t------>|
     *                 (ix, iy)
     *      ________________________________________________
     *    ^               L|                             ^
     * (x1, y1)            |                         (x2, y2)
     *                     |
     *                     ^ (x, y)
     * 
     * The line segment we are finding the distance to is, parametrically:
     * (x1, x2) + t(x2 - x1, y2 - y1)                                        [1]
     * where t ranges from 0 to 1: t = 0 gives x1, t = 1 gives x2.
     * We want to find where we can place (ix, iy) so that the line going 
     * through (ix, iy) and (x, y) is perpendicular to the line segment.
     * The dot product of (x2 - x1, y2 -y1) and (ix - x, iy - y) should be 0 in
     * this case. We know (ix , iy) is on the line [1], so we can find a t
     * such that [1] = (ix, iy).
     * We have (x2 - x1, y2 -y1).((x1, y1) + t(x2 - x1, y2 - y1) - (x, y)) = _0_
     *                           ^-------------[1]-------------^
     * Let:
     */
    double ldx = x2 - x1,
           ldy = y2 - y1;
    /* then we have:
     * (ldx, ldy).((x1, y1) + t(ldx, ldy) - (x, y)) = _0_
     * => (ldx * (x1 + t * ldx - x), ldy * (y1 + t * ldy - y)) = _0_
     * Separate the components of the vector:
     * we have ldx * (x1 + t * ldx - x) = 0
     *      => (x1 - x) / ldx = - t               by division by ldx
     *      => t = (x - x1) / ldx
     * and also ldy * (y1 + t * ldy - y) = 0
     *      => (y1 - y) / ldy = - t               by division by ldy
     *      => t = (y - y1) / ldy
     * providing we can ldx is not 0 and ldy is not 0.
     * If they are are both 0, the line segment is actually a point. We just
     * checked for that. So ldx = ldy = 0 cannot happen. We use any non division
     * by 0 equation to get t.
     */
    double t = ldx == 0 ? (y - y1) / ldy : (x - x1) / ldx;    
    // check if the point lies along the line
    if (t <= 0.0)
    {
        // no, nearest x1.
        double dx = x1 - x,
               dy = y1 - y;
        return std::sqrt(dx * dx + dy * dy);
    }
    if (t >= 1.0)
    {
        // no, nearest x2.
        double dx = x2 - x,
               dy = y2 - y;
        return std::sqrt(dx * dx + dy * dy);
    }
    
    // The point lies somewhere along the line segment. Find the loaction of the
    // intersection with the line segment and the tangent line.
    double ix = x1 + t * ldx;
    double iy = y1 + t * ldy;
    
    // finally, find the distance between the intersection (ix, iy) and (x, y).
    double dx = ix - x,
           dy = iy - y;
    return std::sqrt(dx * dx + dy * dy);
}
