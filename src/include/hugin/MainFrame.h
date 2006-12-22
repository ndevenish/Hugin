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

#include "hugin/OptimizePanel.h"
#include "hugin/PreferencesDialog.h"

using namespace PT;

// forward declarations, to save the #include statements
class AssistantPanel;
class CPEditorPanel;
class LensPanel;
class ImgPreview;
class ImagesPanel;
class CropPanel;
class PanoPanel;
class PreviewFrame;
class CPListFrame;
//class OptimizeFrame;


/** simple class that forward the drop to the mainframe */
class PanoDropTarget : public wxFileDropTarget
{
public:
    PanoDropTarget(PT::Panorama & p, bool imageOnly = false)
        : pano(p)
    { m_imageOnly = imageOnly; }

    bool OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames);

private:
    PT::Panorama & pano;
    bool m_imageOnly;
};


/** The main window frame.
 *
 *  It contains the menu & statusbar and a big notebook with
 *  the different tabs. It also holds the Panorama model.
 *
 *  it therefor also hold operations that determine the lifecycle
 *  of the panorama object (new, open, save, quit).
 */
class MainFrame : public wxFrame, public PT::PanoramaObserver, public utils::MultiProgressDisplay
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

    /// get the path to the xrc directory
    const wxString & GetXRCPath();

    /// hack.. kind of a pseudo singleton...
    static MainFrame * Get();

    // Used to display tip of the day after main interface is initialised and visible
	void OnTipOfDay(wxCommandEvent & e);

    // load a project
    void LoadProjectFile(const wxString & filename);
	
    // Restore the layout
    void RestoreLayout();
    
    /// hack to restore the layout on next resize
    void RestoreLayoutOnNextResize();
    

#ifdef __WXMAC__
    void MacOnOpenFile(const wxString & filename);
#endif
    bool CloseProject(bool cnacelable);

    // TODO: create a nice generic optimisation & stitching function
    // instead of these gateway functions to the optimizer and pano panels.
    void OnOptimize(wxCommandEvent & e);
    void OnDoStitch(wxCommandEvent & e);
    void OnTogglePreviewFrame(wxCommandEvent & e);
    void OnAddImages(wxCommandEvent & e);

    void ShowCtrlPointEditor(unsigned int img1, unsigned int img2);

protected:
    // called when a progress message should be displayed
    /** receive notification about progress. Should not be called directly.
     *
     *  @param msg message text
     *  @param progress optional progress indicator (0-100)
     */
    void updateProgressDisplay();

private:

    // event handlers
    void OnExit(wxCloseEvent & e);
    void OnUserQuit(wxCommandEvent & e);
    void OnAbout(wxCommandEvent & e);
    void OnHelp(wxCommandEvent & e);
    void OnKeyboardHelp(wxCommandEvent & e);
    void OnFAQ(wxCommandEvent & e);
    void OnShowPrefs(wxCommandEvent &e);
    void OnUndo(wxCommandEvent & e);
    void OnRedo(wxCommandEvent & e);
    void OnSaveProject(wxCommandEvent & e);
    void OnSaveProjectAs(wxCommandEvent & e);
    void OnSavePTStitcherAs(wxCommandEvent & e);
    void OnLoadProject(wxCommandEvent & e);
    void OnNewProject(wxCommandEvent & e);
    void OnAddTimeImages(wxCommandEvent & e);
    void OnTextEdit(wxCommandEvent & e);
    void OnFineTuneAll(wxCommandEvent & e);
//    void OnToggleOptimizeFrame(wxCommandEvent & e);
    void OnToggleCPFrame(wxCommandEvent & e);
    void UpdatePanels(wxCommandEvent & e);
    void OnSize(wxSizeEvent &e);
    void enableTools(bool option);

    void DisplayHelp(wxString section);

    wxNotebook * m_notebook;
    // tab panels
    AssistantPanel* assistant_panel;
    ImagesPanel* images_panel;
    LensPanel* lens_panel;
    CropPanel* crop_panel;
    CPEditorPanel * cpe;
    OptimizePanel * opt_panel;
    PanoPanel * pano_panel;

    // flying windows
    PreviewFrame * preview_frame;
    CPListFrame * cp_frame;

    // Image Preview
    ImgPreview *canvas;

    // Preferences
    PreferencesDialog * pref_dlg;

    // the model
    PT::Panorama & pano;

    // filename of the current project
    wxString m_filename;


    bool m_doRestoreLayout;

    // self
    static MainFrame* m_this;

    // the help browser
    wxHtmlHelpController * m_help;

    DECLARE_EVENT_TABLE()
};


#endif // _MAINFRAME_H
