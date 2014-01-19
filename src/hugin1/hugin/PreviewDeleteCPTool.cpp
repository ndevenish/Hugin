// -*- c-basic-offset: 4 -*-

/** @file PreviewDeleteCPTool.cpp
 *
 *  @author T. Modes
 *
 *  @brief implementation of ToolHelper for deleting control points in the pano space
 *
 */

/*
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

#include "hugin/PreviewDeleteCPTool.h"
#include "panoinc_WX.h"
#include "panoinc.h"
#include "hugin/CommandHistory.h"
#include <map>

#include <wx/platform.h>
#ifdef __WXMAC__
#include <OpenGL/gl.h>
#else
#ifdef __WXMSW__
#include <vigra/windows.h>
#endif
#include <GL/gl.h>
#endif

// we want to draw over the panorama when in use, so make sure we get notice.
void PreviewDeleteCPTool::Activate()
{
    helper->NotifyMe(PreviewToolHelper::MOUSE_PRESS, this);
    helper->NotifyMe(PreviewToolHelper::MOUSE_MOVE, this);
    helper->NotifyMe(PreviewToolHelper::REALLY_DRAW_OVER_IMAGES, this);
    m_mouse_down = false;
    helper->SetStatusMessage(_("Drag a rectangle to select which control points should be deleted."));
};

// The panorama has been drawn the selection rectangle.
void PreviewDeleteCPTool::ReallyAfterDrawImagesEvent()
{
    if (m_mouse_down)
    {
        glDisable(GL_TEXTURE_2D);
        glColor3f(1, 0, 0);
        glBegin(GL_LINES);
        glVertex2f(m_start_pos.x, m_start_pos.y);
        glVertex2f(m_current_pos.x, m_start_pos.y);
        glVertex2f(m_current_pos.x, m_start_pos.y);
        glVertex2f(m_current_pos.x, m_current_pos.y);
        glVertex2f(m_current_pos.x, m_current_pos.y);
        glVertex2f(m_start_pos.x, m_current_pos.y);
        glVertex2f(m_start_pos.x, m_current_pos.y);
        glVertex2f(m_start_pos.x, m_start_pos.y);
        glEnd();
        glEnable(GL_TEXTURE_2D);
    };
};

void PreviewDeleteCPTool::MouseMoveEvent(double x, double y, wxMouseEvent & e)
{
    if (m_mouse_down)
    {
        m_current_pos = helper->GetMousePanoPosition();
        // force redrawing
        helper->GetVisualizationStatePtr()->ForceRequireRedraw();
        helper->GetVisualizationStatePtr()->Redraw();
    };
}


void PreviewDeleteCPTool::MouseButtonEvent(wxMouseEvent &e)
{
    if (e.GetButton() == wxMOUSE_BTN_LEFT)
    {
        if (e.ButtonDown())
        {
            m_mouse_down = true;
            m_start_pos = helper->GetMousePanoPosition();
        }
        else
        {
            m_mouse_down = false;
            m_current_pos = helper->GetMousePanoPosition();
            if (m_current_pos.squareDistance(m_start_pos) > 5)
            {
                DeleteCP(m_start_pos, m_current_pos);
            }
            else
            {
                wxBell();
            };
        };
    };
};

void PreviewDeleteCPTool::DeleteCP(const hugin_utils::FDiff2D& pos1, const hugin_utils::FDiff2D& pos2)
{
    PT::Panorama* pano = helper->GetPanoramaPtr();
    HuginBase::UIntSet activeImages = pano->getActiveImages();
    const hugin_utils::FDiff2D panoPos1(pos1.x < pos2.x ? pos1.x : pos2.x, pos1.y < pos2.y ? pos1.y : pos2.y);
    const hugin_utils::FDiff2D panoPos2(pos1.x > pos2.x ? pos1.x : pos2.x, pos1.y > pos2.y ? pos1.y : pos2.y);
    HuginBase::UIntSet cpsToDelete;
    if (!activeImages.empty())
    {
        // create transformation objects
        typedef std::map<size_t, HuginBase::PTools::Transform*> TransformMap;
        TransformMap transformations;
        for (HuginBase::UIntSet::iterator it = activeImages.begin(); it != activeImages.end(); ++it)
        {
            HuginBase::PTools::Transform* trans = new HuginBase::PTools::Transform();
            trans->createInvTransform(pano->getImage(*it), pano->getOptions());
            transformations.insert(std::make_pair(*it, trans));
        };
        HuginBase::CPVector cps = pano->getCtrlPoints();
        for (HuginBase::CPVector::const_iterator cpIt = cps.begin(); cpIt != cps.end(); ++cpIt)
        {
            // check cp only if both images are visible
            if (set_contains(activeImages, cpIt->image1Nr) && set_contains(activeImages, cpIt->image2Nr))
            {
                hugin_utils::FDiff2D pos1;
                hugin_utils::FDiff2D pos2;
                // remove all control points, where both points are inside the given rectangle
                if (transformations[cpIt->image1Nr]->transformImgCoord(pos1, hugin_utils::FDiff2D(cpIt->x1, cpIt->y1)) &&
                    transformations[cpIt->image2Nr]->transformImgCoord(pos2, hugin_utils::FDiff2D(cpIt->x2, cpIt->y2)))
                {
                    if (panoPos1.x <= pos1.x && pos1.x <= panoPos2.x && panoPos1.y <= pos1.y && pos1.y <= panoPos2.y &&
                        panoPos1.x <= pos2.x && pos2.x <= panoPos2.x && panoPos1.y <= pos2.y && pos2.y <= panoPos2.y)
                    {
                        cpsToDelete.insert(cpIt - cps.begin());
                    };
                };
            };
        };
        for (TransformMap::iterator it = transformations.begin(); it != transformations.end(); ++it)
        {
            delete it->second;
        };
    };
    if (cpsToDelete.empty())
    {
        wxMessageBox(_("Selected rectangle does not contain any control points.\nNo control point removed."),
#ifdef _WINDOWS
            _("Hugin"),
#else
            wxT(""),
#endif
            wxOK | wxICON_INFORMATION, (wxWindow*)helper->GetPreviewFrame());
    }
    else
    {
        int r = wxMessageBox(wxString::Format(_("Really delete %lu control points?"), static_cast<unsigned long int>(cpsToDelete.size())),
#ifdef _WINDOWS
            _("Hugin"),
#else
            wxT(""),
#endif
            wxICON_INFORMATION | wxYES_NO, (wxWindow*)helper->GetPreviewFrame());
        if (r == wxYES)
        {
            GlobalCmdHist::getInstance().addCommand(new PT::RemoveCtrlPointsCmd(*pano, cpsToDelete));
        };
    };
    // force redrawing
    helper->GetVisualizationStatePtr()->ForceRequireRedraw();
    helper->GetVisualizationStatePtr()->Redraw();
};