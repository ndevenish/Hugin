// -*- c-basic-offset: 4 -*-
/** @file MainFrame.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
 *
 *  This is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _MAINFRAME_H
#define _MAINFRAME_H


#include "wx/frame.h"

#include "PT/Panorama.h"
using namespace PT;

// forward declarations, to save the #include statements
class CPEditorPanel;

/** The main window frame.
 *
 *  It contains the menu & statusbar and a tab widget for many
 *  ideas. It also holds the Panorama model.
 *
 *  it therefor also hold operations that determine the lifecycle
 *  of the panorama object (new, open, save, quit).
 */
class MainFrame : public wxFrame
{
public:

    /** ctor.
     */
    MainFrame(wxWindow* parent=(wxWindow *)NULL);

    /** dtor.
     */
    virtual ~MainFrame();

private:

    // event handlers
    void OnExit(wxCommandEvent & e);
    void OnAbout(wxCommandEvent & e);
    void OnSaveProject(wxCommandEvent & e);
    void OnLoadProject(wxCommandEvent & e);
    void OnNewProject(wxCommandEvent & e);
    void OnAddImages(wxCommandEvent & e);
    void OnRemoveImages(wxCommandEvent & e);
    void OnTextEdit(wxCommandEvent & e);

    CPEditorPanel * cpe;

    // the model
    Panorama pano;

    DECLARE_EVENT_TABLE()
};



#endif // _MAINFRAME_H
