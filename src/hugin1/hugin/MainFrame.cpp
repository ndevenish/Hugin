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

#include <config.h>
#include <hugin_version.h>
#include <exiv2/exif.hpp>
#include <exiv2/image.hpp>

#include <wx/dir.h>
#include "panoinc_WX.h"
#include "panoinc.h"

#include "base_wx/platform.h"
#include "base_wx/wxPlatform.h"

#include "vigra/imageinfo.hxx"
#include "vigra_ext/Correlation.h"

#include "PT/utils.h"

#include "hugin/config_defaults.h"
#include "hugin/PreferencesDialog.h"
#include "hugin/MainFrame.h"
#include "hugin/wxPanoCommand.h"
#include "hugin/CommandHistory.h"
#include "hugin/PanoPanel.h"
#include "hugin/ImagesPanel.h"
#include "hugin/MaskEditorPanel.h"
#include "hugin/OptimizePanel.h"
#include "hugin/OptimizePhotometricPanel.h"
#include "hugin/PreviewFrame.h"
#include "hugin/GLPreviewFrame.h"
#include "hugin/huginApp.h"
#include "hugin/CPEditorPanel.h"
#include "hugin/CPListFrame.h"
#include "hugin/LocalizedFileTipProvider.h"
#include "hugin/HFOVDialog.h"
#include "algorithms/control_points/CleanCP.h"
#include "hugin/PanoOperation.h"

#include "base_wx/MyProgressDialog.h"
#include "base_wx/RunStitchPanel.h"
#include "base_wx/wxImageCache.h"
#include "base_wx/PTWXDlg.h"
#include "PT/ImageGraph.h"

#include "base_wx/huginConfig.h"
#include "hugin/AboutDialog.h"

#if HUGIN_HSI
#include "PluginItems.h"
#endif

using namespace PT;
using namespace std;
using namespace hugin_utils;

#ifdef __MINGW32__
// fixes for mingw compilation...
#undef FindWindow
#endif

/** class for showing splash screen
    the class wxSplashScreen from wxWidgets does not work correctly for our use case, it closes
    automatically the window if the user presses a key or does mouse clicks. These modified
    version does ignore these events. The window needs to be explicitly closed by the caller */
class HuginSplashScreen : public wxFrame
{
public:
    HuginSplashScreen() {};
    HuginSplashScreen(wxWindow* parent, wxBitmap bitmap) : 
        wxFrame(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxFRAME_TOOL_WINDOW | wxFRAME_NO_TASKBAR | wxSTAY_ON_TOP)
    {
        SetExtraStyle(GetExtraStyle() | wxWS_EX_TRANSIENT);
        wxSizer* topSizer=new wxBoxSizer(wxVERTICAL);
        wxStaticBitmap* staticBitmap=new wxStaticBitmap(this,wxID_ANY,bitmap);
        topSizer->Add(staticBitmap,1,wxEXPAND);
        SetSizerAndFit(topSizer);
        SetClientSize(bitmap.GetWidth(), bitmap.GetHeight());
        CenterOnScreen();
        Show(true);
        SetFocus();
#if defined(__WXMSW__) || defined(__WXMAC__)
        Update();
#elif defined(__WXGTK20__)
        //do nothing
#else
        wxYieldIfNeeded();
#endif
    };
    DECLARE_DYNAMIC_CLASS(HuginSplashScreen)
    DECLARE_EVENT_TABLE()
};

IMPLEMENT_DYNAMIC_CLASS(HuginSplashScreen, wxFrame)
BEGIN_EVENT_TABLE(HuginSplashScreen, wxFrame)
END_EVENT_TABLE()

/** file drag and drop handler method */
bool PanoDropTarget::OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames)
{
    DEBUG_TRACE("OnDropFiles");
    MainFrame * mf = MainFrame::Get();
    if (!mf) return false;

    if (!m_imageOnly && filenames.GetCount() == 1) {
        wxFileName file(filenames[0]);
        if (file.GetExt().CmpNoCase(wxT("pto")) == 0 ||
            file.GetExt().CmpNoCase(wxT("ptp")) == 0 ||
            file.GetExt().CmpNoCase(wxT("pts")) == 0 )
        {
            // load project
            if (mf->CloseProject(true)) {
                mf->LoadProjectFile(file.GetFullPath());
                // remove old images from cache
                ImageCache::getInstance().flush();
            }
            return true;
        }
    }

    // try to add as images
    std::vector<std::string> filesv;
    wxArrayString invalidFiles;
    for (unsigned int i=0; i< filenames.GetCount(); i++) {
        wxFileName file(filenames[i]);

        if (file.GetExt().CmpNoCase(wxT("jpg")) == 0 ||
            file.GetExt().CmpNoCase(wxT("jpeg")) == 0 ||
            file.GetExt().CmpNoCase(wxT("tif")) == 0 ||
            file.GetExt().CmpNoCase(wxT("tiff")) == 0 ||
            file.GetExt().CmpNoCase(wxT("png")) == 0 ||
            file.GetExt().CmpNoCase(wxT("bmp")) == 0 ||
            file.GetExt().CmpNoCase(wxT("gif")) == 0 ||
            file.GetExt().CmpNoCase(wxT("pnm")) == 0 ||
            file.GetExt().CmpNoCase(wxT("sun")) == 0 ||
            file.GetExt().CmpNoCase(wxT("hdr")) == 0 ||
            file.GetExt().CmpNoCase(wxT("viff")) == 0 )
        {
            if(containsInvalidCharacters(filenames[i]))
            {
                invalidFiles.Add(file.GetFullPath());
            }
            else
            {
                filesv.push_back((const char *)filenames[i].mb_str(HUGIN_CONV_FILENAME));
            };
        }
    }
    // we got some images to add.
    if (filesv.size() > 0)
    {
        // use a Command to ensure proper undo and updating of GUI parts
        wxBusyCursor();
        if(pano.getNrOfCtrlPoints()>0)
        {
            GlobalCmdHist::getInstance().addCommand(new wxAddImagesCmd(pano,filesv));
        }
        else
        {
            std::vector<PanoCommand*> cmds;
            cmds.push_back(new wxAddImagesCmd(pano, filesv));
            cmds.push_back(new PT::DistributeImagesCmd(pano));
            cmds.push_back(new PT::CenterPanoCmd(pano));
            GlobalCmdHist::getInstance().addCommand(new PT::CombinedPanoCommand(pano, cmds));
        };

    }
    if(invalidFiles.size()>0)
    {
        ShowFilenameWarning(mf, invalidFiles);
    }

    return true;
}



// event table. this frame will recieve mostly global commands.
BEGIN_EVENT_TABLE(MainFrame, wxFrame)
    EVT_MENU(XRCID("action_new_project"),  MainFrame::OnNewProject)
    EVT_MENU(XRCID("action_load_project"),  MainFrame::OnLoadProject)
    EVT_MENU(XRCID("action_save_project"),  MainFrame::OnSaveProject)
    EVT_MENU(XRCID("action_save_as_project"),  MainFrame::OnSaveProjectAs)
    EVT_MENU(XRCID("action_save_as_ptstitcher"),  MainFrame::OnSavePTStitcherAs)
    EVT_MENU(XRCID("action_open_batch_processor"),  MainFrame::OnOpenPTBatcher)
    EVT_MENU(XRCID("action_import_project"), MainFrame::OnMergeProject)
    EVT_MENU(XRCID("action_apply_template"),  MainFrame::OnApplyTemplate)
    EVT_MENU(XRCID("action_exit_hugin"),  MainFrame::OnUserQuit)
    EVT_MENU_RANGE(wxID_FILE1, wxID_FILE9, MainFrame::OnMRUFiles)
    EVT_MENU(XRCID("action_show_about"),  MainFrame::OnAbout)
    EVT_MENU(XRCID("action_show_help"),  MainFrame::OnHelp)
    EVT_MENU(XRCID("action_show_tip"),  MainFrame::OnTipOfDay)
    EVT_MENU(XRCID("action_show_shortcuts"),  MainFrame::OnKeyboardHelp)
    EVT_MENU(XRCID("action_show_faq"),  MainFrame::OnFAQ)
    EVT_MENU(XRCID("action_show_donate"),  MainFrame::OnShowDonate)
    EVT_MENU(XRCID("action_show_prefs"), MainFrame::OnShowPrefs)
    EVT_MENU(XRCID("action_assistant"), MainFrame::OnRunAssistant)
    EVT_MENU(XRCID("action_batch_assistant"), MainFrame::OnSendToAssistantQueue)
    EVT_MENU(XRCID("action_gui_simple"), MainFrame::OnSetGuiSimple)
    EVT_MENU(XRCID("action_gui_advanced"), MainFrame::OnSetGuiAdvanced)
    EVT_MENU(XRCID("action_gui_expert"), MainFrame::OnSetGuiExpert)
#ifdef HUGIN_HSI
    EVT_MENU(XRCID("action_python_script"), MainFrame::OnPythonScript)
#endif
    EVT_MENU(XRCID("ID_EDITUNDO"), MainFrame::OnUndo)
    EVT_MENU(XRCID("ID_EDITREDO"), MainFrame::OnRedo)
    EVT_MENU(XRCID("ID_SHOW_FULL_SCREEN"), MainFrame::OnFullScreen)
    EVT_MENU(XRCID("ID_SHOW_PREVIEW_FRAME"), MainFrame::OnTogglePreviewFrame)
    EVT_MENU(XRCID("ID_SHOW_GL_PREVIEW_FRAME"), MainFrame::OnToggleGLPreviewFrame)
    EVT_BUTTON(XRCID("ID_SHOW_PREVIEW_FRAME"),MainFrame::OnTogglePreviewFrame)
    EVT_BUTTON(XRCID("ID_SHOW_GL_PREVIEW_FRAME"), MainFrame::OnToggleGLPreviewFrame)

    EVT_MENU(XRCID("action_optimize"),  MainFrame::OnOptimize)
    EVT_BUTTON(XRCID("action_optimize"),  MainFrame::OnOptimize)
    EVT_MENU(XRCID("action_finetune_all_cp"), MainFrame::OnFineTuneAll)
//    EVT_BUTTON(XRCID("action_finetune_all_cp"), MainFrame::OnFineTuneAll)
    EVT_MENU(XRCID("action_remove_cp_in_masks"), MainFrame::OnRemoveCPinMasks)

    EVT_MENU(XRCID("ID_CP_TABLE"), MainFrame::OnShowCPFrame)
    EVT_BUTTON(XRCID("ID_CP_TABLE"),MainFrame::OnShowCPFrame)

    EVT_MENU(XRCID("ID_SHOW_PANEL_IMAGES"), MainFrame::OnShowPanel)
    EVT_MENU(XRCID("ID_SHOW_PANEL_MASK"), MainFrame::OnShowPanel)
    EVT_MENU(XRCID("ID_SHOW_PANEL_CP_EDITOR"), MainFrame::OnShowPanel)
    EVT_MENU(XRCID("ID_SHOW_PANEL_OPTIMIZER"), MainFrame::OnShowPanel)
    EVT_MENU(XRCID("ID_SHOW_PANEL_OPTIMIZER_PHOTOMETRIC"), MainFrame::OnShowPanel)
    EVT_MENU(XRCID("ID_SHOW_PANEL_PANORAMA"), MainFrame::OnShowPanel)
    EVT_MENU(XRCID("action_add_images"),  MainFrame::OnAddImages)
    EVT_BUTTON(XRCID("action_add_images"),  MainFrame::OnAddImages)
    EVT_MENU(XRCID("action_add_time_images"),  MainFrame::OnAddTimeImages)
    EVT_BUTTON(XRCID("action_add_time_images"),  MainFrame::OnAddTimeImages)
    EVT_CLOSE(  MainFrame::OnExit)
    EVT_SIZE(MainFrame::OnSize)
