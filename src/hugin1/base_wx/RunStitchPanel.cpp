// -*- c-basic-offset: 4 -*-

/** @file RunStitchPanel.cpp
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: hugin_stitch_project.cpp 2705 2008-01-27 19:56:06Z ippei $
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
#include <vigra/error.hxx>
#include <vigra_ext/MultiThreadOperations.h>
#include "PT/Panorama.h"
#include "PT/utils.h"
#include "base_wx/huginConfig.h"
#include "base_wx/MyProgressDialog.h"
#include "base_wx/MyExternalCmdExecDialog.h"
#include "base_wx/platform.h"
#include "base_wx/wxPlatform.h"

#include "RunStitchPanel.h"

#include "hugin/config_defaults.h"

using namespace vigra;
using namespace PT;
using namespace std;
using namespace hugin_utils;


// ==========================================
// Implementation of stitch window

// event ID's for RunStitchPanel
enum
{
    ID_Quit = 1,
    ID_About   
};


BEGIN_EVENT_TABLE(RunStitchPanel, wxPanel)
    EVT_END_PROCESS(-1, RunStitchPanel::OnProcessTerminate)
END_EVENT_TABLE()

RunStitchPanel::RunStitchPanel(wxWindow * parent)
    : wxPanel(parent)
{
	m_paused=false;
	m_overwrite=false;
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
    m_execPanel = new MyExecPanel(this);

#ifdef __WXMSW__
    // wxFrame does have a strange background color on Windows, copy color from a child widget
    this->SetBackgroundColour(m_execPanel->GetBackgroundColour());
#endif
    topsizer->Add(m_execPanel, 1, wxEXPAND, 0);
    SetSizer( topsizer );
//    topsizer->SetSizeHints( this );   // set size hints to honour minimum size
}

wxString getGNUMakeCmd(const wxString &args)
{
#if defined __WXMAC__ && defined MAC_SELF_CONTAINED_BUNDLE   
    wxString cmd = MacGetPathToBundledExecutableFile(CFSTR("gnumake"));  
    if(cmd != wxT(""))
    {
        cmd = wxQuoteString(cmd); 
    }
    else
    {
        wxMessageBox(wxString::Format(_("External program %s not found in the bundle, reverting to system path"), wxT("gnumake")), _("Error"));
        cmd = wxT("make");  
    }
    cmd += wxT(" ") + args;
#elif defined __FreeBSD__
    wxString cmd = wxT("gmake ") + args;  
#elif defined __WXMSW__
    wxString cmdExe;
    if(!wxGetEnv(wxT("ComSpec"),&cmdExe))
        cmdExe=wxT("cmd");
    wxString tempDir=wxConfigBase::Get()->Read(wxT("tempDir"),wxT(""));
    wxString cmd = cmdExe + wxString::Format(wxT(" /C \"chcp %d >NUL && "),GetACP());
    //explicit set temp path for make, e. g. in case user name contains an ampersand
    if(tempDir.Len()>0)
    {
        cmd=cmd + wxT("set TEMP=")+tempDir + wxT(" && set TMP=") + tempDir + wxT(" && ");
    };
    cmd = cmd +  wxT("\"") + getExePath(wxTheApp->argv[0])+wxT("\\make\" ") + args + wxT("\"");
#else
    wxString cmd = wxT("make ") + args;  
#endif
    return cmd;
};

bool RunStitchPanel::StitchProject(wxString scriptFile, wxString outname,
                                   HuginBase::PanoramaMakefilelibExport::PTPrograms progs)
{
    DEBUG_TRACE("");
    wxFileName fname(scriptFile);
    if ( !fname.FileExists() ) {
        wxLogError( _("Could not open project file:") + scriptFile);
        return false;
    }

    wxString pathToPTO;
    wxFileName::SplitPath(scriptFile, &pathToPTO, NULL, NULL);
    pathToPTO.Append(wxFileName::GetPathSeparator());

    ifstream prjfile((const char *)scriptFile.mb_str(HUGIN_CONV_FILENAME));
    if (prjfile.bad()) {
        wxLogError( wxString::Format(_("could not open script : %s"), scriptFile.c_str()) );
        return false;
    }
    PT::Panorama pano;
    PT::PanoramaMemento newPano;
    int ptoVersion = 0;
    if (newPano.loadPTScript(prjfile, ptoVersion, (const char *)pathToPTO.mb_str(HUGIN_CONV_FILENAME))) {
        pano.setMemento(newPano);
        HuginBase::PanoramaOptions opts = pano.getOptions();
        if (ptoVersion < 2) {
            // no options stored in file, use default arguments in config
			
			wxConfig* config = new wxConfig(wxT("hugin"));  //needed for PTBatcher console application
			wxConfigBase::Set(config);                      //
			opts.enblendOptions = wxConfigBase::Get()->Read(wxT("/Enblend/Args"), wxT(HUGIN_ENBLEND_ARGS)).mb_str(wxConvLocal);
			opts.enfuseOptions = wxConfigBase::Get()->Read(wxT("/Enfuse/Args"), wxT(HUGIN_ENFUSE_ARGS)).mb_str(wxConvLocal);

        }
        opts.remapUsingGPU = wxConfigBase::Get()->Read(wxT("/Nona/UseGPU"), HUGIN_NONA_USEGPU) == 1;
        pano.setOptions(opts);
    } else {
        wxLogError( wxString::Format(_("error while parsing panotools script: %s"), scriptFile.c_str()) );
        return false;
    }
    // get options and correct for correct makefile
    PanoramaOptions opts = pano.getOptions();
    opts.outputFormat = PanoramaOptions::TIFF_m;
    if (opts.enblendOptions.length() == 0) {
        // no options stored in file, use default arguments in config file

		wxConfig* config = new wxConfig(wxT("hugin"));  //needed for PTBatcher console application
		wxConfigBase::Set(config);                      //
        opts.enblendOptions = wxConfigBase::Get()->Read(wxT("/Enblend/Args"), wxT(HUGIN_ENBLEND_ARGS)).mb_str(wxConvLocal);
    }
    pano.setOptions(opts);
    // make sure we got an absolute path
    if (! wxIsAbsolutePath(outname)) {
        outname = wxGetCwd() + wxFileName::GetPathSeparator() + outname;
    }

    DEBUG_DEBUG("output file specified is " << (const char *)outname.mb_str(wxConvLocal));

    wxString basename;
    wxString outpath;
    wxFileName outputPrefix(outname);
    outpath = outputPrefix.GetPath();
    basename = outputPrefix.GetFullName();
    // stitch only active images
    UIntSet activeImgs = pano.getActiveImages();
    //get temp dir from preferences
    wxString tempDir= wxConfigBase::Get()->Read(wxT("tempDir"),wxT(""));
    if(!tempDir.IsEmpty())
        if(tempDir.Last()!=wxFileName::GetPathSeparator())
            tempDir.Append(wxFileName::GetPathSeparator());

    try {
        PanoramaOptions  opts = pano.getOptions();
		//preview image generation
		/*opts.setHeight(150);
		opts.setWidth(300);
		opts.setROI(vigra::Rect2D(vigra::Size2D(300,150)));
		pano.setOptions(opts);*/
        // copy pto file to temporary file
        m_currentPTOfn = wxFileName::CreateTempFileName(tempDir+wxT("huginpto_"));
        if(m_currentPTOfn.size() == 0) {
            wxLogError(_("Could not create temporary file"));
        }
        DEBUG_DEBUG("tmpPTOfn file: " << (const char *)m_currentPTOfn.mb_str(wxConvLocal));
        // copy is not enough, need to adjust image path names...
        ofstream script(m_currentPTOfn.mb_str(HUGIN_CONV_FILENAME));
        PT::UIntSet all;
        if (pano.getNrOfImages() > 0) {
            fill_set(all, 0, pano.getNrOfImages()-1);
        }
        pano.printPanoramaScript(script, pano.getOptimizeVector(), pano.getOptions(), all, false, "");
        script.close();
        // produce suitable makefile

        wxFile makeFile;
        //TODO: change to implementatin with config->Read(wxT("tempDir"),wxT(""))
        m_currentMakefn = wxFileName::CreateTempFileName(tempDir+wxT("huginmk_"), &makeFile);
        if(m_currentMakefn.size() == 0) {
            wxLogError(_("Could not create temporary file"));
            return false;
        }
        DEBUG_DEBUG("makefn file: " << (const char *)m_currentMakefn.mb_str(wxConvLocal));
        ofstream makeFileStream(m_currentMakefn.mb_str(HUGIN_CONV_FILENAME));
        makeFile.Close();
        std::string resultFn(basename.mb_str(HUGIN_CONV_FILENAME));
        std::string tmpPTOfnC = (const char *) m_currentPTOfn.mb_str(HUGIN_CONV_FILENAME);
        wxConfigBase* config=wxConfigBase::Get();
        std::string tmpDir((config->Read(wxT("tempDir"),wxT(""))).mb_str(HUGIN_CONV_FILENAME));
        bool copyMetadata=config->Read(wxT("/output/useExiftool"),1l)==1l;
        int nrThreads=config->Read(wxT("/output/NumberOfThreads"),0l);

        std::vector<std::string> outputFiles;
        HuginBase::PanoramaMakefilelibExport::createMakefile(pano,
                           activeImgs,
                           tmpPTOfnC,
                           resultFn,
                           progs,
                           "",
                           outputFiles,
                           makeFileStream,
                           tmpDir,
                           copyMetadata,
                           nrThreads);
        makeFileStream.close();

        // cd to output directory, if one is given.
        wxString oldCWD = wxFileName::GetCwd();
        wxFileName::SetCwd(outpath);
        // check output directories.
        //std::vector<std::string> overwrittenFiles;
        wxString overwrittenFiles;
        for(size_t i=0; i < outputFiles.size(); i++) {
            wxString fn(outputFiles[i].c_str(), wxConvLocal);
            if (wxFile::Exists(fn) ) {
                overwrittenFiles.Append(fn + wxT(" "));
            }
        }
        if (!m_overwrite && overwrittenFiles.size() > 0) {
            int overwriteret = wxMessageBox(_("Overwrite existing images?\n\n") + overwrittenFiles, _("Overwrite existing images"), wxYES_NO | wxICON_QUESTION);
            // TODO: change button label ok to overwrite
            if (overwriteret != wxYES) {
                DEBUG_DEBUG("Abort, do not overwrite images!");
                return false;
            }
            DEBUG_DEBUG("Overwrite existing images!");
        }

