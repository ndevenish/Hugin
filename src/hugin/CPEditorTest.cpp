// -*- c-basic-offset: 4 -*-

/** @file
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

// standard hugin include
#include "panoinc.h"
#include "panoinc_WX.h"

// hugin's
#include "hugin/CPEditorPanel.h"
#include "hugin/CPEditorTest.h"
#include "hugin/ImgScrollWindow.h"

IMPLEMENT_APP(PickerApp)

PickerApp::PickerApp()
{
}

PickerApp::~PickerApp()
{
}

bool PickerApp::OnInit()
{
    // initialize image handlers
    wxInitAllImageHandlers();

    // Initialize all the XRC handlers.
    wxXmlResource::Get()->InitAllHandlers();

    // load all XRC files.
    wxXmlResource::Get()->Load(wxT("xrc/cp_editor_panel.xrc"));

    wxFrame *frame = new wxFrame((wxFrame*) NULL, -1, "Hello World");
    frame->CreateStatusBar();
    frame->SetStatusText("CPEditorTest, test of CPEditor/CPImageCtrl windows (everybody else would call it widgets...)");

    wxBoxSizer *topsizer = new wxBoxSizer(wxVERTICAL);

//    wxPanel *cpe = new wxPanel();
//    wxXmlResource::Get()->LoadPanel(cpe, frame, wxT("cp_editor_panel"));;

    CPEditorPanel * cpe = new CPEditorPanel(frame);
//    wxXmlResource::Get()->LoadPanel(cpe, frame, wxT("cp_editor_panel"));
//    cpe->Create(frame);

//    cpe->SetSize(200,300);

    wxImage img;
    if (!img.LoadFile("test.jpg")) {
        DEBUG_FATAL("could not load testimage: test.jpg");
    }
    DEBUG_DEBUG("loaded image w:" << img.GetWidth()
                << "h: " << img.GetHeight());
    cpe->setLeftImage(img);
    cpe->setRightImage(img);

    topsizer->Add(cpe,
                  1,
                  wxEXPAND,
                  5);
    topsizer->SetSizeHints(frame);
    frame->SetSizer(topsizer);

    frame->Show(TRUE);
    SetTopWindow(frame);

    return true;
}
