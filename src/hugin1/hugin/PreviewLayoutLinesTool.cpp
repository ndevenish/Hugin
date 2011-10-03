// -*- c-basic-offset: 4 -*-
/** @file PreviewLayoutLinesTool.cpp
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
#include "GreatCircles.h"

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

PreviewLayoutLinesTool::PreviewLayoutLinesTool(ToolHelper *helper)
    : Tool(helper),
      m_updateStatistics(true),
      m_nearestLine(-1),
      m_useNearestLine(false),
      m_holdOnNear(false)
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

    if (!(helper->IsMouseOverPano())) {
        if (m_useNearestLine) {
            m_useNearestLine = false;
            helper->GetVisualizationStatePtr()->ForceRequireRedraw();
            helper->GetVisualizationStatePtr()->Redraw();
        }
        return;
    }

    if (e.Dragging() && !m_holdOnNear) {
        return;
    }
    
    // Check each line in turn.
    double minDistance = DBL_MAX;
    unsigned int nearestLineOld = m_nearestLine;
    for (unsigned int i = 0; i < m_lines.size(); i++)
    {
        if (m_lines[i].dud) continue;
        double lineDistance = m_lines[i].getDistance(helper->GetMousePanoPosition());
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
    // Coordinates are panorama pixels squared, so we'll need to scale it.
    double scale = helper->GetVisualizationStatePtr()->GetScale();
    scale *= scale;
    m_useNearestLine = minDistance < 4900.0 / scale;
    
    if (oldUseNearestLine != m_useNearestLine || m_nearestLine != nearestLineOld)
    {
        LineDetails & line = m_lines[m_nearestLine];
        // get notification of when the connected images are drawn so we can
        // draw them on top with a highlight.
        helper->NotifyMeBeforeDrawing(line.image1, this);
        helper->NotifyMeBeforeDrawing(line.image2, this);
        
        // Redraw with new indicators. Since the indicators aren't part of the
        // panorama, we have to persuade the viewstate that a redraw is required.
        helper->GetVisualizationStatePtr()->ForceRequireRedraw();
        helper->GetVisualizationStatePtr()->Redraw();
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
    helper->GetViewStatePtr()->GetTextureManager()->DisableTexture();//    for (unsigned int i = 0; i < m_lines.size(); i++)
    for (unsigned int i = 0; i < m_lines.size(); i++)
    {
        m_lines[i].draw(m_useNearestLine && i == m_nearestLine);
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
                        helper->GetVisualizationStatePtr()->GetMeshDisplayList(image1));
    helper->GetViewStatePtr()->GetTextureManager()->DrawImage(image2,
                        helper->GetVisualizationStatePtr()->GetMeshDisplayList(image2));
                        
    // Setup OpenGL blending state for identification borders.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // use the border texture.
    helper->GetViewStatePtr()->GetTextureManager()->DisableTexture(true);
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
    glMatrixMode(GL_MODELVIEW);
    glCallList(helper->GetVisualizationStatePtr()->GetMeshDisplayList(image));
    glMatrixMode(GL_TEXTURE);
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
    if ( e.LeftDown() && !m_lines.empty() && m_useNearestLine)
    {
        m_holdOnNear = true;
    } 

    if (m_holdOnNear && e.LeftUp() && m_useNearestLine) {
        m_holdOnNear = false;
        LineDetails & line = m_lines[m_nearestLine];
        MainFrame::Get()->ShowCtrlPointEditor(line.image1, line.image2);
        MainFrame::Get()->Raise();
    }

    if (m_useNearestLine && e.LeftUp()) {
        m_useNearestLine = false;
    }
}

void PreviewLayoutLinesTool::updateLineInformation()
{
    m_lines.clear();
    const PT::Panorama & pano = *(helper->GetPanoramaPtr());
    unsigned int numberOfImages = pano.getNrOfImages();
    HuginBase::UIntSet active_images = pano.getActiveImages();
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
            /// test if the line should be visible.
            if (!(set_contains(active_images, i) &&
                  set_contains(active_images, j)))
            {
                // At least one of the images is hidden, so don't show the line.
                line.dud = true;
            }
            else if (line.numberOfControlPoints > 0)
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
            
            if (!line.dud)
            {
                line.arc = GreatCircleArc(m_imageCentresSpherical[i].x,
                                          m_imageCentresSpherical[i].y,
                                          m_imageCentresSpherical[j].x,
                                          m_imageCentresSpherical[j].y,
                                          *(helper->GetVisualizationStatePtr()));
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
    m_imageCentresSpherical.resize(m_imageCentres.size());
    
    HuginBase::PanoramaOptions spherical_pano_opts;
    spherical_pano_opts.setProjection(HuginBase::PanoramaOptions::EQUIRECTANGULAR);
    spherical_pano_opts.setWidth(360);
    spherical_pano_opts.setHeight(180);
    spherical_pano_opts.setHFOV(360);
    
    for (unsigned int image_number = 0;
         image_number < helper->GetPanoramaPtr()->getNrOfImages();
         image_number++)
    {
        // transforming image coordinates to panorama coordinates.
        m_transforms[image_number]->createInvTransform  (
            *(helper->GetViewStatePtr()->GetSrcImage(image_number)),
            *(helper->GetViewStatePtr()->GetOptions())
                                                        );
        HuginBase::PTools::Transform to_spherical;
        to_spherical.createInvTransform (
            *(helper->GetViewStatePtr()->GetSrcImage(image_number)),
            spherical_pano_opts
                                         );
        const vigra::Size2D & s = helper->GetViewStatePtr()->GetSrcImage(image_number)->getSize();
        // find where the middle of the image maps to.
        m_transforms[image_number]->transformImgCoord   (
                                    m_imageCentres[image_number].x,
                                    m_imageCentres[image_number].y,
                                    double(s.x) / 2.0, double(s.y) / 2.0
                                                        );
        to_spherical.transformImgCoord(
            m_imageCentresSpherical[image_number].x,
            m_imageCentresSpherical[image_number].y,
            double(s.x) / 2.0, double(s.y) / 2.0
                                      );
    }
}

void PreviewLayoutLinesTool::LineDetails::draw(bool highlight)
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
    }else {
        double red, green, blue;
        hugin_utils::ControlPointErrorColour(worstError,red,green,blue);
        glColor3d(red, green, blue);
    }
    
    double lineWidth = numberOfControlPoints / 5.0 + 1.0;
    if (lineWidth > 5.0) lineWidth = 5.0;
//    glLineWidth(lineWidth);
    
    arc.draw(false, lineWidth);
}

PreviewLayoutLinesTool::LineDetails::LineDetails()
    :   numberOfControlPoints(0),
        worstError(0),
        totalError(0)
{
    
}

float PreviewLayoutLinesTool::LineDetails::getDistance(hugin_utils::FDiff2D point)
{
    if (dud)
    {
        // This isn't a real line, so return the maximum distance to it possible.
        return FLT_MAX;
    }
    else
    {
        return  arc.squareDistance(point);
    }
}