#if defined __WXMSW__
        wxString args = wxT("-f ") + wxQuoteFilename(m_currentMakefn) + wxT(" info test all clean");
#else
        wxString args = wxT("-j1 -f ") + wxQuoteString(m_currentMakefn) + wxT(" info test all clean");
#endif

        wxString caption = wxString::Format(_("Stitching %s"), scriptFile.c_str());

        wxString cmd=getGNUMakeCmd(args);
        if (m_execPanel->ExecWithRedirect(cmd) == -1) {
            wxMessageBox(wxString::Format(_("Error while stitching project\n%s"), cmd.c_str()),
                         _("Error during stitching"),  wxICON_ERROR | wxOK );
        }
		wxFileName::SetCwd(oldCWD);
    } catch (std::exception & e) {
        cerr << "caught exception: " << e.what() << std::endl;
        wxMessageBox(wxString(e.what(), wxConvLocal),
                     _("Error during stitching"), wxICON_ERROR | wxOK );
    }
    return true;
}

bool RunStitchPanel::DetectProject(wxString scriptFile, 
                                   HuginBase::AssistantMakefilelibExport::AssistantPrograms progs)
{
    m_currentPTOfn=wxEmptyString;
    wxFileName fname(scriptFile);
    if ( !fname.FileExists() ) {
        wxLogError( _("Could not open project file:") + scriptFile);
        return false;
    }

    //read project file
    ifstream prjfile((const char *)scriptFile.mb_str(HUGIN_CONV_FILENAME));
    if (prjfile.bad())
    {
        wxLogError( wxString::Format(_("could not open script : %s"), scriptFile.c_str()));
        return false;
    }
    PT::Panorama pano;
    PT::PanoramaMemento newPano;
    int ptoVersion = 0;
    if (!newPano.loadPTScript(prjfile, ptoVersion))
    {
        wxLogError(wxString::Format(_("error while parsing panotools script: %s"), scriptFile.c_str()));
        return false;
    }
    pano.setMemento(newPano);

    //read settings
    wxConfig config(wxT("hugin"));
    bool runCeleste=config.Read(wxT("/Celeste/Auto"), HUGIN_CELESTE_AUTO)!=0;
    double celesteThreshold;
    config.Read(wxT("/Celeste/Threshold"), &celesteThreshold, HUGIN_CELESTE_THRESHOLD);
    bool celesteSmall=config.Read(wxT("/Celeste/Filter"), HUGIN_CELESTE_FILTER)!=0;
    bool runLinefind=config.Read(wxT("/Assistant/Linefind"), HUGIN_ASS_LINEFIND)!=0;
    bool runCPClean=config.Read(wxT("/Assistant/AutoCPClean"), HUGIN_ASS_AUTO_CPCLEAN)!=0;
    double scale;
    config.Read(wxT("/Assistant/panoDownsizeFactor"), &scale, HUGIN_ASS_PANO_DOWNSIZE_FACTOR);
    int scalei=roundi(scale*100);
    wxString tempDir= config.Read(wxT("tempDir"),wxT(""));
    if(!tempDir.IsEmpty())
        if(tempDir.Last()!=wxFileName::GetPathSeparator())
            tempDir.Append(wxFileName::GetPathSeparator());

    try {
        //generate makefile
        m_currentMakefn=wxFileName::CreateTempFileName(tempDir+wxT("ham"));
        if(m_currentMakefn.size() == 0)
        {
            wxLogError(_("Could not create temporary file"));
            return false;
        }
        std::ofstream makefile(m_currentMakefn.mb_str(HUGIN_CONV_FILENAME));
        makefile.exceptions( std::ofstream::eofbit | std::ofstream::failbit | std::ofstream::badbit );
        std::string scriptString(scriptFile.mb_str(HUGIN_CONV_FILENAME));
        HuginBase::AssistantMakefilelibExport::createMakefile(pano,progs,runLinefind,runCeleste,celesteThreshold,celesteSmall,
            runCPClean,scale,makefile,scriptString);
        makefile.close();

        //execute makefile
        wxString args = wxT("-f ") + wxQuoteFilename(m_currentMakefn) + wxT(" all");
        wxString cmd=getGNUMakeCmd(args);
        wxFileName path(scriptFile);
        path.MakeAbsolute();
        wxString oldcwd=path.GetCwd();
        path.SetCwd();
        if (m_execPanel->ExecWithRedirect(cmd) == -1)
        {
            wxMessageBox(wxString::Format(_("Error while running assistant\n%s"), cmd.c_str()),
                         _("Error during running assistant"),  wxICON_ERROR | wxOK );
        }
        wxFileName::SetCwd(oldcwd);
    } 
    catch (std::exception & e)
    {
        cerr << "caught exception: " << e.what() << std::endl;
        wxMessageBox(wxString(e.what(), wxConvLocal),
                     _("Error during running assistant"), wxICON_ERROR | wxOK );
    }
    return true;
}