END_EVENT_TABLE()

// change this variable definition
//wxTextCtrl *itemProjTextMemo;
// image preview
//wxBitmap *p_img = (wxBitmap *) NULL;
//WX_DEFINE_ARRAY()

MainFrame::MainFrame(wxWindow* parent, Panorama & pano)
    : cp_frame(0), pano(pano)
{
    preview_frame = 0;
    m_progressMax = 1;
    m_progress = 0;
    svmModel=NULL;

    bool disableOpenGL=false;
#if wxCHECK_VERSION(2,9,4)
    if(wxGetKeyState(WXK_COMMAND))
#else
    if(wxGetKeyState(WXK_CONTROL))
#endif
    {
        wxDialog dlg;
        wxXmlResource::Get()->LoadDialog(&dlg, NULL, wxT("disable_opengl_dlg"));
        long noOpenGL=wxConfigBase::Get()->Read(wxT("DisableOpenGL"), 0l);
        if(noOpenGL==1)
        {
            XRCCTRL(dlg, "disable_dont_ask_checkbox", wxCheckBox)->SetValue(true);
        };
        if(dlg.ShowModal()==wxID_OK)
        {
            if(XRCCTRL(dlg, "disable_dont_ask_checkbox", wxCheckBox)->IsChecked())
            {
                wxConfigBase::Get()->Write(wxT("DisableOpenGL"), 1l);
            }
            else
            {
                wxConfigBase::Get()->Write(wxT("DisableOpenGL"), 0l);
            };
            disableOpenGL=true;
        }
        else
        {
            wxConfigBase::Get()->Write(wxT("DisableOpenGL"), 0l);
        };
    }
    else
    {
        long noOpenGL=wxConfigBase::Get()->Read(wxT("DisableOpenGL"), 0l);
        disableOpenGL=(noOpenGL==1);
    };

    wxBitmap bitmap;
    HuginSplashScreen* splash = 0;
    wxYield();

    if (bitmap.LoadFile(huginApp::Get()->GetXRCPath() + wxT("data/splash.png"), wxBITMAP_TYPE_PNG))
    {
        // embed package version into string.
        {
            wxMemoryDC dc;
            dc.SelectObject(bitmap);
#ifdef __WXMAC__
            wxFont font(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
#else
            wxFont font(8, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
#endif
            dc.SetFont(font);
            dc.SetTextForeground(*wxBLACK);
            dc.SetTextBackground(*wxWHITE);
            int tw, th;
            wxString version;
            version.Printf(_("Version %s"),wxString(DISPLAY_VERSION, wxConvLocal).c_str());
            dc.GetTextExtent(version, &tw, &th);
            // place text on bitmap.
            dc.DrawText(version, bitmap.GetWidth() - tw - 3, bitmap.GetHeight() - th - 3);
        }

        splash = new HuginSplashScreen(NULL, bitmap);
    } else {
        wxLogFatalError(_("Fatal installation error\nThe file data/splash.png was not found at:") + huginApp::Get()->GetXRCPath());
        abort();
    }
    //splash->Refresh();
    wxYield();

    // save our pointer
    m_this = this;

    DEBUG_TRACE("");
    // load our children. some children might need special
    // initialization. this will be done later.
    wxXmlResource::Get()->LoadFrame(this, parent, wxT("main_frame"));
    DEBUG_TRACE("");

    // load our menu bar
#ifdef __WXMAC__
    wxApp::s_macAboutMenuItemId = XRCID("action_show_about");
    wxApp::s_macPreferencesMenuItemId = XRCID("action_show_prefs");
    wxApp::s_macExitMenuItemId = XRCID("action_exit_hugin");
    wxApp::s_macHelpMenuTitleName = _("&Help");
#endif
    wxMenuBar* mainMenu=wxXmlResource::Get()->LoadMenuBar(this, wxT("main_menubar"));
    m_menu_file_simple=wxXmlResource::Get()->LoadMenu(wxT("file_menu_simple"));
    m_menu_file_advanced=wxXmlResource::Get()->LoadMenu(wxT("file_menu_advanced"));
    mainMenu->Insert(0, m_menu_file_simple, _("&File"));
    SetMenuBar(mainMenu);

#ifdef HUGIN_HSI
    wxMenuBar* menubar=GetMenuBar();
    // the plugin menu will be generated dynamically
    wxMenu *pluginMenu=new wxMenu();
    // search for all .py files in plugins directory
    wxDir dir(GetDataPath()+wxT("plugins"));
    wxString filename;
    bool cont=dir.GetFirst(&filename,wxT("*.py"),wxDIR_FILES|wxDIR_HIDDEN);
    PluginItems items;
    while(cont)
    {
        wxFileName file(dir.GetName(),filename);
        file.MakeAbsolute();
        PluginItem item(file);
        if(item.IsAPIValid())
        {
            items.push_back(item);
        };
        cont=dir.GetNext(&filename);
    };
    items.sort(comparePluginItem);

    int pluginID=wxID_HIGHEST+2000;
    for(PluginItems::const_iterator it=items.begin();it!=items.end();it++)
    {
        PluginItem item=*it;
        int categoryID=pluginMenu->FindItem(item.GetCategory());
        wxMenu* categoryMenu;
        if(categoryID==wxNOT_FOUND)
        {
            categoryMenu=new wxMenu();
            pluginMenu->AppendSubMenu(categoryMenu,item.GetCategory());
        }
        else
        {
            categoryMenu=pluginMenu->FindItem(categoryID)->GetSubMenu();
        };
        categoryMenu->Append(pluginID,item.GetName(),item.GetDescription());
        m_plugins[pluginID]=item.GetFilename();
        Connect(pluginID, wxEVT_COMMAND_MENU_SELECTED,wxCommandEventHandler(MainFrame::OnPlugin));
        pluginID++;
    };
    // show the new menu
    if(pluginMenu->GetMenuItemCount()>0)
    {
        menubar->Insert(menubar->GetMenuCount()-2,pluginMenu,_("&Actions"));
    };
#else
    GetMenuBar()->Enable(XRCID("action_python_script"), false);
#endif

    // create tool bar
    SetToolBar(wxXmlResource::Get()->LoadToolBar(this, wxT("main_toolbar")));

    // Disable tools by default
    enableTools(false);

    // put an "unknown" object in an xrc file and
    // take as wxObject (second argument) the return value of wxXmlResource::Get
    // finish the images_panel
    DEBUG_TRACE("");

    // image_panel
    images_panel = XRCCTRL(*this, "images_panel_unknown", ImagesPanel);
    assert(images_panel);
    images_panel->Init(&pano);
    DEBUG_TRACE("");

    m_notebook = XRCCTRL((*this), "controls_notebook", wxNotebook);
//    m_notebook = ((wxNotebook*) ((*this).FindWindow(XRCID("controls_notebook"))));
//    m_notebook = ((wxNotebook*) (FindWindow(XRCID("controls_notebook"))));
    DEBUG_ASSERT(m_notebook);

    // the mask panel
    mask_panel = XRCCTRL(*this, "mask_panel_unknown", MaskEditorPanel);
    assert(mask_panel);
    mask_panel->Init(&pano);

    // the pano_panel
    DEBUG_TRACE("");
    pano_panel = XRCCTRL(*this, "panorama_panel_unknown", PanoPanel);
    assert(pano_panel);
    pano_panel->Init(&pano);
    pano_panel->panoramaChanged (pano); // initialize from pano

    cpe = XRCCTRL(*this, "cp_editor_panel_unknown", CPEditorPanel);
    assert(cpe);
    cpe->Init(&pano);

    opt_panel = XRCCTRL(*this, "optimizer_panel_unknown", OptimizePanel);
    assert(opt_panel);
    opt_panel->Init(&pano);
    m_show_opt_panel=true;

    opt_photo_panel = XRCCTRL(*this, "optimizer_photometric_panel_unknown", OptimizePhotometricPanel);
    assert(opt_photo_panel);
    opt_photo_panel->Init(&pano);
    m_show_opt_photo_panel=true;

    // generate list of most recently uses files and add it to menu
    // needs to be initialized before the fast preview, there we call AddFilesToMenu()
    wxConfigBase * config=wxConfigBase::Get();
    m_mruFiles.Load(*config);
    m_mruFiles.UseMenu(m_menu_file_advanced->FindItem(XRCID("menu_mru"))->GetSubMenu());

    preview_frame = new PreviewFrame(this, pano);
    if(disableOpenGL)
    {
        gl_preview_frame=NULL;
        DisableOpenGLTools();
        m_mruFiles.AddFilesToMenu();
    }
    else
    {
        gl_preview_frame = new GLPreviewFrame(this, pano);
    };

    // set the minimize icon
#ifdef __WXMSW__
    wxIcon myIcon(GetXRCPath() + wxT("data/hugin.ico"),wxBITMAP_TYPE_ICO);
#else
    wxIcon myIcon(GetXRCPath() + wxT("data/hugin.png"),wxBITMAP_TYPE_PNG);
#endif
    SetIcon(myIcon);

    // create a new drop handler. wxwindows deletes the automaticall
    SetDropTarget(new PanoDropTarget(pano));
    DEBUG_TRACE("");

    PanoOperation::GeneratePanoOperationVector();

    // create a status bar
    const int fields (2);
    CreateStatusBar(fields);
    int widths[fields] = {-1, 85};
    SetStatusWidths( fields, &widths[0]);
    SetStatusText(_("Started"), 0);
    wxYield();
    DEBUG_TRACE("");

    // observe the panorama
    pano.addObserver(this);

    // Set sizing characteristics
    //set minumum size
#if defined __WXMAC__ || defined __WXMSW__
    // a minimum nice looking size; smaller than this would clutter the layout.
    SetSizeHints(900, 675);
#else
    // For ASUS eeePc
    SetSizeHints(780, 455); //set minumum size
#endif

    // set progress display for image cache.
    ImageCache::getInstance().setProgressDisplay(this);
#if defined __WXMSW__
    unsigned long long mem = HUGIN_IMGCACHE_UPPERBOUND;
    unsigned long mem_low = wxConfigBase::Get()->Read(wxT("/ImageCache/UpperBound"), HUGIN_IMGCACHE_UPPERBOUND);
    unsigned long mem_high = wxConfigBase::Get()->Read(wxT("/ImageCache/UpperBoundHigh"), (long) 0);
    if (mem_high > 0) {
      mem = ((unsigned long long) mem_high << 32) + mem_low;
    }
    else {
      mem = mem_low;
    }
    ImageCache::getInstance().SetUpperLimit(mem);
#else
    ImageCache::getInstance().SetUpperLimit(wxConfigBase::Get()->Read(wxT("/ImageCache/UpperBound"), HUGIN_IMGCACHE_UPPERBOUND));
#endif

    if(splash) {
        splash->Close();
        delete splash;
    }
    wxYield();

    // disable automatic Layout() calls, to it by hand
    SetAutoLayout(false);


#ifdef __WXMSW__
    // wxFrame does have a strange background color on Windows, copy color from a child widget
    this->SetBackgroundColour(images_panel->GetBackgroundColour());
#endif

// By using /SUBSYSTEM:CONSOLE /ENTRY:"WinMainCRTStartup" in the linker
// options for the debug build, a console window will be used for stdout
// and stderr. No need to redirect to a file. Better security since we can't
// guarantee that c: exists and writing a file to the root directory is
// never a good idea. release build still uses /SUBSYSTEM:WINDOWS

#if 0
#ifdef DEBUG
#ifdef __WXMSW__

    freopen("c:\\hugin_stdout.txt", "w", stdout);    // redirect stdout to file
    freopen("c:\\hugin_stderr.txt", "w", stderr);    // redirect stderr to file
#endif
#endif
#endif
    //reload gui level
    long guiLevel=config->Read(wxT("/GuiLevel"),(long)0);
    guiLevel=max<long>(0,min<long>(2,guiLevel));
    if(guiLevel==GUI_SIMPLE && disableOpenGL)
    {
        guiLevel=GUI_ADVANCED;
    };
    SetGuiLevel((GuiLevel)guiLevel);

    DEBUG_TRACE("");
#ifdef __WXGTK__
    // set explicit focus to assistant panel for better processing key presses
    images_panel->SetFocus();
#endif
}

MainFrame::~MainFrame()
{
    DEBUG_TRACE("dtor");
    if(m_guiLevel==GUI_SIMPLE)
    {
        delete m_menu_file_advanced;
    }
    else
    {
        delete m_menu_file_simple;
    };
    ImageCache::getInstance().setProgressDisplay(0);
	delete & ImageCache::getInstance();
	delete & GlobalCmdHist::getInstance();
//    delete cpe;
//    delete images_panel;
    DEBUG_DEBUG("removing observer");
    pano.removeObserver(this);

    // get the global config object
    wxConfigBase* config = wxConfigBase::Get();

    StoreFramePosition(this, wxT("MainFrame"));

    //store most recently used files
    m_mruFiles.Save(*config);
    //store gui level
    config->Write(wxT("/GuiLevel"),(long)m_guiLevel);
    config->Flush();
    if(svmModel!=NULL)
    {
        celeste::destroySVMmodel(svmModel);
    };
    PanoOperation::CleanPanoOperationVector();

    DEBUG_TRACE("dtor end");
}

void MainFrame::panoramaChanged(PT::Panorama &pano)
{
    wxToolBar* theToolBar = GetToolBar();
    wxMenuBar* theMenuBar = GetMenuBar();
    bool can_undo = GlobalCmdHist::getInstance().canUndo();
    theMenuBar->Enable    (XRCID("ID_EDITUNDO"), can_undo);
    theToolBar->EnableTool(XRCID("ID_EDITUNDO"), can_undo);
    bool can_redo = GlobalCmdHist::getInstance().canRedo();
    theMenuBar->Enable    (XRCID("ID_EDITREDO"), can_redo);
    theToolBar->EnableTool(XRCID("ID_EDITREDO"), can_redo);

    //show or hide optimizer and exposure optimizer tab depending on optimizer master switches
    if(pano.getOptimizerSwitch()==0 && !m_show_opt_panel)
    {
        m_notebook->InsertPage(3, opt_panel, _("Optimizer"));
        m_show_opt_panel=true;
    };
    if(pano.getOptimizerSwitch()!=0 && m_show_opt_panel)
    {
        m_notebook->RemovePage(3);
        m_show_opt_panel=false;
    };
    if(pano.getPhotometricOptimizerSwitch()==0 && !m_show_opt_photo_panel)
    {
        if(m_show_opt_panel)
        {
            m_notebook->InsertPage(4, opt_photo_panel, _("Exposure"));
        }
        else
        {
            m_notebook->InsertPage(3, opt_photo_panel, _("Exposure"));
        }
        m_show_opt_photo_panel=true;
    };
    if(pano.getPhotometricOptimizerSwitch()!=0 && m_show_opt_photo_panel)
    {
        if(m_show_opt_panel)
        {
            m_notebook->RemovePage(4);
        }
        else
        {
            m_notebook->RemovePage(3);
        };
        m_show_opt_photo_panel=false;
    };
    theMenuBar->Enable(XRCID("ID_SHOW_PANEL_OPTIMIZER"), m_show_opt_panel);
    theMenuBar->Enable(XRCID("ID_SHOW_PANEL_OPTIMIZER_PHOTOMETRIC"), m_show_opt_photo_panel);
}

//void MainFrame::panoramaChanged(PT::Panorama &panorama)
void MainFrame::panoramaImagesChanged(PT::Panorama &panorama, const PT::UIntSet & changed)
{
    DEBUG_TRACE("");
    assert(&pano == &panorama);
    if (pano.getNrOfImages() == 0) {
	  enableTools(false);
	} else {
	  enableTools(true);
	}
    GetMenuBar()->Enable(XRCID("action_assistant"), pano.getNrOfImages()>=2);
    GetMenuBar()->Enable(XRCID("action_batch_assistant"), pano.getNrOfImages()>=2);
}

void MainFrame::OnUserQuit(wxCommandEvent & e)
{
    Close();
}

bool MainFrame::CloseProject(bool cancelable)
{
    if (pano.isDirty()) {
        wxMessageDialog message(wxTheApp->GetTopWindow(),
                                _("Save changes to the panorama before closing?"),
#ifdef _WINDOWS
                                _("Hugin"),
#else
                                wxT(""),
#endif
                                wxICON_EXCLAMATION | wxYES_NO | (cancelable? (wxCANCEL):0));
#if wxCHECK_VERSION(2, 9, 0)
    message.SetExtendedMessage(_("If you close without saving, changes since your last save will be discarded"));
    #if defined __WXMAC__ || defined __WXMSW__
        // Apple human interface guidelines and Windows user experience interaction guidelines
        message.SetYesNoLabels(wxID_SAVE, _("Don't Save"));
    #else
        // Gnome human interface guidelines:
        message.SetYesNoLabels(wxID_SAVE, _("Close without saving"));
    #endif
#endif
        int answer = message.ShowModal();
        switch (answer){
            case wxID_YES:
            {
                wxCommandEvent dummy;
                OnSaveProject(dummy);
                return !pano.isDirty();
            }
            case wxID_CANCEL:
                return false;
            default: //no save
                return true;
        }
    }
    else return true;
}

void MainFrame::OnExit(wxCloseEvent & e)
{
    DEBUG_TRACE("");
    if(m_guiLevel!=GUI_SIMPLE)
    {
        if(!CloseProject(e.CanVeto()))
        {
           if (e.CanVeto())
           {
                e.Veto();
                return;
           }
           wxLogError(_("forced close"));
        }
    }
    else
    {
        if(e.CanVeto())
        {
            Hide();
            e.Veto();
            return;
        };
    };

    if(preview_frame)
    {
       preview_frame->Close(true);
    }

    ImageCache::getInstance().flush();
    Destroy();
    DEBUG_TRACE("");
}

void MainFrame::OnSaveProject(wxCommandEvent & e)
{
    DEBUG_TRACE("");
    bool savedProjectFile=false;
    try {
    wxFileName scriptName = m_filename;
    if (m_filename == wxT("")) {
        OnSaveProjectAs(e);
        scriptName = m_filename;
    } else {
        // the project file is just a PTOptimizer script...
        std::string path = getPathPrefix(std::string(scriptName.GetFullPath().mb_str(HUGIN_CONV_FILENAME)));
        DEBUG_DEBUG("stripping " << path << " from image filenames");
        std::ofstream script(scriptName.GetFullPath().mb_str(HUGIN_CONV_FILENAME));
        script.exceptions ( std::ofstream::eofbit | std::ofstream::failbit | std::ofstream::badbit );
        PT::UIntSet all;
        if (pano.getNrOfImages() > 0) {
           fill_set(all, 0, pano.getNrOfImages()-1);
        }
        pano.printPanoramaScript(script, pano.getOptimizeVector(), pano.getOptions(), all, false, path);
        script.close();
        savedProjectFile=true;

        int createMakefile = 1;
#if defined __WXMAC__ && defined MAC_SELF_CONTAINED_BUNDLE
        createMakefile = 0;
#endif
        if (createMakefile && pano.getNrOfImages() > 0) {
            wxString makefn = scriptName.GetFullPath() + wxT(".mk");
            std::ofstream makefile(makefn.mb_str(HUGIN_CONV_FILENAME));
            makefile.exceptions ( std::ofstream::eofbit | std::ofstream::failbit | std::ofstream::badbit );
            wxString ptoFnWX = scriptName.GetFullPath();
            std::string ptoFn(ptoFnWX.mb_str(HUGIN_CONV_FILENAME));
            wxString bindir = huginApp::Get()->GetUtilsBinDir();
            PTPrograms progs = getPTProgramsConfig(bindir, wxConfigBase::Get());
            std::string resultFn;
            wxString resultFnwx = scriptName.GetFullPath();
            resultFn = resultFnwx.mb_str(HUGIN_CONV_FILENAME);
            resultFn = stripPath(stripExtension(resultFn));
            std::string tmpDir((wxConfigBase::Get()->Read(wxT("tempDir"),wxT(""))).mb_str(HUGIN_CONV_FILENAME));

            std::vector<std::string> outputFiles;
            HuginBase::PanoramaMakefilelibExport::createMakefile(pano,
                                                              pano.getActiveImages(),
                                                              ptoFn,
                                                              resultFn,
                                                              progs,
                                                              "",
                                                              outputFiles,
                                                              makefile,
                                                              tmpDir,
                                                              true,
                                                              0);
        }
        SetStatusText(wxString::Format(_("saved project %s"), m_filename.c_str()),0);
        if(m_guiLevel==GUI_SIMPLE)
        {
            if(gl_preview_frame)
            {
                gl_preview_frame->SetTitle(scriptName.GetName() + wxT(".") + scriptName.GetExt() + wxT(" - ") + _("Hugin - Panorama Stitcher"));
            };
            SetTitle(scriptName.GetName() + wxT(".") + scriptName.GetExt() + wxT(" - ") + _("Panorama editor"));
        }
        else
        {
            SetTitle(scriptName.GetName() + wxT(".") + scriptName.GetExt() + wxT(" - ") + _("Hugin - Panorama Stitcher"));
        };

        pano.clearDirty();
    }
    } catch (std::exception & e) {
        wxString err(e.what(), wxConvLocal);
        if(savedProjectFile)
        {
            wxMessageBox(wxString::Format(_("Could not save project makefile \"%s\".\nBut the project file was saved.\nMaybe the file or the folder is read-only.\n\n(Error code: %s)"),(m_filename+wxT(".mk")).c_str(),err.c_str()),_("Warning"),wxOK|wxICON_INFORMATION);
        }
        else
        {
            wxMessageBox(wxString::Format(_("Could not save project file \"%s\".\nMaybe the file or the folder is read-only.\n\n(Error code: %s)"),m_filename.c_str(),err.c_str()),_("Error"),wxOK|wxICON_ERROR);
        };
    }
}

void MainFrame::OnSaveProjectAs(wxCommandEvent & e)
{
    DEBUG_TRACE("");
    wxFileName scriptName;
    if (m_filename.IsEmpty())
    {
        scriptName.Assign(getDefaultProjectName(pano) + wxT(".pto"));
    }
    else
    {
        scriptName=m_filename;
    };
    scriptName.Normalize();
    wxFileDialog dlg(wxTheApp->GetTopWindow(),
                     _("Save project file"),
                     scriptName.GetPath(), scriptName.GetFullName(),
                     _("Project files (*.pto)|*.pto|All files (*)|*"),
                     wxFD_SAVE, wxDefaultPosition);
    dlg.SetDirectory(wxConfigBase::Get()->Read(wxT("/actualPath"),wxT("")));
    if (dlg.ShowModal() == wxID_OK) {
        wxConfig::Get()->Write(wxT("/actualPath"), dlg.GetDirectory());  // remember for later
        wxString fn = dlg.GetPath();
        if (fn.Right(4) != wxT(".pto")) {
            fn.Append(wxT(".pto"));
        }
        if (wxFile::Exists(fn)) {
            int d = wxMessageBox(wxString::Format(_("File %s exists. Overwrite?"), fn.c_str()),
                                 _("Save project"), wxYES_NO | wxICON_QUESTION);
            if (d != wxYES) {
                return;
            }
        }
        m_filename = fn;
        m_mruFiles.AddFileToHistory(m_filename);
        OnSaveProject(e);
    }
}

void MainFrame::OnSavePTStitcherAs(wxCommandEvent & e)
{
    DEBUG_TRACE("");
    wxString scriptName = m_filename;
    if (m_filename == wxT("")) {
        scriptName = getDefaultProjectName(pano);
    }
    wxFileName scriptNameFN(scriptName);
    wxString fn = scriptNameFN.GetName() + wxT(".txt");
    wxFileDialog dlg(wxTheApp->GetTopWindow(),
                     _("Save PTmender script file"),
                     wxConfigBase::Get()->Read(wxT("/actualPath"),wxT("")), fn,
                     _("PTmender files (*.txt)|*.txt"),
                     wxFD_SAVE, wxDefaultPosition);
    dlg.SetDirectory(wxConfigBase::Get()->Read(wxT("/actualPath"),wxT("")));
    if (dlg.ShowModal() == wxID_OK) {
        wxString fname = dlg.GetPath();
        // the project file is just a PTStitcher script...
        wxFileName scriptName = fname;
        PT::UIntSet all;
        if (pano.getNrOfImages() > 0) {
            fill_set(all, 0, pano.getNrOfImages()-1);
        }
        std::ofstream script(scriptName.GetFullPath().mb_str(HUGIN_CONV_FILENAME));
        pano.printStitcherScript(script, pano.getOptions(), all);
        script.close();
    }

}

void MainFrame::LoadProjectFile(const wxString & filename)
{
    DEBUG_TRACE("");
    m_filename = filename;

    // remove old images from cache
    // hmm probably not a good idea, if the project is reloaded..
    // ImageCache::getInstance().flush();

    SetStatusText( _("Open project:   ") + filename);

    wxFileName fname(filename);
    wxString path = fname.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
    if (fname.IsOk() && fname.FileExists()) {
        wxBusyCursor wait;
        deregisterPTWXDlgFcn();
        GlobalCmdHist::getInstance().addCommand(
           new wxLoadPTProjectCmd(pano,(const char *)filename.mb_str(HUGIN_CONV_FILENAME), (const char *)path.mb_str(HUGIN_CONV_FILENAME), true)
           );
        GlobalCmdHist::getInstance().clear();
        registerPTWXDlgFcn(wxGetApp().GetTopWindow());
        DEBUG_DEBUG("project contains " << pano.getNrOfImages() << " after load");
        GuiLevel reqGuiLevel=GetMinimumGuiLevel(pano);
        if(reqGuiLevel>m_guiLevel)
        {
            SetGuiLevel(reqGuiLevel);
        };
        SetStatusText(_("Project opened"));
        m_mruFiles.AddFileToHistory(fname.GetFullPath());
        if(m_guiLevel==GUI_SIMPLE)
        {
            if(gl_preview_frame)
            {
                gl_preview_frame->SetTitle(fname.GetName() + wxT(".") + fname.GetExt() + wxT(" - ") + _("Hugin - Panorama Stitcher"));
            };
            SetTitle(fname.GetName() + wxT(".") + fname.GetExt() + wxT(" - ") + _("Panorama editor"));
        }
        else
        {
            SetTitle(fname.GetName() + wxT(".") + fname.GetExt() + wxT(" - ") + _("Hugin - Panorama Stitcher"));
        };
        if (! (fname.GetExt() == wxT("pto"))) {
            // do not remember filename if its not a hugin project
            // to avoid overwriting the original project with an
            // incompatible one
            m_filename = wxT("");
        }
        // get the global config object
        wxConfigBase* config = wxConfigBase::Get();
        config->Write(wxT("/actualPath"), path);  // remember for later
    } else {
        SetStatusText( _("Error opening project:   ") + filename);
        DEBUG_ERROR("Could not open file " << filename);
    }

    // force update of preview window
    if ( !(preview_frame->IsIconized() ||(! preview_frame->IsShown()) ) ) {
        wxCommandEvent dummy;
        preview_frame->OnUpdate(dummy);
    }
}

#ifdef __WXMAC__
void MainFrame::MacOnOpenFile(const wxString & filename)
{
    if(!CloseProject(true)) return; //if closing old project is canceled do nothing.

    ImageCache::getInstance().flush();
    LoadProjectFile(filename);
}
#endif

void MainFrame::OnLoadProject(wxCommandEvent & e)
{
    DEBUG_TRACE("");

    if(CloseProject(true)) //if closing old project is canceled do nothing.
    {
        // get the global config object
        wxConfigBase* config = wxConfigBase::Get();

        wxString defaultdir = config->Read(wxT("/actualPath"),wxT(""));
        wxFileDialog dlg(wxTheApp->GetTopWindow(),
                         _("Open project file"),
                         defaultdir, wxT(""),
                         _("Project files (*.pto,*.ptp,*.pts,*.oto)|*.pto;*.ptp;*.pts;*.oto;|All files (*)|*"),
                         wxFD_OPEN, wxDefaultPosition);
        dlg.SetDirectory(defaultdir);
        if (dlg.ShowModal() == wxID_OK)
        {
            wxString filename = dlg.GetPath();
            if(vigra::isImage(filename.mb_str(HUGIN_CONV_FILENAME)))
            {
                if(wxMessageBox(wxString::Format(_("File %s is an image file and not a project file.\nThis file can't be open with File, Open.\nDo you want to add this image file to the current project?"),filename.c_str()),
#ifdef __WXMSW__
                    _("Hugin"),
#else
                    wxT(""),
#endif
                    wxYES_NO | wxICON_QUESTION)==wxYES)
                {
                    wxArrayString filenameArray;
                    filenameArray.Add(filename);
                    AddImages(filenameArray);
                };
                return;
            }
            else
            {
                // remove old images from cache
                ImageCache::getInstance().flush();

                LoadProjectFile(filename);
                return;
            };
        }
    }
    // do not close old project
    // nothing to open
    SetStatusText( _("Open project: cancel"));
}

void MainFrame::OnNewProject(wxCommandEvent & e)
{
    if(!CloseProject(true)) return; //if closing current project is canceled

    m_filename = wxT("");
    GlobalCmdHist::getInstance().addCommand( new wxNewProjectCmd(pano));
    GlobalCmdHist::getInstance().clear();
    // remove old images from cache
    ImageCache::getInstance().flush();
    if(m_guiLevel==GUI_SIMPLE)
    {
        if(gl_preview_frame)
        {
            gl_preview_frame->SetTitle(_("Hugin - Panorama Stitcher"));
        };
        SetTitle(_("Panorama editor"));
    }
    else
    {
        SetTitle(_("Hugin - Panorama Stitcher"));
    };

    wxCommandEvent dummy;
    preview_frame->OnUpdate(dummy);
}

void MainFrame::OnAddImages( wxCommandEvent& event )
{
    DEBUG_TRACE("");
    PanoOperation::AddImageOperation addImage;
    UIntSet images;
    PanoCommand* cmd=addImage.GetCommand(wxTheApp->GetTopWindow(),pano,images);
    if(cmd!=NULL)
    {
        GlobalCmdHist::getInstance().addCommand(cmd);
    }
    else
    {
        // nothing to open
        SetStatusText( _("Add Image: cancel"));
    }

    DEBUG_TRACE("");
}

void MainFrame::AddImages(wxArrayString& filenameArray)
{
    wxArrayString invalidFiles;
    for(unsigned int i=0;i<filenameArray.GetCount(); i++)
    {
        if(containsInvalidCharacters(filenameArray[i]))
        {
            invalidFiles.Add(filenameArray[i]);
        };
    };
    if(invalidFiles.size()>0)
    {
        ShowFilenameWarning(this, invalidFiles);
    }
    else
    {
        std::vector<std::string> filesv;
        for (unsigned int i=0; i< filenameArray.GetCount(); i++) {
            filesv.push_back((const char *)filenameArray[i].mb_str(HUGIN_CONV_FILENAME));
        }

        // we got some images to add.
        if (filesv.size() > 0) {
            // use a Command to ensure proper undo and updating of GUI
            // parts
            wxBusyCursor();
            GlobalCmdHist::getInstance().addCommand(
                new wxAddImagesCmd(pano,filesv)
                );
        };
    };
};

void MainFrame::OnAddTimeImages( wxCommandEvent& event )
{
    PanoOperation::AddImagesSeriesOperation imageSeriesOp;
    UIntSet images;
    PT::PanoCommand* cmd=imageSeriesOp.GetCommand(wxTheApp->GetTopWindow(),pano,images);
    if(cmd!=NULL)
    {
        GlobalCmdHist::getInstance().addCommand(cmd);
    };
};

void MainFrame::OnShowDonate(wxCommandEvent & e)
{
    wxLaunchDefaultBrowser(wxT("http://sourceforge.net/project/project_donations.php?group_id=77506"));
}


void MainFrame::OnShowPanel(wxCommandEvent & e)
{
    if(e.GetId()==XRCID("ID_SHOW_PANEL_MASK"))
        m_notebook->SetSelection(1);
    else
        if(e.GetId()==XRCID("ID_SHOW_PANEL_CP_EDITOR"))
            m_notebook->SetSelection(2);
        else
            if(e.GetId()==XRCID("ID_SHOW_PANEL_OPTIMIZER"))
                m_notebook->SetSelection(3);
            else
                if(e.GetId()==XRCID("ID_SHOW_PANEL_OPTIMIZER_PHOTOMETRIC"))
                {
                    if(m_show_opt_panel)
                    {
                        m_notebook->SetSelection(4);
                    }
                    else
                    {
                        m_notebook->SetSelection(3);
                    };
                }
                else
                    if(e.GetId()==XRCID("ID_SHOW_PANEL_PANORAMA"))
                    {
                        if(m_show_opt_panel && m_show_opt_photo_panel)
                        {
                            m_notebook->SetSelection(5);
                        }
                        else
                        {
                            if(m_show_opt_panel || m_show_opt_photo_panel)
                            {
                                m_notebook->SetSelection(4);
                            }
                            else
                            {
                                m_notebook->SetSelection(3);
                            };
                        };
                    }
                    else
                        m_notebook->SetSelection(0);
}


void MainFrame::OnAbout(wxCommandEvent & e)
{
    AboutDialog dlg(wxTheApp->GetTopWindow());
    dlg.ShowModal();
}

/*
void MainFrame::OnAbout(wxCommandEvent & e)
{
    DEBUG_TRACE("");
    wxDialog dlg;
	wxString strFile;
	wxString langCode;

    wxXmlResource::Get()->LoadDialog(&dlg, this, wxT("about_dlg"));

#if __WXMAC__ && defined MAC_SELF_CONTAINED_BUNDLE
    //rely on the system's locale choice
    strFile = MacGetPathToBundledResourceFile(CFSTR("about.htm"));
    if(strFile!=wxT("")) XRCCTRL(dlg,"about_html",wxHtmlWindow)->LoadPage(strFile);
#else
    //if the language is not default, load custom About file (if exists)
    langCode = huginApp::Get()->GetLocale().GetName().Left(2).Lower();
    DEBUG_INFO("Lang Code: " << langCode.mb_str(wxConvLocal));
    if(langCode != wxString(wxT("en")))
    {
        strFile = GetXRCPath() + wxT("data/about_") + langCode + wxT(".htm");
        if(wxFile::Exists(strFile))
        {
            DEBUG_TRACE("Using About: " << strFile.mb_str(wxConvLocal));
            XRCCTRL(dlg,"about_html",wxHtmlWindow)->LoadPage(strFile);
        }
    }
#endif
    dlg.ShowModal();
}
*/

void MainFrame::OnHelp(wxCommandEvent & e)
{
    DisplayHelp(wxT("/Hugin.html"));
}

void MainFrame::OnKeyboardHelp(wxCommandEvent & e)
{
    DisplayHelp(wxT("/Hugin_Keyboard_shortcuts.html"));
}

void MainFrame::OnFAQ(wxCommandEvent & e)
{
    DisplayHelp(wxT("/Hugin_FAQ.html"));
}


void MainFrame::DisplayHelp(wxString section)
{
    // TODO:
    // structure a frame with navigation on the left and content on the right
    // always load the same navigation on the left and the section into the frame
    // find a way to target always the same window rather than opening new window / tabs in the browser every time
    // make it look nicer with some CSS styling

    // section is the HTML document to be displayed, from inside the data folder
    if(section==wxT("")){
        section = wxT("/Hugin.html");
    }

    DEBUG_TRACE("");

#ifdef __WXMSW__
    GetHelpController().DisplaySection(section);
#else
#if defined __WXMAC__ && defined MAC_SELF_CONTAINED_BUNDLE
    // On Mac, xrc/data/help_LOCALE should be in the bundle as LOCALE.lproj/help
    // which we can rely on the operating sytem to pick the right locale's.
    wxString strFile = MacGetPathToBundledResourceFile(CFSTR("help"));
    if(strFile!=wxT(""))
    {
        strFile += section;
    } else {
        wxLogError(wxString::Format(wxT("Could not find help directory in the bundle"), strFile.c_str()));
        return;
    }
#else
    // find base filename
    wxString helpFile = wxT("help_") + huginApp::Get()->GetLocale().GetCanonicalName() + section;
    DEBUG_INFO("help file candidate: " << helpFile.mb_str(wxConvLocal));
    //if the language is not default, load custom About file (if exists)
    wxString strFile = GetXRCPath() + wxT("data/") + helpFile;
    if(wxFile::Exists(strFile))
    {
        DEBUG_TRACE("Using About: " << strFile.mb_str(wxConvLocal));
    } else {
        strFile = GetXRCPath() + wxT("data/help_en_EN") + section;
    }
#endif
    if(!wxFile::Exists(strFile))
    {
        wxLogError(wxString::Format(wxT("Could not open help file: %s"), strFile.c_str()));
        return;
    }
    DEBUG_INFO("help file: " << strFile.mb_str(wxConvLocal));
    if(!wxLaunchDefaultBrowser(strFile))
    {
        wxLogError(_("Can't start system's web browser"));
    }
#endif
}

void MainFrame::OnTipOfDay(wxCommandEvent& WXUNUSED(e))
{
    wxString strFile;
    bool bShowAtStartup;
// DGSW FIXME - Unreferenced
//	bool bTipsExist = false;
    int nValue;

    wxConfigBase * config = wxConfigBase::Get();
    nValue = config->Read(wxT("/MainFrame/ShowStartTip"),1l);

    //TODO: tips not localisable
    DEBUG_INFO("Tip index: " << nValue);
    strFile = GetXRCPath() + wxT("data/tips.txt");  //load default file

    DEBUG_INFO("Reading tips from " << strFile.mb_str(wxConvLocal));
    wxTipProvider *tipProvider = new LocalizedFileTipProvider(strFile, nValue);
    bShowAtStartup = wxShowTip(wxTheApp->GetTopWindow(), tipProvider,(nValue ? true:false));

    //store startup preferences
    nValue = (bShowAtStartup ? tipProvider->GetCurrentTip() : 0);
    DEBUG_INFO("Writing tip index: " << nValue);
    config->Write(wxT("/MainFrame/ShowStartTip"), nValue);
    delete tipProvider;
}


void MainFrame::OnShowPrefs(wxCommandEvent & e)
{
    DEBUG_TRACE("");
    PreferencesDialog pref_dlg(wxTheApp->GetTopWindow());
    pref_dlg.ShowModal();
    //update image cache size
    wxConfigBase* cfg=wxConfigBase::Get();
#if defined __WXMSW__
    unsigned long long mem = HUGIN_IMGCACHE_UPPERBOUND;
    unsigned long mem_low = cfg->Read(wxT("/ImageCache/UpperBound"), HUGIN_IMGCACHE_UPPERBOUND);
    unsigned long mem_high = cfg->Read(wxT("/ImageCache/UpperBoundHigh"), (long) 0);
    if (mem_high > 0)
    {
      mem = ((unsigned long long) mem_high << 32) + mem_low;
    }
    else
    {
      mem = mem_low;
    }
    ImageCache::getInstance().SetUpperLimit(mem);
#else
    ImageCache::getInstance().SetUpperLimit(cfg->Read(wxT("/ImageCache/UpperBound"), HUGIN_IMGCACHE_UPPERBOUND));
#endif
    images_panel->ReloadCPDetectorSettings();
    if(gl_preview_frame)
    {
        gl_preview_frame->SetShowProjectionHints(cfg->Read(wxT("/GLPreviewFrame/ShowProjectionHints"),HUGIN_SHOW_PROJECTION_HINTS)!=0);
    };
}

void MainFrame::UpdatePanels( wxCommandEvent& WXUNUSED(event) )
{   // Maybe this can be invoced by the Panorama::Changed() or
    // something like this. So no everytime update would be needed.
    DEBUG_TRACE("");
}

void MainFrame::OnTogglePreviewFrame(wxCommandEvent & e)
{
    DEBUG_TRACE("");
    if (preview_frame->IsIconized()) {
        preview_frame->Iconize(false);
    }
    preview_frame->Show();
    preview_frame->Raise();

	// we need to force an update since autoupdate fires
	// before the preview frame is shown
    wxCommandEvent dummy;
	preview_frame->OnUpdate(dummy);
}

void MainFrame::OnToggleGLPreviewFrame(wxCommandEvent & e)
{
    if(gl_preview_frame==NULL)
    {
        return;
    };
#if defined __WXMSW__ || defined __WXMAC__
    gl_preview_frame->InitPreviews();
#endif
    if (gl_preview_frame->IsIconized()) {
        gl_preview_frame->Iconize(false);
    }
    gl_preview_frame->Show();
#if defined __WXMSW__
    // on wxMSW Show() does not send OnShowEvent needed to update the
    // visibility state of the fast preview windows
    // so explicit calling this event handler
    wxShowEvent se;
    se.SetShow(true);
    gl_preview_frame->OnShowEvent(se);
#elif defined __WXGTK__
    gl_preview_frame->LoadOpenGLLayout();
#endif
    gl_preview_frame->Raise();
}

void MainFrame::OnShowCPFrame(wxCommandEvent & e)
{
    DEBUG_TRACE("");
    if (cp_frame) {
        if (cp_frame->IsIconized()) {
            cp_frame->Iconize(false);
        }
        cp_frame->Show();
        cp_frame->Raise();
    } else {
        cp_frame = new CPListFrame(this, pano);
        cp_frame->Show();
    }
}

void MainFrame::OnCPListFrameClosed()
{
    cp_frame = 0;
}

void MainFrame::OnOptimize(wxCommandEvent & e)
{
    DEBUG_TRACE("");
    wxCommandEvent dummy;
    opt_panel->OnOptimizeButton(dummy);
}

void MainFrame::OnPhotometricOptimize(wxCommandEvent & e)
{
    wxCommandEvent dummy;
    opt_photo_panel->OnOptimizeButton(dummy);
};

void MainFrame::OnDoStitch(wxCommandEvent & e)
{
    DEBUG_TRACE("");
    wxCommandEvent cmdEvt(wxEVT_COMMAND_BUTTON_CLICKED,XRCID("pano_button_stitch"));
    pano_panel->GetEventHandler()->AddPendingEvent(cmdEvt);
}

void MainFrame::OnMergeProject(wxCommandEvent & e)
{
    // get the global config object
    wxConfigBase* config = wxConfigBase::Get();

    wxString defaultdir = config->Read(wxT("/actualPath"),wxT(""));
    wxFileDialog dlg(wxTheApp->GetTopWindow(),
                     _("Open project file"),
                     defaultdir, wxT(""),
                     _("Project files (*.pto,*.ptp,*.pts,*.oto)|*.pto;*.ptp;*.pts;*.oto;|All files (*)|*"),
                     wxFD_OPEN, wxDefaultPosition);
    dlg.SetDirectory(defaultdir);
    if (dlg.ShowModal() == wxID_OK)
    {
        wxString filename = dlg.GetPath();
        wxFileName fname(filename);
        wxString path = fname.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
        if (fname.IsOk() && fname.FileExists())
        {
            wxBusyCursor wait;
            PanoramaMemento newPano;
            std::ifstream in((const char *)fname.GetFullPath().mb_str(HUGIN_CONV_FILENAME));
            int ptoversion=0;
            if (newPano.loadPTScript(in, ptoversion, (const char *)path.mb_str(HUGIN_CONV_FILENAME)))
            {
                Panorama new_pano;
                new_pano.setMemento(newPano);
                GlobalCmdHist::getInstance().addCommand(
                    new MergePanoCmd(pano, new_pano)
                );
                m_mruFiles.AddFileToHistory(fname.GetFullPath());
                // force update of preview window
                if ( !(preview_frame->IsIconized() ||(! preview_frame->IsShown()) ) )
                {
                    wxCommandEvent dummy;
                    preview_frame->OnUpdate(dummy);
                };
            }
            else
            {
                wxMessageBox(wxString::Format(_("Could not read project file %s."),fname.GetFullPath().c_str()),_("Error"),wxOK|wxICON_ERROR);
            };
        };
    }
}

void MainFrame::OnApplyTemplate(wxCommandEvent & e)
{
    // get the global config object
    wxConfigBase* config = wxConfigBase::Get();

    wxFileDialog dlg(wxTheApp->GetTopWindow(),
                     _("Choose template project"),
                     config->Read(wxT("/templatePath"),wxT("")), wxT(""),
                     _("Project files (*.pto,*.ptp,*.pts,*.oto)|*.pto;*.ptp;*.pts;*.oto;|All files (*)|*"),
                     wxFD_OPEN, wxDefaultPosition);
    dlg.SetDirectory(wxConfigBase::Get()->Read(wxT("/templatePath"),wxT("")));
    if (dlg.ShowModal() == wxID_OK) {
        wxString filename = dlg.GetPath();
        wxConfig::Get()->Write(wxT("/templatePath"), dlg.GetDirectory());  // remember for later

        std::ifstream file((const char *)filename.mb_str(HUGIN_CONV_FILENAME));

        GlobalCmdHist::getInstance().addCommand(
                new wxApplyTemplateCmd(pano, file));

    }
}

void MainFrame::OnOpenPTBatcher(wxCommandEvent & e)
{
#if defined __WXMAC__ && defined MAC_SELF_CONTAINED_BUNDLE
	// Original patch for OSX by Charlie Reiman dd. 18 June 2011
	// Slightly modified by HvdW. Errors in here are mine, not Charlie's. 
	FSRef appRef;
	FSRef actuallyLaunched;
	OSStatus err;
	FSRef documentArray[1]; // Don't really need an array if we only have 1 item
	LSLaunchFSRefSpec launchSpec;
	Boolean  isDir;
	
	err = LSFindApplicationForInfo(kLSUnknownCreator,
								   CFSTR("net.sourceforge.hugin.PTBatcherGUI"),
								   NULL,
								   &appRef,
								   NULL);
	if (err != noErr) {
		// error, can't find PTBatcherGUI
		wxMessageBox(wxString::Format(_("External program %s not found in the bundle, reverting to system path"), wxT("open")), _("Error"));
		// Possibly a silly attempt otherwise the previous would have worked as well, but just try it.
		wxExecute(_T("open -b net.sourceforge.hugin.PTBatcherGUI"));
	}
	else {
		wxExecute(_T("open -b net.sourceforge.hugin.PTBatcherGUI"));
	}	
#else
#ifdef __WINDOWS__
	wxString huginPath = getExePath(wxGetApp().argv[0])+wxFileName::GetPathSeparator();
#else
	wxString huginPath = _T("");	//we call the batch processor directly without path on linux
#endif
	wxExecute(huginPath+_T("PTBatcherGUI"));
#endif
}

void MainFrame::OnFineTuneAll(wxCommandEvent & e)
{
    DEBUG_TRACE("");
    // fine-tune all points

    CPVector cps = pano.getCtrlPoints();

    // create a map of all control points.
    std::set<unsigned int> unoptimized;
    for (unsigned int i=0; i < cps.size(); i++) {
        // create all control points.
        unoptimized.insert(i);
    }

    unsigned int nGood=0;
    unsigned int nBad=0;

    wxConfigBase *cfg = wxConfigBase::Get();
    bool rotatingFinetune = cfg->Read(wxT("/Finetune/RotationSearch"), HUGIN_FT_ROTATION_SEARCH) == 1;
    double startAngle=HUGIN_FT_ROTATION_START_ANGLE;
    cfg->Read(wxT("/Finetune/RotationStartAngle"),&startAngle,HUGIN_FT_ROTATION_START_ANGLE);
    startAngle=DEG_TO_RAD(startAngle);
    double stopAngle=HUGIN_FT_ROTATION_STOP_ANGLE;
    cfg->Read(wxT("/Finetune/RotationStopAngle"),&stopAngle,HUGIN_FT_ROTATION_STOP_ANGLE);
    stopAngle=DEG_TO_RAD(stopAngle);
    int nSteps = cfg->Read(wxT("/Finetune/RotationSteps"), HUGIN_FT_ROTATION_STEPS);

    double corrThresh=HUGIN_FT_CORR_THRESHOLD;
    cfg->Read(wxT("/Finetune/CorrThreshold"), &corrThresh, HUGIN_FT_CORR_THRESHOLD);
    double curvThresh = HUGIN_FT_CURV_THRESHOLD;
    wxConfigBase::Get()->Read(wxT("/Finetune/CurvThreshold"),&curvThresh,
                              HUGIN_FT_CURV_THRESHOLD);

    {
    ProgressReporterDialog progress(unoptimized.size(),_("Fine-tuning all points"),_("Fine-tuning"),wxTheApp->GetTopWindow());

    ImageCache & imgCache = ImageCache::getInstance();

    // do not process the control points in random order,
    // but walk from image to image, to reduce image reloading
    // in low mem situations.
    for (unsigned int imgNr = 0 ; imgNr < pano.getNrOfImages(); imgNr++) {
        std::set<unsigned int>::iterator it=unoptimized.begin();

        imgCache.softFlush();

        while (it != unoptimized.end()) {
            if (cps[*it].image1Nr == imgNr || cps[*it].image2Nr == imgNr) {
                progress.increaseProgress(1);
                if (cps[*it].mode == ControlPoint::X_Y) {
                    // finetune only normal points
                    DEBUG_DEBUG("fine tuning point: " << *it);
                    wxImage wxSearchImg;
                    ImageCache::EntryPtr searchImg = imgCache.getImage(
                         pano.getImage(cps[*it].image2Nr).getFilename());

                    ImageCache::EntryPtr templImg = imgCache.getImage(
                         pano.getImage(cps[*it].image1Nr).getFilename());


                    // load parameters
                    long templWidth = wxConfigBase::Get()->Read(
                        wxT("/Finetune/TemplateSize"),HUGIN_FT_TEMPLATE_SIZE);
                    long sWidth = templWidth + wxConfigBase::Get()->Read(
                        wxT("/Finetune/LocalSearchWidth"),HUGIN_FT_LOCAL_SEARCH_WIDTH);
                    vigra_ext::CorrelationResult res;
                    vigra::Diff2D roundP1(roundi(cps[*it].x1), roundi(cps[*it].y1));
                    vigra::Diff2D roundP2(roundi(cps[*it].x2), roundi(cps[*it].y2));

                    if (rotatingFinetune) {
                        res = vigra_ext::PointFineTuneRotSearch(
                            *(templImg->get8BitImage()),
                            roundP1,
                            templWidth,
                            *(searchImg->get8BitImage()),
                            roundP2,
                            sWidth,
                            startAngle, stopAngle, nSteps
                            );

                    } else {
                        res = vigra_ext::PointFineTune(
                            *(templImg->get8BitImage()),
                            roundP1,
                            templWidth,
                            *(searchImg->get8BitImage()),
                            roundP2,
                            sWidth
                            );

                    }
                    // invert curvature. we always assume its a maxima, the curvature there is negative
                    // however, we allow the user to specify a positive threshold, so we need to
                    // invert it
                    res.curv.x = - res.curv.x;
                    res.curv.y = - res.curv.y;

                    if (res.maxi < corrThresh ||res.curv.x < curvThresh || res.curv.y < curvThresh  )
                    {
                        // Bad correlation result.
                        nBad++;
                        if (res.maxi >= corrThresh) {
                            cps[*it].error = 0;
                        }
                        cps[*it].error = res.maxi;
                        DEBUG_DEBUG("low correlation: " << res.maxi << " curv: " << res.curv);
                    } else {
                        nGood++;
                        // only update if a good correlation was found
                        cps[*it].x1 = roundP1.x;
                        cps[*it].y1 = roundP1.y;
                        cps[*it].x2 = res.maxpos.x;
                        cps[*it].y2 = res.maxpos.y;
                        cps[*it].error = res.maxi;
                    }
                }
                unsigned int rm = *it;
                it++;
                unoptimized.erase(rm);
            } else {
                it++;
            }
        }
    }
    }
    wxString result;
    result.Printf(_("%d points fine-tuned, %d points not updated due to low correlation\n\nHint: The errors of the fine-tuned points have been set to the correlation coefficient\nProblematic points can be spotted (just after fine-tune, before optimizing)\nby an error <= %.3f.\nThe error of points without a well defined peak (typically in regions with uniform color)\nwill be set to 0\n\nUse the Control Point list (F3) to see all points of the current project\n"),
                  nGood, nBad, corrThresh);
    wxMessageBox(result, _("Fine-tune result"), wxOK);
    // set newly optimized points
    GlobalCmdHist::getInstance().addCommand(
        new UpdateCPsCmd(pano,cps,false)
        );
}

void MainFrame::OnRemoveCPinMasks(wxCommandEvent & e)
{
    if(pano.getCtrlPoints().size()<2)
        return;
    UIntSet cps=getCPinMasks(pano);
    if(cps.size()>0)
    {
        GlobalCmdHist::getInstance().addCommand(
                    new PT::RemoveCtrlPointsCmd(pano,cps)
                    );
        wxMessageBox(wxString::Format(_("Removed %d control points"), cps.size()),
                   _("Removing control points in masks"),wxOK|wxICON_INFORMATION);
    };
}

#ifdef HUGIN_HSI
void MainFrame::OnPythonScript(wxCommandEvent & e)
{
    wxString fname;
    wxFileDialog dlg(wxTheApp->GetTopWindow(),
            _("Select python script"),
            wxConfigBase::Get()->Read(wxT("/lensPath"),wxT("")), wxT(""),
            _("Python script (*.py)|*.py|All files (*.*)|*.*"),
            wxFD_OPEN, wxDefaultPosition);
    dlg.SetDirectory(wxConfigBase::Get()->Read(wxT("/pythonScriptPath"),wxT("")));

    if (dlg.ShowModal() == wxID_OK)
    {
        wxString filename = dlg.GetPath();
        wxConfig::Get()->Write(wxT("/pythonScriptPath"), dlg.GetDirectory());
        std::string scriptfile((const char *)filename.mb_str(HUGIN_CONV_FILENAME));
        GlobalCmdHist::getInstance().addCommand(
            new PythonScriptPanoCmd(pano,scriptfile)
            );
    }
}

void MainFrame::OnPlugin(wxCommandEvent & e)
{
    wxFileName file=m_plugins[e.GetId()];
    if(file.FileExists())
    {
        std::string scriptfile((const char *)file.GetFullPath().mb_str(HUGIN_CONV_FILENAME));
        GlobalCmdHist::getInstance().addCommand(
                                 new PythonScriptPanoCmd(pano,scriptfile)
                                 );
    }
    else
    {
        wxMessageBox(wxString::Format(wxT("Python-Script %s not found.\nStopping processing."),file.GetFullPath().c_str()),_("Warning"),wxOK|wxICON_INFORMATION);
    };
}

#endif

void MainFrame::OnUndo(wxCommandEvent & e)
{
    DEBUG_TRACE("OnUndo");
    if(GlobalCmdHist::getInstance().canUndo())
    {
        GlobalCmdHist::getInstance().undo();
    }
    else
    {
        wxBell();
    };
}

void MainFrame::OnRedo(wxCommandEvent & e)
{
    DEBUG_TRACE("OnRedo");
    if(GlobalCmdHist::getInstance().canRedo())
    {
        GlobalCmdHist::getInstance().redo();
    };
}

void MainFrame::ShowCtrlPoint(unsigned int cpNr)
{
    DEBUG_DEBUG("Showing control point " << cpNr);
    m_notebook->SetSelection(2);
    cpe->ShowControlPoint(cpNr);
}

void MainFrame::ShowCtrlPointEditor(unsigned int img1, unsigned int img2)
{
    if(!IsShown())
    {
        Show();
        Raise();
    };
    m_notebook->SetSelection(2);
    cpe->setLeftImage(img1);
    cpe->setRightImage(img2);
}

void MainFrame::ShowMaskEditor(size_t imgNr)
{
    if(!IsShown())
    {
        Show();
        Raise();
    };
    m_notebook->SetSelection(1);
    mask_panel->setImage(imgNr, true);
};

void MainFrame::ShowStitcherTab()
{
    ///@todo Stop using magic numbers for the tabs.
    if(m_show_opt_panel && m_show_opt_photo_panel)
    {
        m_notebook->SetSelection(5);
    }
    else
    {
        if(m_show_opt_panel || m_show_opt_photo_panel)
        {
            m_notebook->SetSelection(4);
        }
        else
        {
            m_notebook->SetSelection(3);
        };
    };
}

/** update the display */
void MainFrame::updateProgressDisplay()
{
    wxString msg;
    // build the message:
    for (std::vector<AppBase::ProgressTask>::reverse_iterator it = tasks.rbegin();
                 it != tasks.rend(); ++it)
    {
        wxString cMsg;
        if (it->getProgress() > 0) {
            cMsg.Printf(wxT("%s %s [%3.0f%%]"),
                        wxString(it->getShortMessage().c_str(), wxConvLocal).c_str(),
                        wxString(it->getMessage().c_str(), wxConvLocal).c_str(),
                        100 * it->getProgress());
        } else {
            cMsg.Printf(wxT("%s %s"),wxString(it->getShortMessage().c_str(), wxConvLocal).c_str(),
                        wxString(it->getMessage().c_str(), wxConvLocal).c_str());
        }
        // append to main message
        if (it == tasks.rbegin()) {
            msg = cMsg;
        } else {
            msg.Append(wxT(" | "));
            msg.Append(cMsg);
        }
    }
    wxStatusBar *m_statbar = GetStatusBar();
    DEBUG_TRACE("Statusmb : " << msg.mb_str(wxConvLocal));
    m_statbar->SetStatusText(msg,0);

#ifdef __WXMSW__
    UpdateWindow(NULL);
#else
    // This is a bad call.. we just want to repaint the window, instead we will
    // process user events as well :( Unfortunately, there is not portable workaround...
    //wxYield();
#endif
}

void MainFrame::enableTools(bool option)
{
    wxToolBar* theToolBar = GetToolBar();
    theToolBar->EnableTool(XRCID("action_optimize"), option);
    theToolBar->EnableTool(XRCID("ID_SHOW_PREVIEW_FRAME"), option);
    //theToolBar->EnableTool(XRCID("ID_SHOW_GL_PREVIEW_FRAME"), option);
    wxMenuBar* theMenuBar = GetMenuBar();
    theMenuBar->Enable(XRCID("action_optimize"), option);
    theMenuBar->Enable(XRCID("action_finetune_all_cp"), option);
    theMenuBar->Enable(XRCID("ID_SHOW_PREVIEW_FRAME"), option);
    //theMenuBar->Enable(XRCID("ID_SHOW_GL_PREVIEW_FRAME"), option);
}


void MainFrame::OnSize(wxSizeEvent &e)
{
    wxSize sz = this->GetSize();
    wxSize csz = this->GetClientSize();
    wxSize vsz = this->GetVirtualSize();
    DEBUG_TRACE(" size:" << sz.x << "," << sz.y <<
                " client: "<< csz.x << "," << csz.y <<
                " virtual: "<< vsz.x << "," << vsz.y);

    Layout();
    e.Skip();
}

CPDetectorSetting& MainFrame::GetDefaultSetting()
{
    return images_panel->GetDefaultSetting();
};

const wxString & MainFrame::GetXRCPath()
{
     return huginApp::Get()->GetXRCPath();
};

const wxString & MainFrame::GetDataPath()
{
    return wxGetApp().GetDataPath();
};

/// hack.. kind of a pseudo singleton...
MainFrame * MainFrame::Get()
{
    if (m_this) {
        return m_this;
    } else {
        DEBUG_FATAL("MainFrame not yet created");
        DEBUG_ASSERT(m_this);
        return 0;
    }
}

void MainFrame::resetProgress(double max)
{
    m_progressMax = max;
    m_progress = 0;
    m_progressMsg = wxT("");
}

bool MainFrame::increaseProgress(double delta)
{
    m_progress += delta;

    // build the message:
    int percentage = (int) floor(m_progress/m_progressMax*100);
    if (percentage > 100) percentage = 100;

    return displayProgress();
}

bool MainFrame::increaseProgress(double delta, const std::string & msg)
{
    m_progress += delta;
    m_progressMsg = wxString(msg.c_str(), wxConvLocal);

    return displayProgress();
}


void MainFrame::setMessage(const std::string & msg)
{
    m_progressMsg = wxString(msg.c_str(), wxConvLocal);
}

bool MainFrame::displayProgress()
{
    // build the message:
    int percentage = (int) floor(m_progress/m_progressMax*100);
    if (percentage > 100) percentage = 100;

    wxStatusBar *statbar = GetStatusBar();
    statbar->SetStatusText(wxString::Format(wxT("%s: %d%%"),m_progressMsg.c_str(), percentage),0);
#ifdef __WXMSW__
    UpdateWindow(NULL);
#else
    // This is a bad call.. we just want to repaint the window, instead we will
    // process user events as well :( Unfortunately, there is not portable workaround...
    //wxYield();
#endif
    return true;
}

wxString MainFrame::getProjectName()
{
    return m_filename;
}

bool getLensDataFromUser(wxWindow * parent, SrcPanoImage & srcImg,
                         double & focalLength, double & cropFactor)
{
    // display lens dialog
    HFOVDialog dlg(parent, srcImg, focalLength, cropFactor);
    dlg.CenterOnParent();
    int ret = dlg.ShowModal();
    if (ret == wxID_OK) {
        // assume a cancel dialog.
        srcImg = dlg.GetSrcImage();
        if (dlg.GetCropFactor() <= 0) {
            srcImg.setExifCropFactor(1);
        }
        return true;
    } else {
        return false;
    }
}

void MainFrame::OnMRUFiles(wxCommandEvent &e)
{
    size_t index = e.GetId() - wxID_FILE1;
    wxString f(m_mruFiles.GetHistoryFile(index));
    if (!f.empty())
    {
        wxFileName fn(f);
        if(fn.FileExists())
            LoadProjectFile(f);
        else
        {
            m_mruFiles.RemoveFileFromHistory(index);
            wxMessageBox(wxString::Format(_("File \"%s\" not found.\nMaybe file was renamed, moved or deleted."),f.c_str()),
                _("Error!"),wxOK | wxICON_INFORMATION );
        };
    };
}

void MainFrame::OnFullScreen(wxCommandEvent & e)
{
    ShowFullScreen(!IsFullScreen(), wxFULLSCREEN_NOBORDER | wxFULLSCREEN_NOCAPTION);
#ifdef __WXGTK__
    //workaround a wxGTK bug that also the toolbar is hidden, but not requested to hide
    GetToolBar()->Show(true);
#endif
};

struct celeste::svm_model* MainFrame::GetSVMModel()
{
    if(svmModel==NULL)
    {
        // determine file name of SVM model file
        // get XRC path from application
        wxString wxstrModelFileName = huginApp::Get()->GetDataPath() + wxT(HUGIN_CELESTE_MODEL);
        // convert wxString to string
        string strModelFileName(wxstrModelFileName.mb_str(HUGIN_CONV_FILENAME));

        // SVM model file
        if (! wxFile::Exists(wxstrModelFileName) ) {
            wxMessageBox(wxString::Format(_("Celeste model expected in %s not found, Hugin needs to be properly installed."),wxstrModelFileName.c_str()), _("Fatal Error"));
            return NULL;
        }
        if(!celeste::loadSVMmodel(svmModel,strModelFileName))
        {
            wxMessageBox(wxString::Format(_("Could not load Celeste model file %s"),wxstrModelFileName.c_str()),_("Error"));
            svmModel=NULL;
        };
    }
    return svmModel;
};

GLPreviewFrame * MainFrame::getGLPreview()
{
    return gl_preview_frame;
}

void MainFrame::SetGuiLevel(GuiLevel newLevel)
{
    if(gl_preview_frame==NULL && newLevel==GUI_SIMPLE)
    {
        SetGuiLevel(GUI_ADVANCED);
        return;
    };
    if(m_guiLevel==GUI_EXPERT && newLevel!=GUI_EXPERT && pano.getOptimizerSwitch()==0)
    {
        bool needsUpdateOptimizerVar=false;
        OptimizeVector optVec=pano.getOptimizeVector();
        for(size_t i=0; i<optVec.size(); i++)
        {
            bool hasTrX=optVec[i].erase("TrX")>0;
            bool hasTrY=optVec[i].erase("TrY")>0;
            bool hasTrZ=optVec[i].erase("TrZ")>0;
            bool hasg=optVec[i].erase("g")>0;
            bool hast=optVec[i].erase("t")>0;
            needsUpdateOptimizerVar=needsUpdateOptimizerVar || hasTrX || hasTrY || hasTrZ || hasg || hast;
        };
        if(needsUpdateOptimizerVar)
        {
            GlobalCmdHist::getInstance().addCommand(
                new PT::UpdateOptimizeVectorCmd(pano, optVec)
            );
        };
    };
    if(newLevel==GUI_SIMPLE && pano.getPhotometricOptimizerSwitch()==0)
    {
        bool needsUpdateOptimizerVar=false;
        OptimizeVector optVec=pano.getOptimizeVector();
        for(size_t i=0; i<optVec.size(); i++)
        {
            bool hasVx=optVec[i].erase("Vx")>0;
            bool hasVy=optVec[i].erase("Vy")>0;
            needsUpdateOptimizerVar=needsUpdateOptimizerVar || hasVx || hasVy;
        };
        if(needsUpdateOptimizerVar)
        {
            GlobalCmdHist::getInstance().addCommand(
                new PT::UpdateOptimizeVectorCmd(pano, optVec)
            );
        };
    };
    m_guiLevel=newLevel;
    images_panel->SetGuiLevel(m_guiLevel);
    opt_panel->SetGuiLevel(m_guiLevel);
    opt_photo_panel->SetGuiLevel(m_guiLevel);
    pano_panel->SetGuiLevel(m_guiLevel);
    if(gl_preview_frame)
    {
        gl_preview_frame->SetGuiLevel(m_guiLevel);
    };
    switch(m_guiLevel)
    {
        case GUI_SIMPLE:
            GetMenuBar()->FindItem(XRCID("action_gui_simple"))->Check();
            break;
        case GUI_ADVANCED:
            GetMenuBar()->FindItem(XRCID("action_gui_advanced"))->Check();
            break;
        case GUI_EXPERT:
            GetMenuBar()->FindItem(XRCID("action_gui_expert"))->Check();
            break;
    };
    if(m_guiLevel==GUI_SIMPLE)
    {
        if(!gl_preview_frame->IsShown())
        {
            wxCommandEvent dummy;
            OnToggleGLPreviewFrame(dummy);
        };
        wxGetApp().SetTopWindow(gl_preview_frame);
        GetMenuBar()->Replace(0, m_menu_file_simple, _("&File"));
        if(m_filename.IsEmpty())
        {
            gl_preview_frame->SetTitle(_("Hugin - Panorama Stitcher"));
            SetTitle(_("Panorama editor"));
        }
        else
        {
            wxFileName scriptName = m_filename;
            gl_preview_frame->SetTitle(scriptName.GetName() + wxT(".") + scriptName.GetExt() + wxT(" - ") + _("Hugin - Panorama Stitcher"));
            SetTitle(scriptName.GetName() + wxT(".") + scriptName.GetExt() + wxT(" - ") + _("Panorama editor"));
        };
        Hide();
    }
    else
    {
        wxGetApp().SetTopWindow(this);
        GetMenuBar()->Replace(0, m_menu_file_advanced, _("&File"));
        if(m_filename.IsEmpty())
        {
            SetTitle(_("Hugin - Panorama Stitcher"));
        }
        else
        {
            wxFileName scriptName = m_filename;
            SetTitle(scriptName.GetName() + wxT(".") + scriptName.GetExt() + wxT(" - ") + _("Hugin - Panorama Stitcher"));
        };
        if(!IsShown())
        {
            Show();
        };
    };
};

void MainFrame::OnSetGuiSimple(wxCommandEvent & e)
{
    GuiLevel reqGuiLevel=GetMinimumGuiLevel(pano);
    if(reqGuiLevel<=GUI_SIMPLE)
    {
        SetGuiLevel(GUI_SIMPLE);
    }
    else
    {
        if(reqGuiLevel==GUI_ADVANCED)
        {
            wxMessageBox(_("Can't switch to simple interface. The project is using stacks and/or vignetting center shift.\nThese features are not supported in simple interface."),
#ifdef __WXMSW__
                         wxT("Hugin"),
#else
                         wxT(""),
#endif
                         wxOK | wxICON_INFORMATION);
        }
        else
        {
            wxMessageBox(_("Can't switch to simple interface. The project is using translation or shear parameters.\nThese parameters are not supported in simple interface."),
#ifdef __WXMSW__
                         wxT("Hugin"),
#else
                         wxT(""),
#endif
                         wxOK | wxICON_INFORMATION);
        }
        SetGuiLevel(m_guiLevel);
    };
};

void MainFrame::OnSetGuiAdvanced(wxCommandEvent & e)
{
    GuiLevel reqGuiLevel=GetMinimumGuiLevel(pano);
    if(reqGuiLevel<=GUI_ADVANCED)
    {
        SetGuiLevel(GUI_ADVANCED);
    }
    else
    {
        wxMessageBox(_("Can't switch to advanced interface. The project is using translation or shear parameters.\nThese parameters are not supported in advanced interface."),
#ifdef __WXMSW__
                     wxT("Hugin"),
#else
                     wxT(""),
#endif
                     wxOK | wxICON_INFORMATION);
        SetGuiLevel(GUI_EXPERT);
    };
};

void MainFrame::OnSetGuiExpert(wxCommandEvent & e)
{
    SetGuiLevel(GUI_EXPERT);
};

void MainFrame::DisableOpenGLTools()
{
    GetMenuBar()->Enable(XRCID("ID_SHOW_GL_PREVIEW_FRAME"), false);
    GetMenuBar()->Enable(XRCID("action_gui_simple"), false);
    GetToolBar()->EnableTool(XRCID("ID_SHOW_GL_PREVIEW_FRAME"), false); 
};

void MainFrame::RunAssistant(wxWindow* mainWin)
{
    if (pano.getNrOfImages() < 2)
    {
        wxMessageBox(_("At least two images are required.\nPlease add more images."),_("Error"), wxOK, mainWin);
        return;
    }

    //generate list of all necessary programs with full path
    wxString bindir = huginApp::Get()->GetUtilsBinDir();
    wxConfigBase* config=wxConfigBase::Get();
    AssistantPrograms progs = getAssistantProgramsConfig(bindir, config);

    //read main settings
    bool runCeleste=config->Read(wxT("/Celeste/Auto"), HUGIN_CELESTE_AUTO)!=0;
    double celesteThreshold;
    config->Read(wxT("/Celeste/Threshold"), &celesteThreshold, HUGIN_CELESTE_THRESHOLD);
    bool celesteSmall=config->Read(wxT("/Celeste/Filter"), HUGIN_CELESTE_FILTER)==0;
    bool runLinefind=config->Read(wxT("/Assistant/Linefind"), HUGIN_ASS_LINEFIND)!=0;
    bool runCPClean=config->Read(wxT("/Assistant/AutoCPClean"), HUGIN_ASS_AUTO_CPCLEAN)!=0;
    double scale;
    config->Read(wxT("/Assistant/panoDownsizeFactor"), &scale, HUGIN_ASS_PANO_DOWNSIZE_FACTOR);
    int scalei=roundi(scale*100);

    //save project into temp directory
    wxString tempDir= config->Read(wxT("tempDir"),wxT(""));
    if(!tempDir.IsEmpty())
    {
        if(tempDir.Last()!=wxFileName::GetPathSeparator())
        {
            tempDir.Append(wxFileName::GetPathSeparator());
        }
    };
    wxString scriptName=wxFileName::CreateTempFileName(tempDir+wxT("ha"));
    std::ofstream script(scriptName.mb_str(HUGIN_CONV_FILENAME));
    script.exceptions ( std::ofstream::eofbit | std::ofstream::failbit | std::ofstream::badbit );
    PT::UIntSet all;
    fill_set(all, 0, pano.getNrOfImages()-1);
    pano.printPanoramaScript(script, pano.getOptimizeVector(), pano.getOptions(), all, false);
    script.close();
    //generate makefile
    wxString makefileName=wxFileName::CreateTempFileName(tempDir+wxT("ham"));
    std::ofstream makefile(makefileName.mb_str(HUGIN_CONV_FILENAME));
    makefile.exceptions( std::ofstream::eofbit | std::ofstream::failbit | std::ofstream::badbit );
    std::string scriptString(scriptName.mb_str(HUGIN_CONV_FILENAME));
    HuginBase::AssistantMakefilelibExport::createMakefile(pano,progs,runLinefind,runCeleste,celesteThreshold,celesteSmall,
        runCPClean,scale,makefile,scriptString);
    makefile.close();

    //execute makefile
    wxString args = wxT("-f ") + wxQuoteFilename(makefileName) + wxT(" all");
    int ret=MyExecuteCommandOnDialog(getGNUMakeCmd(args), wxEmptyString, mainWin, _("Running assistant"), true);

    //read back panofile
    GlobalCmdHist::getInstance().addCommand(new wxLoadPTProjectCmd(pano,
        (const char *)scriptName.mb_str(HUGIN_CONV_FILENAME), "", ret==0, false));

    //delete temporary files
    wxRemoveFile(scriptName);
    wxRemoveFile(makefileName);
    //if return value is non-zero, an error occured in assistant makefile
    if(ret!=0)
    {
        //check for unconnected images
        CPGraph graph;
        createCPGraph(pano, graph);
        CPComponents comps;
        int n = findCPComponents(graph, comps);
        if(n > 1)
        {
            // switch to images panel.
            unsigned i1 = *(comps[0].rbegin());
            unsigned i2 = *(comps[1].begin());
            ShowCtrlPointEditor( i1, i2);
            // display message box with 
            wxMessageBox(wxString::Format(_("Warning %d unconnected image groups found:"), n) + Components2Str(comps) + wxT("\n")
                + _("Please create control points between unconnected images using the Control Points tab.\n\nAfter adding the points, press the \"Align\" button again"),_("Error"), wxOK , mainWin);
            return;
        };
        wxMessageBox(_("The assistant did not complete successfully. Please check the resulting project file."),
                     _("Warning"),wxOK | wxICON_INFORMATION, mainWin); 
    };
};

void MainFrame::OnRunAssistant(wxCommandEvent & e)
{
    RunAssistant(this);
};

void MainFrame::OnSendToAssistantQueue(wxCommandEvent &e)
{
    wxCommandEvent dummy;
    OnSaveProject(dummy);
    wxString projectFile = getProjectName();
    if(wxFileName::FileExists(projectFile))
    {
#if defined __WXMAC__ && defined MAC_SELF_CONTAINED_BUNDLE
        // Original patch for OSX by Charlie Reiman dd. 18 June 2011
        // Slightly modified by HvdW. Errors in here are mine, not Charlie's. 
        FSRef appRef;
        FSRef actuallyLaunched;
        OSStatus err;
        FSRef documentArray[1]; // Don't really need an array if we only have 1 item
        LSLaunchFSRefSpec launchSpec;
        Boolean  isDir;

        err = LSFindApplicationForInfo(kLSUnknownCreator,
                                       CFSTR("net.sourceforge.hugin.PTBatcherGUI"),
                                       NULL,
                                       &appRef,
                                       NULL);
        if (err != noErr)
        {
            // error, can't find PTBatcherGUI
            wxMessageBox(wxString::Format(_("External program %s not found in the bundle, reverting to system path"), wxT("open")), _("Error"));
            // Possibly a silly attempt otherwise the previous would have worked as well, but just try it.
            wxExecute(_T("open -b net.sourceforge.hugin.PTBatcherGUI ")+wxQuoteFilename(projectFile));
            return;
        }

        wxCharBuffer projectFilebuffer=projectFile.ToUTF8();
        // Point to document
        err = FSPathMakeRef((unsigned char*) projectFilebuffer.data(), &documentArray[0], &isDir);
        if (err != noErr || isDir)
        {
            // Something went wrong.
            wxMessageBox(wxString::Format(_("Project file not found"), wxT("open")), _("Error"));
            return;
        }
        launchSpec.appRef = &appRef;
        launchSpec.numDocs = sizeof(documentArray)/sizeof(documentArray[0]);
        launchSpec.itemRefs = documentArray;
        launchSpec.passThruParams = NULL;
        launchSpec.launchFlags = kLSLaunchDontAddToRecents + kLSLaunchDontSwitch;
        launchSpec.asyncRefCon = NULL;

        err = LSOpenFromRefSpec(&launchSpec, &actuallyLaunched);
        if (err != noErr && err != kLSLaunchInProgressErr)
        {  
            // Should be ok if it's in progress... I think. 
            // Launch failed.
            wxMessageBox(wxString::Format(_("Can't launch PTBatcherGui"), wxT("open")), _("Error"));
            return;
        }

        // Should verify that actuallyLaunched and appRef are the same.
        if (FSCompareFSRefs(&appRef, &actuallyLaunched) != noErr)
        {
            // error, lauched the wrong thing.
            wxMessageBox(wxString::Format(_("Launched incorrect programme"), wxT("open")), _("Error"));
            return;
        }
#else
#ifdef __WINDOWS__
        wxString huginPath = getExePath(wxGetApp().argv[0])+wxFileName::GetPathSeparator(); 
#else
        wxString huginPath = _T("");    //we call the batch processor directly without path on linux
#endif    
        wxExecute(huginPath+wxT("PTBatcherGUI -a ")+wxQuoteFilename(projectFile));
#endif
    }
};

MainFrame * MainFrame::m_this = 0;
