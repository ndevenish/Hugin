// -*- c-basic-offset: 4 -*-

/** @file CPEditorPanel.cpp
 *
 *  @brief implementation of CPEditorPanel Class
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
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

//-----------------------------------------------------------------------------
// Standard wxWindows headers
//-----------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// For all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers)
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "wx/xrc/xmlres.h"              // XRC XML resouces
#include "wx/notebook.h"
#include "common/utils.h"
#include "hugin/CPImageCtrl.h"
//#include "hugin/TabBar.h"
#include "hugin/CPEditorPanel.h"


using namespace std;

BEGIN_EVENT_TABLE(CPEditorPanel, wxPanel)
    EVT_BUTTON( XRCID("button_wide"), CPEditorPanel::OnMyButtonClicked )
    EVT_CPEVENT(CPEditorPanel::OnCPEvent)
    EVT_NOTEBOOK_PAGE_CHANGED(XRCID("cp_editor_left_tabbar"),CPEditorPanel::OnLeftImgChange)
    EVT_NOTEBOOK_PAGE_CHANGED(XRCID("cp_editor_right_tabbar"),CPEditorPanel::OnRightImgChange)
END_EVENT_TABLE()

CPEditorPanel::CPEditorPanel(wxWindow * parent)
{
    wxXmlResource::Get()->LoadPanel(this, parent, wxT("cp_editor_panel"));
    DEBUG_TRACE("Panel created");

    // create custom controls

    // left image
    m_leftImg = new CPImageCtrl(this);
    wxXmlResource::Get()->AttachUnknownControl(wxT("cp_editor_left_img"),
                                               m_leftImg);
//    TabBar * tb = new TabBar(this);
//    m_leftTabs = new TabBarView(tb);
//    tb->SetTabView(m_leftTabs);
//    wxXmlResource::Get()->AttachUnknownControl(wxT("cp_editor_left_tabs"),
//                                               tb);

    // right image
    m_rightImg = new CPImageCtrl(this);
    wxXmlResource::Get()->AttachUnknownControl(wxT("cp_editor_right_img"),
                                               m_rightImg);
//    tb = new TabBar(this);
//    m_rightTabs = new TabBarView(tb);
//    tb->SetTabView(m_rightTabs);
//    wxXmlResource::Get()->AttachUnknownControl(wxT("cp_editor_right_tabs"),
//                                               tb);
}


CPEditorPanel::~CPEditorPanel()
{
}


void CPEditorPanel::setLeftImage(wxImage & img)
{
    m_leftImg->setImage(img);
}


void CPEditorPanel::setRightImage(wxImage & img)
{
    m_rightImg->setImage(img);
}


void CPEditorPanel::OnCPEvent( CPEvent&  ev)
{
    wxString text;
    switch (ev.getMode()) {
    case CPEvent::NONE:
        text = "NONE";
        break;
    case CPEvent::NEW_POINT_CHANGED:
        text = "NEW_POINT_CHANGED";
        break;
    case CPEvent::POINT_SELECTED:
        text << "POINT_SELECTED: " << ev.getPointNr();
        break;
    case CPEvent::POINT_CHANGED:
        text << "POINT_CHANGED: " << ev.getPointNr();
        break;
    case CPEvent::REGION_SELECTED:
        text = "REGION_SELECTED";
        break;
//    default:
//        text = "FATAL: unknown event mode";
    }

    wxMessageBox( "CPEditorPanel recieved CPEvent event:" + text
                  << " from:" << (unsigned long int) ev.GetEventObject(),
                  "CPEditorPanel::OnCPEvent",
                  wxOK | wxICON_INFORMATION,
                  this );
}

void CPEditorPanel::OnMyButtonClicked(wxCommandEvent &e)
{
    DEBUG_DEBUG("on my button");
}

void CPEditorPanel::panoramaChanged(PT::Panorama &pano)
{
    DEBUG_TRACE("panoramChanged()");

    // update Tabs
    unsigned int nrImages = pano.getNrOfImages();
    unsigned int nrTabs;
    // = m_leftTabs->GetNumberOfTabs();
    // update tab buttons
/*    
    if (nrTabs < nrImages) {
        // FIXME add buttons
        for (unsigned int img = nrTabs; img <nrImages; ++img) {
            m_leftTabs->AddTab(img, wxString::Format("%d",img));
            m_rightTabs->AddTab(img, wxString::Format("%d",img));
        }
    } else if (nrTabs > nrImages) {
        // FIXME delete buttons
        for (unsigned int img = nrImages; img > nrTabs; img--) {
            m_leftTabs->RemoveTab(img);
            m_rightTabs->RemoveTab(img);
        }
    }
*/

    // update control points
    const PT::CPVector & controlPoints = pano.getCtrlPoints();
    currentPoints.clear();
    mirroredPoints.clear();
    std::vector<wxPoint> left;
    std::vector<wxPoint> right;

    // create a list of all control points
    unsigned int i = 0;
    for (PT::CPVector::const_iterator it = controlPoints.begin(); it != controlPoints.end(); ++it) {
        PT::ControlPoint point = *it;
        if ((point.image1Nr == firstImage) && (point.image2Nr == secondImage)){
            left.push_back(wxPoint( (int) point.x1, (int) point.y1));
            right.push_back(wxPoint( (int) point.x2, (int) point.y2));
            currentPoints.push_back(make_pair(it - controlPoints.begin(), *it));
            i++;
        } else if ((point.image2Nr == firstImage) && (point.image1Nr == secondImage)){
            point.mirror();
            mirroredPoints.insert(i);
            left.push_back(wxPoint( (int) point.x1, (int) point.y1));
            right.push_back(wxPoint( (int) point.x2, (int) point.y2));
            currentPoints.push_back(std::make_pair(it - controlPoints.begin(), point));
            i++;
        }
    }
    m_leftImg->setCtrlPoints(left);
    m_rightImg->setCtrlPoints(right);
}


void CPEditorPanel::OnLeftImgChange(wxNotebookEvent & e)
{
    DEBUG_TRACE("OnLeftImgChange() to " << e.GetSelection());
}

void CPEditorPanel::OnRightImgChange(wxNotebookEvent & e)
{
    DEBUG_TRACE("OnRightImgChange() to " << e.GetSelection());
}
