// -*- c-basic-offset: 4 -*-

/** @file hugin_stitch_project.cpp
 *
 *  @brief Stitch a pto project file, with GUI output etc.
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

#include <hugin_config.h>
#include <hugin_version.h>
#include "panoinc_WX.h"
#include "panoinc.h"

#include <wx/wfstream.h>


#include <fstream>
#include <sstream>
#include "PT/Panorama.h"
#include "PT/utils.h"
#include "base_wx/RunStitchPanel.h"
#include "base_wx/huginConfig.h"
#include "base_wx/MyProgressDialog.h"
#include "base_wx/MyExternalCmdExecDialog.h"
#include "base_wx/platform.h"
#include "base_wx/wxPlatform.h"

#include <tiffio.h>

// somewhere SetDesc gets defined.. this breaks wx/cmdline.h on OSX
#ifdef SetDesc
#undef SetDesc
#endif

#include <wx/cmdline.h>

using namespace vigra;
using namespace PT;
using namespace std;
using namespace hugin_utils;


class RunStitchFrame: public wxFrame
{
public:
    RunStitchFrame(wxWindow * parent, const wxString& title, const wxPoint& pos, const wxSize& size);

    bool StitchProject(wxString scriptFile, wxString outname,
                       HuginBase::PanoramaMakefilelibExport::PTPrograms progs,
                       bool doDeleteOnExit);

    void OnQuit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
    /** sets, if existing output file should be automatic overwritten */
    void SetOverwrite(bool doOverwrite);

private:

    bool m_isStitching;
    wxString m_scriptFile;
    bool m_deleteOnExit;

    void OnProcessTerminate(wxProcessEvent & event);
    void OnCancel(wxCommandEvent & event);

    RunStitchPanel * m_stitchPanel;

    DECLARE_EVENT_TABLE()
};

// event ID's for RunStitchPanel
enum
{
    ID_Quit = 1,
    ID_About   
};

BEGIN_EVENT_TABLE(RunStitchFrame, wxFrame)
    EVT_MENU(ID_Quit,  RunStitchFrame::OnQuit)
    EVT_MENU(ID_About, RunStitchFrame::OnAbout)
    EVT_BUTTON(wxID_CANCEL, RunStitchFrame::OnCancel)
    EVT_END_PROCESS(-1, RunStitchFrame::OnProcessTerminate)
END_EVENT_TABLE()

RunStitchFrame::RunStitchFrame(wxWindow * parent, const wxString& title, const wxPoint& pos, const wxSize& size)
    : wxFrame(parent, -1, title, pos, size), m_isStitching(false)
{
    /*
    wxMenu *menuFile = new wxMenu;

    menuFile->AppendSeparator();
    menuFile->Append( ID_Quit, _("E&xit") );

    wxMenu *menuHelp = new wxMenu;
    menuHelp->Append( ID_About, _("&About...") );

    wxMenuBar *menuBar = new wxMenuBar;
    menuBar->Append( menuFile, _("&File") );
    menuBar->Append( menuHelp, _("&Help") );
    SetMenuBar( menuBar );
    */

    wxBoxSizer * topsizer = new wxBoxSizer( wxVERTICAL );
    m_stitchPanel = new RunStitchPanel(this);

    topsizer->Add(m_stitchPanel, 1, wxEXPAND | wxALL, 2);

    topsizer->Add( new wxButton(this, wxID_CANCEL, _("Cancel")),
                   0, wxALL | wxALIGN_RIGHT, 10);

#ifdef __WXMSW__
    // wxFrame does have a strange background color on Windows..
    this->SetBackgroundColour(m_stitchPanel->GetBackgroundColour());
#endif

    SetSizer( topsizer );
//    topsizer->SetSizeHints( this );   // set size hints to honour minimum size
    m_deleteOnExit=false;
}

void RunStitchFrame::OnAbout(wxCommandEvent& WXUNUSED(event))
{
    wxMessageBox( wxString::Format(_("HuginStitchProject. Application to stitch hugin project files.\n Version: %s"), wxT(DISPLAY_VERSION)),
                  wxT("About hugin_stitch_project"),
                  wxOK | wxICON_INFORMATION );
}
void RunStitchFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
    DEBUG_TRACE("");
    if (m_isStitching) {
        m_stitchPanel->CancelStitch();
        m_isStitching = false;
    }
    Close();
}

