// -*- c-basic-offset: 4 -*-

/** @file TestPanel.cpp
 *
 *  @brief implementation of TestPanel Class
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

#include "TestPanel.h"


IMPLEMENT_DYNAMIC_CLASS(TestPanel, wxPanel);

BEGIN_EVENT_TABLE(TestPanel, wxPanel)
  EVT_SIZE(TestPanel::OnSize)
END_EVENT_TABLE()

TestPanel::TestPanel(wxWindow* parent, wxWindowID id)
  : wxPanel(parent,id)
{

    m_sizer = new wxBoxSizer(wxHORIZONTAL);

  // add some widgets.
    m_sizer->Add(
        new wxTextCtrl( this, -1, _T("some text 1") ),
        1,    // vertically strechable
        wxALL | wxEXPAND,
        7 );
    m_sizer->Add(
        new wxButton( this, -1, _T("some text 2") ),
        1,                 // make vertically stretchable
        wxALL | wxEXPAND,  // make border all around
        7 );               // set border width to 7

    m_sizer->SetSizeHints(this);
    SetSizer(m_sizer);

}

TestPanel::~TestPanel()
{
}

void TestPanel::OnSize(wxSizeEvent& event)
{
    Layout();
    event.Skip();
}

