// -*- c-basic-offset: 4 -*-

/** @file ImageOrientationFrame.cpp
 *
 *  @brief implementation of ImageOrientationFrame Class
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

#include <algorithm>
#include <utility>
#include <functional>

#include "panoinc.h"

#include "hugin/MainFrame.h"
#include "hugin/CommandHistory.h"
#include "hugin/ImageOrientationFrame.h"
#include "hugin/ImageOrientationPanel.h"

using namespace PT;
using namespace std;

BEGIN_EVENT_TABLE(ImageOrientationFrame, wxFrame)
END_EVENT_TABLE()


ImageOrientationFrame::ImageOrientationFrame(wxWindow * parent, Panorama & pano)
    : m_pano(pano)
{
    DEBUG_TRACE("");
    bool ok = wxXmlResource::Get()->LoadFrame(this, parent, wxT("anchor_orientation_panel"));
    DEBUG_ASSERT(ok);

    CreateStatusBar(1);

    // attach our image Panel
    m_orientationPanel = new ImageOrientationPanel(this, &pano);
    wxXmlResource::Get()->AttachUnknownControl (
        wxT("anchor_orientation_panel_unknown"),
        m_orientationPanel );

#if __WXMSW__
    wxIcon myIcon(MainFrame::Get()->GetXRCPath() + wxT("data/icon.ico"),wxBITMAP_TYPE_ICO);
#else
    wxIcon myIcon(MainFrame::Get()->GetXRCPath() + wxT("data/icon.png"),wxBITMAP_TYPE_PNG);
#endif
    SetIcon(myIcon);

    wxConfigBase * config = wxConfigBase::Get();

    long w = config->Read(wxT("/ImageOrientationFrame/width"),-1);
    long h = config->Read(wxT("/ImageOrientationFrame/height"),-1);
    if (w != -1) {
        SetClientSize(w,h);
    }



    // observe the panorama
    //m_pano.addObserver(this);
    DEBUG_TRACE("ctor end");
}

ImageOrientationFrame::~ImageOrientationFrame()
{
    DEBUG_TRACE("dtor");
    wxSize sz = GetClientSize();
    wxConfigBase * config = wxConfigBase::Get();
    config->Write(wxT("/ImageOrientationFrame/width"),sz.GetWidth());
    config->Write(wxT("/ImageOrientationFrame/height"),sz.GetHeight());
    DEBUG_TRACE("dtor end");
}

void ImageOrientationFrame::OnClose(wxCloseEvent& event)
{
    DEBUG_DEBUG("OnClose");
    /*
    // do not close, just hide if we're not forced
    if (event.CanVeto()) {
        event.Veto();
        Hide();
        DEBUG_DEBUG("hiding");
    } else {
        DEBUG_DEBUG("closing");
        Destroy();
    }
    */
}


