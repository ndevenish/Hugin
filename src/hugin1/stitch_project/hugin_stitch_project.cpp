// -*- c-basic-offset: 4 -*-

/** @file wx_nona.cpp
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
#include <vigra/error.hxx>
#include <vigra_ext/MultiThreadOperations.h>
#include "PT/Panorama.h"
#include "PT/utils.h"
#include "base_wx/huginConfig.h"
#include "base_wx/MyProgressDialog.h"
#include "base_wx/MyExternalCmdExecDialog.h"
#include "base_wx/platform.h"
#include "common/wxPlatform.h"

#include <tiffio.h>

// somewhere SetDesc gets defined.. this breaks wx/cmdline.h on OSX
#ifdef SetDesc
#undef SetDesc
#endif

#include <wx/cmdline.h>

using namespace vigra;
using namespace PT;
using namespace std;
using namespace utils;

/** The application class for nona gui
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
    void stitchApp::MacOpenFile(const wxString &fileName);
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

    m_locale.Init(wxLANGUAGE_DEFAULT);

    // setup the environment for the different operating systems
#if defined __WXMSW__
    wxString huginExeDir = getExePath(argv[0]);

    wxString huginRoot;
    wxFileName::SplitPath( huginExeDir, &huginRoot, NULL, NULL );
    m_locale.AddCatalogLookupPathPrefix(huginRoot + wxT("/locale"));

    PTPrograms progs = getPTProgramsConfig(huginExeDir, wxConfigBase::Get());
#else
    // add the locale directory specified during configure
    m_locale.AddCatalogLookupPathPrefix(wxT(INSTALL_LOCALE_DIR));
    PTPrograms progs = getPTProgramsConfig(wxT(""), wxConfigBase::Get());
#endif
    
#if defined __WXMAC__ && defined MAC_SELF_CONTAINED_BUNDLE
    {	 
        wxString exec_path = MacGetPathToBundledExecutableFile(CFSTR("nona"));	 
        if(exec_path != wxT(""))	 
        {	 
            progs.nona = exec_path.mb_str();	 
        }	 
        
        exec_path = MacGetPathToBundledExecutableFile(CFSTR("hugin_hdrmerge"));	 
        if(exec_path != wxT(""))	 
        {	 
            progs.hdrmerge = exec_path.mb_str();	 
        }	 
    }	 
#endif

    // set the name of locale recource to look for
    m_locale.AddCatalog(wxT("hugin"));

    // parse arguments
    static const wxCmdLineEntryDesc cmdLineDesc[] =
    {
      { wxCMD_LINE_SWITCH, wxT("h"), wxT("help"), wxT("show this help message"),
        wxCMD_LINE_VAL_NONE, wxCMD_LINE_OPTION_HELP },
      { wxCMD_LINE_OPTION, wxT("o"), wxT("output"),  wxT("output prefix") },
      { wxCMD_LINE_OPTION, wxT("t"), wxT("threads"),  wxT("number of threads"),
             wxCMD_LINE_VAL_NUMBER, wxCMD_LINE_PARAM_OPTIONAL },
      { wxCMD_LINE_PARAM,  NULL, NULL, _T("<project> <images>"),
        wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL + wxCMD_LINE_PARAM_MULTIPLE },
      { wxCMD_LINE_NONE }
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

    bool imgsFromCmdline = false;

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
                         _("Project files (*.pto,*.ptp,*.pts,*.oto)|*.pto;*.ptp;*.pts;*.oto;|All files (*)|*"),
                         wxOPEN, wxDefaultPosition);

        dlg.SetDirectory(wxConfigBase::Get()->Read(wxT("/actualPath"),wxT("")));
        if (dlg.ShowModal() == wxID_OK) {
            wxConfig::Get()->Write(wxT("/actualPath"), dlg.GetDirectory());  // remember for later
            scriptFile = dlg.GetPath();
        } else { // bail
            return false;
        }
    } else if(wxIsEmpty(scriptFile)) {
        scriptFile = parser.GetParam(0);
        cout << "********************* script file: " << (const char *)scriptFile.mb_str() << endl;
        if (! wxIsAbsolutePath(scriptFile)) {
            scriptFile = wxGetCwd() + wxT("/") + scriptFile;
        }
        if ( parser.GetParamCount() > 1) {
          // load images.
          imgsFromCmdline = true;
        }
    }


    cout << "input file is " << (const char *)scriptFile.mb_str() << endl;

    wxFileName fname(scriptFile);
    if ( !fname.FileExists() ) {
      wxLogError( _("Could not open project file:") + scriptFile);
      return false;
    }

    wxString pathToPTO;
    wxFileName::SplitPath(scriptFile, &pathToPTO, NULL, NULL);
    pathToPTO.Append(wxT("/"));

    ifstream prjfile((const char *)scriptFile.mb_str());
    if (prjfile.bad()) {
        wxLogError( wxString::Format(_("could not open script : %s"), scriptFile.c_str()) );
        return false;
    }
    

    Panorama pano;
    PanoramaMemento newPano;

    if (newPano.loadPTScript(prjfile, (const char *)pathToPTO.mb_str())) {
      pano.setMemento(newPano);
    } else {
      wxLogError( wxString::Format(_("error while parsing panos tool script: %s"), scriptFile.c_str()) );
      return false;
    }

    wxString outname;

    if ( !parser.Found(wxT("o"), &outname) ) {
        // ask for output.
        wxFileDialog dlg(0,_("Specify output prefix"),
                         wxConfigBase::Get()->Read(wxT("/actualPath"),wxT("")),
                         wxT(""), wxT(""),
                         wxSAVE, wxDefaultPosition);
        dlg.SetDirectory(wxConfigBase::Get()->Read(wxT("/actualPath"),wxT("")));
        if (dlg.ShowModal() == wxID_OK) {
            wxConfig::Get()->Write(wxT("/actualPath"), dlg.GetDirectory());  // remember for later
            outname = dlg.GetPath();
        } else { // bail
            wxLogError( _("No project files specified"));
            return false;
        }
    }
    // make sure we got an absolute path
    if (! wxIsAbsolutePath(outname)) {
        outname = wxGetCwd() + wxT("/") + outname;
    }
	
    DEBUG_DEBUG("output file specified is " << (const char *)outname.mb_str());

    wxString basename;
    wxString outpath;
    wxFileName outputPrefix(outname);
    outpath = outputPrefix.GetPath();
    basename = outputPrefix.GetFullName();
    cout << "output path: " << outpath.mb_str() << " file:" << basename.mb_str() << endl;

    long nThreads = wxThread::GetCPUCount();
    parser.Found(wxT("t"), & nThreads);
    if (nThreads <= 0) nThreads = 1;
    vigra_ext::ThreadManager::get().setNThreads((unsigned) nThreads);

    //utils::StreamMultiProgressDisplay pdisp(cout);
    //MyProgressDialog pdisp(_("Stitching Panorama"), wxT(""), NULL, wxPD_ELAPSED_TIME | wxPD_AUTO_HIDE | wxPD_APP_MODAL );


    // stitch only active images
    UIntSet activeImgs = pano.getActiveImages();

    bool keepWindow = false;
    if (imgsFromCmdline) {
        if (parser.GetParamCount() -1 != activeImgs.size()) {
            wxLogError(_("Wrong number of images specified on command line"));
            return false;
        }
        size_t i = 0;
        for (UIntSet::iterator it = activeImgs.begin();
             it != activeImgs.end(); ++it, ++i)
        {
            pano.setImageFilename(*it, (const char *)parser.GetParam(i+1).mb_str());
        }
    }

    try {
        PanoramaOptions  opts = pano.getOptions();

        // copy pto file to temporary file
        wxString tmpPTOfn = wxFileName::CreateTempFileName(wxT("huginpto_"));
        if(tmpPTOfn.size() == 0) {
            wxLogError(_("Could not create temporary file"));
        }
        DEBUG_DEBUG("tmpPTOfn file: " << (const char *)tmpPTOfn.mb_str());
        // copy is not enough, need to adjust image path names...
        ofstream script(tmpPTOfn.mb_str());
        PT::UIntSet all;
        if (pano.getNrOfImages() > 0) {
           fill_set(all, 0, pano.getNrOfImages()-1);
        }
        pano.printPanoramaScript(script, pano.getOptimizeVector(), pano.getOptions(), all, false, "");
        script.close();


        // produce suitable makefile

        wxFile makeFile;
        //TODO: change to implementatin with config->Read(wxT("tempDir"),wxT(""))
        wxString makefn = wxFileName::CreateTempFileName(wxT("huginmk_"), &makeFile);
        if(makefn.size() == 0) {
            wxLogError(_("Could not create temporary file"));
            return false;
        }
        DEBUG_DEBUG("makefn file: " << (const char *)makefn.mb_str());
        ofstream makeFileStream(makefn.mb_str());
        makeFile.Close();

        std::string ptoFn = (const char *) tmpPTOfn.mb_str();

        std::string resultFn(basename.mb_str());

        std::string tmpPTOfnC = (const char *) tmpPTOfn.mb_str();

        PT::createMakefile(pano,
                           activeImgs,
                           tmpPTOfnC,
                           resultFn,
                           progs,
                           "",
                           makeFileStream);

        // cd to output directory, if one is given.
        wxString oldCWD = wxFileName::GetCwd();
        wxFileName::SetCwd(outpath);

        wxString args = wxT("-f ") + wxQuoteString(makefn) + wxT(" all clean");

        wxString caption = wxString::Format(_("Stitching %s"), scriptFile.c_str());

        wxString cmd = wxT("make ") + args;

#if 1
        int ret = MyExecuteCommandOnDialog(wxT("make"), args, NULL, caption);
        if (ret != 0) {
            keepWindow = true;
            wxMessageBox(wxString::Format(_("Error while stitching project\n%s"), scriptFile.c_str()),
                         _("Error during stitching"), wxICON_ERROR | wxOK );
        }
#else
        // This crashes.. Don't know why..
        MyExternalCmdExecDialog execDlg(NULL, wxID_ANY);
        int ret = execDlg.ShowModal(cmd);
        cout << " exit code: " << ret << std::endl;
        if (ret != 0) {
            keepWindow = true;
            wxMessageBox(wxString::Format(_("Error while stitching project\n%s"), scriptFile.c_str()),
                         _("Error during stitching"), wxICON_ERROR | wxOK );
        }
#endif

        // delete temporary files
#ifndef DEBUG
        wxRemoveFile(makefn);
        wxRemoveFile(tmpPTOfn);
#endif
    } catch (std::exception & e) {
        cerr << "caught exception: " << e.what() << std::endl;
        wxMessageBox(wxString(e.what(), wxConvLocal),
                     _("Error during stitching"), wxICON_ERROR | wxOK );
        
        return true;
    }

    return keepWindow;
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
