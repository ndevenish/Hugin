// -*- c-basic-offset: 4 -*-

/** @file MainFrame.cpp
 *
 *  @brief implementation of MainFrame Class
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

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <wx/xrc/xmlres.h>          // XRC XML resouces

#include "hugin/config.h"
#include "hugin/CPEditorPanel.h"
#include "hugin/MainFrame.h"

// event table. this frame will recieve mostly global commands.
BEGIN_EVENT_TABLE(MainFrame, wxFrame)
    EVT_MENU(XRCID("action_exit_hugin"),  MainFrame::OnExit)
    EVT_MENU(XRCID("action_show_about"),  MainFrame::OnAbout)
END_EVENT_TABLE()

MainFrame::MainFrame(wxWindow* parent)
{
    // load our children. some children might need special
    // initialization. this will be done later.
    wxXmlResource::Get()->LoadFrame(this, parent, wxT("main_frame"));

    // load our menu bar
    SetMenuBar(wxXmlResource::Get()->LoadMenuBar(this, wxT("main_menubar")));

    // create the custom widget referenced by the main_frame XRC
    cpe = new CPEditorPanel(this);
    wxXmlResource::Get()->AttachUnknownControl(wxT("cp_editor_panel_unknown"),
                                               cpe);


    // create a status bar
    // I hope that we can also add other widget (like a
    // progress dialog to the status bar
//    CreateStatusBar(1);
}

MainFrame::~MainFrame()
{
}


void MainFrame::OnExit(wxCommandEvent & e)
{
    // FIXME ask to save is panorama is true
    Close(TRUE);
}


void MainFrame::OnSave(wxCommandEvent & e)
{
    wxLogError("not implemented");
}


void MainFrame::OnLoad(wxCommandEvent & e)
{
    wxLogError("not implemented");
}

void MainFrame::OnNew(wxCommandEvent & e)
{
    wxLogError("not implemented");
}

void MainFrame::OnAbout(wxCommandEvent & e)
{
    wxString msg;
    msg << _("Hugin (version")
        << HUGIN_VERSION << _(", compiled on ") << __DATE__")\n"
        << _("A program to generate panoramic images.\n"
             "Uses Panorama Tools to do the actual work\n"
             "\n"
             "Licenced under the GPL\n"
             "Authors (in alphabetical order): \n"
             "Pablo d'Angelo\n"
             "Kai-Uwe Behrmann\n"
             "Juha Helminen\n\n"
             "Homepage: http://hugin.sourceforge.net");
    wxMessageBox(msg, _("About XML resources demo"),
                 wxOK | wxICON_INFORMATION, this);
}
