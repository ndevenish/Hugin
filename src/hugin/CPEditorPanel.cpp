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
#include "common/utils.h"
#include "hugin/CPImageCtrl.h"

#include "hugin/CPEditorPanel.h"


BEGIN_EVENT_TABLE(CPEditorPanel, wxPanel)
    EVT_BUTTON( XRCID("button_wide"), CPEditorPanel::OnMyButtonClicked )
    EVT_CPEVENT(CPEditorPanel::OnCPEvent)

END_EVENT_TABLE()

CPEditorPanel::CPEditorPanel(wxWindow * parent)
{
    wxXmlResource::Get()->LoadPanel(this, parent, wxT("cp_editor_panel"));
    DEBUG_TRACE("Panel created");

    m_leftImg = new CPImageCtrl(this);
    wxXmlResource::Get()->AttachUnknownControl(wxT("cp_editor_left_img"),
                                               m_leftImg);
    m_rightImg = new CPImageCtrl(this);
    wxXmlResource::Get()->AttachUnknownControl(wxT("cp_editor_right_img"),
                                               m_rightImg);

/*
  wxButton *m_leftImg = new wxButton(this,12345, "left Image");
  wxXmlResource::Get()->AttachUnknownControl(wxT("cp_editor_left_img"),
    m_leftImg);
  wxButton *m_rightImg = new wxButton(this, -1, "right Image");
  wxXmlResource::Get()->AttachUnknownControl(wxT("cp_editor_right_img"),
    m_rightImg);

*/
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
