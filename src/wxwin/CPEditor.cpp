// -*- c-basic-offset: 4 -*-

/** @file CPEditor.cpp
 *
 *  @brief implementation of CPEditor Class
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

#include "CPEditor.h"

#include "../utils.h"

IMPLEMENT_DYNAMIC_CLASS(CPEditor, wxPanel);

BEGIN_EVENT_TABLE( CPEditor, wxWindow )
    EVT_CPEVENT( CPEditor::OnCPEvent )
    EVT_SIZE(CPEditor::OnSize)
END_EVENT_TABLE()

CPEditor::CPEditor(wxWindow* parent, wxWindowID id,
                   const wxPoint& pos,
                   const wxSize& size,
                   long style,
                   const wxString& name)
    : wxPanel(parent, id, pos, size, style, name)
{

    wxBoxSizer *cpeSizer = new wxBoxSizer(wxHORIZONTAL);

    m_leftImg = new CPImageCtrl(this,-1);
    m_rightImg = new CPImageCtrl(this,-1);

    std::vector<wxPoint> points;
    for(int i = 0; i < 20; i++) {
        points.push_back(wxPoint( (int) (800* (rand()/(RAND_MAX+1.0))),
                                  (int) (600* (rand()/(RAND_MAX+1.0)))
            ));
    }
    m_leftImg->setCtrlPoints(points);
    m_rightImg->setCtrlPoints(points);

    cpeSizer->Add(m_leftImg,
               1,
               wxALL | wxEXPAND,
               3);
    cpeSizer->Add(m_rightImg,
               1,
               wxALL | wxEXPAND,
               3);
    cpeSizer->SetSizeHints(this);
    SetSizer(cpeSizer);
}

/*
wxSize CPEditor::DoGetBestSize() const
{
    DEBUG_TRACE("DoGetBestSize");
    return wxSize(800,600);
//    return cpeSizer->Fit(this);
}
*/

void CPEditor::OnCPEvent( ptCPEvent&  ev)
{
    wxString text;
    switch (ev.getMode()) {
    case ptCPEvent::NONE:
        text = "NONE";
        break;
    case ptCPEvent::NEW_POINT_CHANGED:
        text = "NEW_POINT_CHANGED";
        break;
    case ptCPEvent::POINT_SELECTED:
        text << "POINT_SELECTED: " << ev.getPointNr();
        break;
    case ptCPEvent::POINT_CHANGED:
        text << "POINT_CHANGED: " << ev.getPointNr();
        break;
    case ptCPEvent::REGION_SELECTED:
        text = "REGION_SELECTED";
        break;
//    default:
//        text = "FATAL: unknown event mode";
    }
    
    wxMessageBox( "CPEditor recieved custom event:" + text,
                  "CPEditor::OnCPEvent",
                  wxOK | wxICON_INFORMATION,
                  this );
}

void CPEditor::OnSize(wxSizeEvent& event)
{
    DEBUG_TRACE("OnSize");
    Layout();
    int vw,vh,w,h;
    m_rightImg->GetVirtualSize(&vw,&vh);
    m_rightImg->GetSize(&w,&h);
    DEBUG_DEBUG("size: " << w << "," << h
                << " virtual size: " << vw << "," << vh);
    m_rightImg->GetVirtualSize(&vw,&vh);
    m_rightImg->GetSize(&w,&h);
    DEBUG_DEBUG("imgscrollwin size: " << w << "," << h
                << " virtual size: " << vw << "," << vh);

    event.Skip();
}



void CPEditor::setLeftImage(wxImage & img)
{
    m_leftImg->setImage(img);
}


void CPEditor::setRightImage(wxImage & img)
{
    m_rightImg->setImage(img);
}