void RunStitchFrame::OnCancel(wxCommandEvent& WXUNUSED(event))
{
    DEBUG_TRACE("");
    if (m_isStitching) {
        m_stitchPanel->CancelStitch();
        m_isStitching = false;
    } else {
        Close();
    }
}

void RunStitchFrame::OnProcessTerminate(wxProcessEvent & event)
{
    if (! m_isStitching) {
        // canceled stitch
        // TODO: Cleanup files?
        Close();
    } else {
        m_isStitching = false;
        if (event.GetExitCode() != 0)
        {
            if(wxMessageBox(_("Error during stitching\nPlease report the complete text to the bug tracker on https://bugs.launchpad.net/hugin.\n\nDo you want to save the log file?"),
                _("Error during stitching"), wxICON_ERROR | wxYES_NO )==wxYES)
            {
                wxString defaultdir = wxConfigBase::Get()->Read(wxT("/actualPath"),wxT(""));
                wxFileDialog dlg(this,
                         _("Specify log file"),
                         defaultdir, wxT(""),
                         _("Log files (*.log)|*.log|All files (*)|*"),
                         wxFD_SAVE | wxFD_OVERWRITE_PROMPT, wxDefaultPosition);
                dlg.SetDirectory(wxConfigBase::Get()->Read(wxT("/actualPath"),wxT("")));
                if (dlg.ShowModal() == wxID_OK)
                {
                    wxConfig::Get()->Write(wxT("/actualPath"), dlg.GetDirectory());  // remember for later
                    m_stitchPanel->SaveLog(dlg.GetPath());
                };
            }
        } else {
            if(m_deleteOnExit)
            {
                wxRemoveFile(m_scriptFile);
            };
            Close();
        }
    }
}

bool RunStitchFrame::StitchProject(wxString scriptFile, wxString outname,
                                   HuginBase::PanoramaMakefilelibExport::PTPrograms progs, bool doDeleteOnExit)
{
    if (! m_stitchPanel->StitchProject(scriptFile, outname, progs)) {
        return false;
    }
    m_isStitching = true;
    m_scriptFile=scriptFile;
    m_deleteOnExit=doDeleteOnExit;
    return true;
}

void RunStitchFrame::SetOverwrite(bool doOverwrite)
{
    m_stitchPanel->SetOverwrite(doOverwrite);
};

// **********************************************************************


/** The application class for hugin_stitch_project
 *
 *  it contains the main frame.
 */
class stitchApp : public wxApp
{
public:

    /** ctor.
     */
    stitchApp();

    /** dtor.
     */
    virtual ~stitchApp();

    /** pseudo constructor. with the ability to fail gracefully.
     */
    virtual bool OnInit();

    /** just for testing purposes */
    virtual int OnExit();
    
#ifdef __WXMAC__
    /** the wx calls this method when the app gets "Open file" AppleEvent */
    void MacOpenFile(const wxString &fileName);
#endif

private:
    wxLocale m_locale;
#ifdef __WXMAC__
    wxString m_macFileNameToOpenOnStart;
#endif
};

stitchApp::stitchApp()
{
    // suppress tiff warnings
    TIFFSetWarningHandler(0);

    DEBUG_TRACE("ctor");
}

stitchApp::~stitchApp()
{
    DEBUG_TRACE("dtor");
    DEBUG_TRACE("dtor end");
}

