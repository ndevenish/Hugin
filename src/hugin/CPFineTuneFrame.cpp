// -*- c-basic-offset: 4 -*-

/** @file CPFineTuneFrame.cpp
 *
 *  @brief implementation of CPFineTuneFrame Class
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


#include <config.h>
#include "panoinc_WX.h"
#include "panoinc.h"

#include "hugin/CPFineTuneFrame.h"

BEGIN_EVENT_TABLE(CPFineTuneFrame, wxFrame)
END_EVENT_TABLE()

CPFineTuneFrame::CPFineTuneFrame(wxWindow * parent, PT::Panorama & pano)
    : wxFrame(parent, -1, _("Control point finetune"))
{
    SetAutoLayout(TRUE);
    wxBoxSizer *topsizer = new wxBoxSizer( wxHORIZONTAL );

    m_leftImg = new CPZoomDisplayPanel(this, pano);
    m_rightImg = new CPZoomDisplayPanel(this, pano);

    topsizer->Add(m_leftImg,
                  1,                // horizontally stretchable
                  wxEXPAND | wxALL, // vertically stretchable + border
                  5);               // 5 pixels border

    topsizer->Add(new wxStaticLine(this, -1, wxDefaultPosition,
                                   wxDefaultSize, wxLI_VERTICAL),
                  0,                // not horizontally stretchable
                  wxEXPAND,         // vertically stretchable
                  0);               // 0 pixels border

    topsizer->Add(m_rightImg,
                  1,                // horizontally stretchable
                  wxEXPAND | wxALL, // vertically stretchable + border
                  5);               // 5 pixels border
    this->SetSizer(topsizer);
    topsizer->SetSizeHints( this );

    wxConfigBase * config = wxConfigBase::Get();
    long w = config->Read("/CPFineTuneFrame/width",-1);
    long h = config->Read("/CPFineTuneFrame/height",-1);
    if (w != -1) {
        SetClientSize(w,h);
    }
}

CPFineTuneFrame::~CPFineTuneFrame()
{
    wxSize sz = GetClientSize();
    wxConfigBase * config = wxConfigBase::Get();
    config->Write("/CPFineTuneFrame/width",sz.GetWidth());
    config->Write("/CPFineTuneFrame/height",sz.GetHeight());

}
