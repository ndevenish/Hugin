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

#include "common/utils.h"
#include "PT/Panorama.h"

#include "wx/frame.h"
#include "wx/dnd.h"
#include "wx/listctrl.h"
#include "hugin/OptimizePanel.h"

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


/** simple class that forward the drop to the mainframe */
class PanoDropTarget : public wxFileDropTarget
{
public:
    PanoDropTarget(PT::Panorama & p)
        : pano(p)
        { }
    bool OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames);

private:
    PT::Panorama & pano;
};


/** The main window frame.
 *
 *  It contains the menu & statusbar and a big notebook with
 *  the different tabs. It also holds the Panorama model.
 *
 *  it therefor also hold operations that determine the lifecycle
 *  of the panorama object (new, open, save, quit).
 */
class MainFrame : public wxFrame, public PT::PanoramaObserver, public utils::ProgressDisplay
{
public:

    /** ctor.
     */
    MainFrame(wxWindow* parent, PT::Panorama & pano);

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


    // called when a control point in CPListFrame is selected
    void ShowCtrlPoint(unsigned int cpNr);

    // called when a progress message should be displayed
    /** receive notification about progress
     *
     *  @param msg message text
     *  @param progress optional progress indicator (0-100)
     */
    virtual void progressMessage(const std::string & msg,
                                 int progress=-1);

    /// get the path to the xrc directory
    const wxString & GetXRCPath()
    { return m_xrcPrefix;};

    /// hack.. kind of a pseudo singleton...
    static MainFrame * Get();

private:

    // event handlers
    void OnExit(wxCloseEvent & e);
    void OnUserQuit(wxCommandEvent & e);
    void OnAbout(wxCommandEvent & e);
    void OnHelp(wxCommandEvent & e);
    void OnUndo(wxCommandEvent & e);
    void OnRedo(wxCommandEvent & e);
    void OnSaveProject(wxCommandEvent & e);
    void OnSaveProjectAs(wxCommandEvent & e);
    void OnLoadProject(wxCommandEvent & e);
    void OnNewProject(wxCommandEvent & e);
    void OnAddImages(wxCommandEvent & e);
    void OnAddTimeImages(wxCommandEvent & e);
    void OnRemoveImages(wxCommandEvent & e);
    void OnTextEdit(wxCommandEvent & e);
//    void OnToggleOptimizeFrame(wxCommandEvent & e);
    void OnTogglePreviewFrame(wxCommandEvent & e);
    void OnToggleCPFrame(wxCommandEvent & e);
    void OnOptimize(wxCommandEvent & e);
    void UpdatePanels(wxCommandEvent & e);
    void Resize(wxSizeEvent & e);

    wxNotebook * m_notebook;
    // tab panels
    ImagesPanel* images_panel;
    LensPanel* lens_panel;
    CPEditorPanel * cpe;
    OptimizePanel * opt_panel;
    PanoPanel * pano_panel;

    // flying windows
    PreviewFrame * preview_frame;
    CPListFrame * cp_frame;

    // Image Preview
    ImgPreview *canvas;

    // the model
    PT::Panorama & pano;

    // filename of the current project
    wxString m_filename;

    // data prefix
    wxString m_xrcPrefix;

    // self
    static MainFrame* m_this;

    DECLARE_EVENT_TABLE()
};


#endif // _MAINFRAME_H