bool stitchApp::OnInit()
{
    // Required to access the preferences of hugin
    SetAppName(wxT("hugin"));

#if defined __WXMSW__
    int localeID = wxConfigBase::Get()->Read(wxT("language"), (long) wxLANGUAGE_DEFAULT);
    m_locale.Init(localeID);
#else
    m_locale.Init(wxLANGUAGE_DEFAULT);
#endif

    // setup the environment for the different operating systems
#if defined __WXMSW__
    wxString huginExeDir = getExePath(argv[0]);

    wxString huginRoot;
    wxFileName::SplitPath(huginExeDir, &huginRoot, NULL, NULL);
	
	// locale setup
    m_locale.AddCatalogLookupPathPrefix(huginRoot + wxT("/share/locale"));

    PTPrograms progs = getPTProgramsConfig(huginExeDir, wxConfigBase::Get());
#else
    // add the locale directory specified during configure
    m_locale.AddCatalogLookupPathPrefix(wxT(INSTALL_LOCALE_DIR));
    PTPrograms progs = getPTProgramsConfig(wxT(""), wxConfigBase::Get());
#endif

#if defined __WXMAC__ && defined MAC_SELF_CONTAINED_BUNDLE
    {
        wxString thePath = MacGetPathToBundledResourceFile(CFSTR("locale"));
        if(thePath != wxT(""))
            m_locale.AddCatalogLookupPathPrefix(thePath);
        else {
            wxMessageBox(_("Translations not found in bundle"), _("Fatal Error"));
            return false;
        }
    }
#endif
    
    // set the name of locale recource to look for
    m_locale.AddCatalog(wxT("hugin"));

    // parse arguments
    static const wxCmdLineEntryDesc cmdLineDesc[] =
    {
        //On wxWidgets 2.9, wide characters don't work here.
        //On previous versions, the wxT macro is required for unicode builds.
#if wxCHECK_VERSION(2,9,0)
      { wxCMD_LINE_SWITCH, "h", "help", "show this help message",
        wxCMD_LINE_VAL_NONE, wxCMD_LINE_OPTION_HELP },
      { wxCMD_LINE_OPTION, "o", "output",  "output prefix" },
      { wxCMD_LINE_SWITCH, "d", "delete",  "delete pto file after stitching" },
      { wxCMD_LINE_SWITCH, "w", "overwrite", "overwrite existing files" },
      { wxCMD_LINE_PARAM,  NULL, NULL, "<project>",
        wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL },
      { wxCMD_LINE_NONE }
#else 
      { wxCMD_LINE_SWITCH, wxT("h"), wxT("help"), wxT("show this help message"),
        wxCMD_LINE_VAL_NONE, wxCMD_LINE_OPTION_HELP },
      { wxCMD_LINE_OPTION, wxT("o"), wxT("output"),  wxT("output prefix") },
      { wxCMD_LINE_SWITCH, wxT("d"), wxT("delete"),  wxT("delete pto file after stitching") },
      { wxCMD_LINE_SWITCH, wxT("w"), wxT("overwrite"), wxT("overwrite existing files") },
      { wxCMD_LINE_PARAM,  NULL, NULL, wxT("<project>"),
        wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL },
      { wxCMD_LINE_NONE }
#endif 
    };

    wxCmdLineParser parser(cmdLineDesc, argc, argv);

    switch ( parser.Parse() ) {
      case -1: // -h or --help was given, and help displayed so exit
	return false;
	break;
      case 0:  // all is well
        break;
      default:
        wxLogError(_("Syntax error in parameters detected, aborting."));
	return false;
        break;
    }

    wxString scriptFile;
#ifdef __WXMAC__
    m_macFileNameToOpenOnStart = wxT("");
    wxYield();
    scriptFile = m_macFileNameToOpenOnStart;
    
    // bring myself front (for being called from command line)
    {
        ProcessSerialNumber selfPSN;
        OSErr err = GetCurrentProcess(&selfPSN);
        if (err == noErr)
        {
            SetFrontProcess(&selfPSN);
        }
    }
#endif
    
    if( parser.GetParamCount() == 0 && wxIsEmpty(scriptFile)) 
    {
        wxString defaultdir = wxConfigBase::Get()->Read(wxT("/actualPath"),wxT(""));
        wxFileDialog dlg(0,
                         _("Specify project source project file"),
                         defaultdir, wxT(""),
                         _("Project files (*.pto)|*.pto|All files (*)|*"),
                         wxFD_OPEN, wxDefaultPosition);

        dlg.SetDirectory(wxConfigBase::Get()->Read(wxT("/actualPath"),wxT("")));
        if (dlg.ShowModal() == wxID_OK) {
            wxConfig::Get()->Write(wxT("/actualPath"), dlg.GetDirectory());  // remember for later
            scriptFile = dlg.GetPath();
        } else { // bail
            return false;
        }
    } else if(wxIsEmpty(scriptFile)) {
        scriptFile = parser.GetParam(0);
        cout << "********************* script file: " << (const char *)scriptFile.mb_str(wxConvLocal) << endl;
        if (! wxIsAbsolutePath(scriptFile)) {
            scriptFile = wxGetCwd() + wxFileName::GetPathSeparator() + scriptFile;
        }
    }

    cout << "input file is " << (const char *)scriptFile.mb_str(wxConvLocal) << endl;

    wxString outname;

    if ( !parser.Found(wxT("o"), &outname) ) {
        // ask for output.
        wxFileDialog dlg(0,_("Specify output prefix"),
                         wxConfigBase::Get()->Read(wxT("/actualPath"),wxT("")),
                         wxT(""), wxT(""),
                         wxFD_SAVE, wxDefaultPosition);
        dlg.SetDirectory(wxConfigBase::Get()->Read(wxT("/actualPath"),wxT("")));
        if (dlg.ShowModal() == wxID_OK) {
            while(containsInvalidCharacters(dlg.GetPath()))
            {
                wxMessageBox(wxString::Format(_("The given filename contains one of the following invalid characters: %s\nHugin can not work with this filename. Please enter a valid filename."),getInvalidCharacters().c_str()),
                    _("Error"),wxOK | wxICON_EXCLAMATION);
                if(dlg.ShowModal()!=wxID_OK)
                    return false;
            };
            wxFileName prefix(dlg.GetPath());
            while (!prefix.IsDirWritable())
            {
                wxMessageBox(wxString::Format(_("You have no permissions to write in folder \"%s\".\nPlease select another folder for the final output."), prefix.GetPath().c_str()),
#ifdef __WXMSW__
                    wxT("Hugin_stitch_project"),
#else
                    wxT(""),
#endif
                    wxOK | wxICON_INFORMATION);
                if (dlg.ShowModal() != wxID_OK)
                {
                    return false;
                };
                prefix = dlg.GetPath();
            };

            wxConfig::Get()->Write(wxT("/actualPath"), dlg.GetDirectory());  // remember for later
            outname = dlg.GetPath();
        } else { // bail
//            wxLogError( _("No project files specified"));
            return false;
        }
    }

    // check output filename
    wxFileName outfn(outname);
    wxString ext = outfn.GetExt();
    // remove extension if it indicates an image file
    if (ext.CmpNoCase(wxT("jpg")) == 0 || ext.CmpNoCase(wxT("jpeg")) == 0|| 
        ext.CmpNoCase(wxT("tif")) == 0|| ext.CmpNoCase(wxT("tiff")) == 0 || 
        ext.CmpNoCase(wxT("png")) == 0 || ext.CmpNoCase(wxT("exr")) == 0 ||
        ext.CmpNoCase(wxT("pnm")) == 0 || ext.CmpNoCase(wxT("hdr"))) 
    {
        outfn.ClearExt();
        outname = outfn.GetFullPath();
    }

    RunStitchFrame *frame = new RunStitchFrame(NULL, wxT("Hugin Stitcher"), wxDefaultPosition, wxSize(640,600) );
    frame->Show( true );
    SetTopWindow( frame );

    wxFileName basename(scriptFile);
    frame->SetTitle(wxString::Format(_("%s - Stitching"), basename.GetName().c_str()));
    frame->SetOverwrite(parser.Found(wxT("w")));
    bool n = frame->StitchProject(scriptFile, outname, progs, parser.Found(wxT("d")));
    return n;
}


int stitchApp::OnExit()
{
    DEBUG_TRACE("");
    return 0;
}


#ifdef __WXMAC__
// wx calls this method when the app gets "Open file" AppleEvent 
void stitchApp::MacOpenFile(const wxString &fileName)
{
    m_macFileNameToOpenOnStart = fileName;
}
#endif

// make wxwindows use this class as the main application
IMPLEMENT_APP(stitchApp)
