// -*- c-basic-offset: 4 -*-

/** @file huginApp.cpp
 *
 *  @brief implementation of huginApp Class
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

#include <wx/wxprec.h>
#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// For all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers)
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "wx/image.h"               // wxImage
#include "wx/xrc/xmlres.h"          // XRC XML resouces

#include "hugin/MainFrame.h"

#include "hugin/huginApp.h"

// make wxwindows use this class as the main application
IMPLEMENT_APP(huginApp)

huginApp::huginApp()
{
}

huginApp::~huginApp()
{
}


bool huginApp::OnInit()
{

    // initialize i18n
    locale.Init(wxLANGUAGE_DEFAULT);

    // add local Path
    locale.AddCatalogLookupPathPrefix("../po");

    // set the name of locale recource to look for
    locale.AddCatalog(wxT("hugin"));

    // initialize image handlers
    wxInitAllImageHandlers();

    // Initialize all the XRC handlers.
    wxXmlResource::Get()->InitAllHandlers();

    // load all XRC files.
#ifdef _INCLUDE_UI_RESOURCES
    InitXmlResource();
#else
    wxXmlResource::Get()->Load(wxT("xrc/main_frame.xrc"));
//    wxXmlResource::Get()->Load(wxT("xrc/main_menubar.xrc"));
    wxXmlResource::Get()->Load(wxT("xrc/cp_editor_panel.xrc"));
    wxXmlResource::Get()->Load(wxT("xrc/main_menu.xrc"));
    wxXmlResource::Get()->Load(wxT("xrc/main_tool.xrc"));
    wxXmlResource::Get()->Load(wxT("xrc/edit_text.xrc"));
    wxXmlResource::Get()->Load(wxT("xrc/about.xrc"));
#endif

    // create main frame
    MainFrame *frame = new MainFrame();

    // show the frame.
    frame->Show(TRUE);

    return true;
}
