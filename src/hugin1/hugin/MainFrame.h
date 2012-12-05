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
#include <map>

#include "PT/Panorama.h"

#include "wx/docview.h"
#ifdef __WXMSW__
#include "wx/msw/helpchm.h"
#endif

#include <appbase/ProgressDisplayOld.h>
#include <appbase/ProgressReporterOld.h>

#include "hugin/OptimizePanel.h"
#include "hugin/PreferencesDialog.h"

// Celeste header
#include "Celeste.h"
#include "GuiLevel.h"

using namespace PT;

// forward declarations, to save the #include statements
class CPEditorPanel;
class ImgPreview;
class ImagesPanel;
class MaskEditorPanel;
class OptimizePhotometricPanel;
class PanoPanel;
class PreviewFrame;
class GLPreviewFrame;
class CPListFrame;


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
class MainFrame : public wxFrame, public PT::PanoramaObserver, public AppBase::MultiProgressDisplay, public AppBase::ProgressReporter
{
public:

    /** ctor.
     */
    MainFrame(wxWindow* parent, PT::Panorama & pano);

    /** dtor.
     */
    virtual ~MainFrame();

    /** Enable or disable undo and redo.
     *  They should be enabled only when there is a command to act upon.
     */
    virtual void panoramaChanged(PT::Panorama &pano);
    void panoramaImagesChanged(PT::Panorama &pano, const PT::UIntSet & imgNr);
    /** returns panorama object */
    const PT::Panorama & getPanorama() { return pano; };

    // called when a control point in CPListFrame is selected
    void ShowCtrlPoint(unsigned int cpNr);

    /// get the path to the xrc directory
    const wxString & GetXRCPath();
    /** get the path to data directory */
    const wxString & GetDataPath();

    /// hack.. kind of a pseudo singleton...
    static MainFrame * Get();

    // Used to display tip of the day after main interface is initialised and visible
	void OnTipOfDay(wxCommandEvent & e);

    // load a project
    void LoadProjectFile(const wxString & filename);
    void RunAssistant(wxWindow* mainWin);

    /** disables all OpenGL related menu items and toobar buttons */
    void DisableOpenGLTools();
#ifdef __WXMAC__
    void MacOnOpenFile(const wxString & filename);
#endif
    bool CloseProject(bool cancelable);

    // TODO: create a nice generic optimisation & stitching function
    // instead of these gateway functions to the optimizer and pano panels.
    void OnOptimize(wxCommandEvent & e);
    void OnPhotometricOptimize(wxCommandEvent & e);
    void OnDoStitch(wxCommandEvent & e);
    void OnTogglePreviewFrame(wxCommandEvent & e);
    void OnToggleGLPreviewFrame(wxCommandEvent & e);
    void OnAddImages(wxCommandEvent & e);
    void OnSaveProject(wxCommandEvent & e);
    void OnSetGuiSimple(wxCommandEvent & e);
    void OnSetGuiAdvanced(wxCommandEvent & e);
    void OnSetGuiExpert(wxCommandEvent & e);

    /** call help browser with given file */
    void DisplayHelp(wxString section);

    /** opens the control points tab with the both images selected */
    void ShowCtrlPointEditor(unsigned int img1, unsigned int img2);
    /** opens the mask/crop editor with the given image selected */
    void ShowMaskEditor(size_t imgNr);
    /** opens the stitcher tab */
    void ShowStitcherTab();

    void resetProgress(double max);
    bool increaseProgress(double delta);
    bool increaseProgress(double delta, const std::string & msg);
    void setMessage(const std::string & msg);

    void OnCPListFrameClosed();
    /** returns default cp detector setting */
    CPDetectorSetting& GetDefaultSetting();

    wxString getProjectName();

    struct celeste::svm_model* GetSVMModel();
    
    GLPreviewFrame * getGLPreview();
#ifdef __WXMSW__
    wxCHMHelpController& GetHelpController() { return m_msHtmlHelp; }
#endif
    void SetGuiLevel(GuiLevel newLevel);
    const GuiLevel GetGuiLevel() const { return m_guiLevel; };

    wxFileHistory* GetFileHistory() { return &m_mruFiles; };

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
#ifdef HUGIN_HSI
    void OnPythonScript(wxCommandEvent & e);
    void OnPlugin(wxCommandEvent& e);
#endif
    void OnUndo(wxCommandEvent & e);
    void OnRedo(wxCommandEvent & e);
    void OnSaveProjectAs(wxCommandEvent & e);
    void OnSavePTStitcherAs(wxCommandEvent & e);
    void OnLoadProject(wxCommandEvent & e);
    void OnNewProject(wxCommandEvent & e);
    void OnAddTimeImages(wxCommandEvent & e);
    void OnRunAssistant(wxCommandEvent & e);
    void OnFineTuneAll(wxCommandEvent & e);
    void OnRemoveCPinMasks(wxCommandEvent & e);
    void OnMergeProject(wxCommandEvent & e);
    void OnApplyTemplate(wxCommandEvent & e);
    void OnSendToBatch(wxCommandEvent & e);
    void OnSendToAssistantQueue(wxCommandEvent & e);
    void OnOpenPTBatcher(wxCommandEvent & e);
//    void OnToggleOptimizeFrame(wxCommandEvent & e);
    void OnShowCPFrame(wxCommandEvent & e);
    /** event handler for recently used files */
    void OnMRUFiles(wxCommandEvent &e);
    /** event handler for full screen */
    void OnFullScreen(wxCommandEvent &e);
    void UpdatePanels(wxCommandEvent & e);
    void OnSize(wxSizeEvent &e);
    void enableTools(bool option);
    /** adds the given files to the projects, with checking for invalid filenames */
    void AddImages(wxArrayString& filenameArray);

    void OnShowDonate(wxCommandEvent &e);
    void OnShowPanel(wxCommandEvent &e);

    // update progress display
    bool displayProgress();

    wxFileHistory m_mruFiles;
    wxNotebook * m_notebook;
    // tab panels
    ImagesPanel* images_panel;
    MaskEditorPanel* mask_panel;
    CPEditorPanel * cpe;
    OptimizePanel * opt_panel;
    bool m_show_opt_panel;
    OptimizePhotometricPanel * opt_photo_panel;
    bool m_show_opt_photo_panel;
    PanoPanel * pano_panel;
    struct celeste::svm_model* svmModel;

    // flying windows
    PreviewFrame * preview_frame;
    GLPreviewFrame * gl_preview_frame;
    CPListFrame * cp_frame;
    GuiLevel m_guiLevel;

    // Image Preview
    ImgPreview *canvas;

    // Preferences
    //PreferencesDialog * pref_dlg;

    // the model
    PT::Panorama & pano;

    // filename of the current project
    wxString m_filename;

    // self
    static MainFrame* m_this;
    // file menu
    wxMenu* m_menu_file_simple;
    wxMenu* m_menu_file_advanced;

    // progress reporter
    double m_progressMax;
    double m_progress;
    wxString m_progressMsg;
#ifdef __WXMSW__
    wxCHMHelpController     m_msHtmlHelp;
#endif

#ifdef HUGIN_HSI
    // list associating the wxID in the menu with a python script
    std::map<int, wxFileName> m_plugins;
#endif

    DECLARE_EVENT_TABLE()
};

bool getLensDataFromUser(wxWindow * parent, SrcPanoImage & srcImg,
                         double & focalLength, double & cropFactor);

#endif // _MAINFRAME_H