void RunStitchPanel::OnProcessTerminate(wxProcessEvent & event)
{
    DEBUG_TRACE("");
	//if(!m_paused)
	//{
		// delete temporary files
#ifndef DEBUG
    if(!m_currentMakefn.IsEmpty())
    {
		wxRemoveFile(m_currentMakefn);
    };
    if(!m_currentPTOfn.IsEmpty())
    {
		wxRemoveFile(m_currentPTOfn);
    };
#endif
		// notify parent of exit
		if (this->GetParent()) {
			event.SetEventObject( this );
			DEBUG_TRACE("Sending wxProcess event");   
			this->GetParent()->GetEventHandler()->ProcessEvent( event );
		}
	//}
}

void RunStitchPanel::CancelStitch()
{
    DEBUG_TRACE("");
    m_execPanel->KillProcess();
}

bool RunStitchPanel::IsPaused()
{
	return m_paused;
}

void RunStitchPanel::SetOverwrite(bool over)
{
	m_overwrite = over;
}

void RunStitchPanel::PauseStitch()
{
	m_paused=true;
	m_execPanel->PauseProcess();
}

void RunStitchPanel::ContinueStitch()
{
	m_execPanel->ContinueProcess();
	m_paused=false;
}

long RunStitchPanel::GetPid()
{
	return m_execPanel->GetPid();
}

bool RunStitchPanel::SaveLog(const wxString &filename)
{
    return m_execPanel->SaveLog(filename);
};
