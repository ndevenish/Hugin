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


#include <vector>
#include <set>
#include "wx/frame.h"
#include "wx/dnd.h"
#include "wx/listctrl.h"

#include "PT/Panorama.h"
#include "hugin/OptimizeFrame.h"
using namespace PT;

// forward declarations, to save the #include statements
class CPEditorPanel;
class LensPanel;
class ImgPreview;
class ImagesPanel;
class PanoPanel;
class PreviewFrame;
class CPListFrame;
//class OptimizeFrame;


/** The main window frame.
 *
 *  It contains the menu & statusbar and a big notebook with
 *  the different tabs. It also holds the Panorama model.
 *
 *  it therefor also hold operations that determine the lifecycle
 *  of the panorama object (new, open, save, quit).
 */
class MainFrame : public wxFrame, public PT::PanoramaObserver, public wxFileDropTarget
{
public:

    /** ctor.
     */
    MainFrame(wxWindow* parent=(wxWindow *)NULL);

    /** dtor.
     */
    virtual ~MainFrame();

    /** this is called whenever the panorama has changed.
     *
     *  This function must now update all the gui representations
     *  of the panorama to display the new state.
     *
     *  Functions that change the panororama must not update
     *  the GUI directly. The GUI should always be updated
     *  to reflect the current panorama state in this function.
     *
     *  This avoids unnessecary close coupling between the
     *  controller and the view (even if they sometimes
     *  are in the same object). See model view controller
     *  pattern.
     *
     */
// virtual void panoramaChanged(PT::Panorama &pano);
    void panoramaImagesChanged(PT::Panorama &pano, const PT::UIntSet & imgNr);

    /** file drag and drop handler method */
    bool OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames);

    // called when a control point in CPListFrame is selected
    void ShowCtrlPoint(unsigned int cpNr);

private:

    // event handlers
    void OnExit(wxCommandEvent & e);
    void OnAbout(wxCommandEvent & e);
    void OnUndo(wxCommandEvent & e);
    void OnRedo(wxCommandEvent & e);
    void OnSaveProject(wxCommandEvent & e);
    void OnLoadProject(wxCommandEvent & e);
    void OnNewProject(wxCommandEvent & e);
    void OnAddImages(wxCommandEvent & e);
    void OnRemoveImages(wxCommandEvent & e);
    void OnTextEdit(wxCommandEvent & e);
    void OnToggleOptimizeFrame(wxCommandEvent & e);
    void OnTogglePreviewFrame(wxCommandEvent & e);
    void OnToggleCPFrame(wxCommandEvent & e);
    void UpdatePanels(wxCommandEvent & e);
    void Resize(wxSizeEvent & e);

    // tab panels
    ImagesPanel* images_panel;
    LensPanel* lens_panel;
    CPEditorPanel * cpe;
    PanoPanel * pano_panel;

    // flying windows
    OptimizeFrame * opt_frame;
    PreviewFrame * preview_frame;
    CPListFrame * cp_frame;

    // the model
    Panorama pano;

    // Image Preview
    ImgPreview *canvas;

    // self
    MainFrame* GetFrame(void);

    DECLARE_EVENT_TABLE()
};


#endif // _MAINFRAME_H
