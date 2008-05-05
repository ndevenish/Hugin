// -*- c-basic-offset: 4 -*-

/** @file wx_nona.cpp
 *
 *  @brief stitcher, with wxwindows progress display
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


#include <fstream>
#include <sstream>
#include <vigra/error.hxx>
#include "PT/Stitcher.h"
#include "base_wx/MyProgressDialog.h"
#include "base_wx/huginConfig.h"

#include <tiffio.h>

// somewhere SetDesc gets defined.. this breaks wx/cmdline.h on OSX
#ifdef SetDesc
#undef SetDesc
#endif

#include <wx/cmdline.h>

using namespace vigra;
using namespace PT;
using namespace std;

/** The application class for nona gui
 *
 *  it contains the main frame.
 */
class nonaApp : public wxApp
{
public:

    /** ctor.
     */
    nonaApp();

    /** dtor.
     */
    virtual ~nonaApp();

    /** pseudo constructor. with the ability to fail gracefully.
     */
    virtual bool OnInit();

    /** just for testing purposes */
    virtual int OnExit();
    
#ifdef __WXMAC__
    /** the wx calls this method when the app gets "Open file" AppleEvent */
    void nonaApp::MacOpenFile(const wxString &fileName);
#endif

private:
    wxLocale m_locale;
#ifdef __WXMAC__
    wxString m_macFileNameToOpenOnStart;
#endif
};


nonaApp::nonaApp()
{
    // suppress tiff warnings
    TIFFSetWarningHandler(0);

    DEBUG_TRACE("ctor");
}

nonaApp::~nonaApp()
{
    DEBUG_TRACE("dtor");
    DEBUG_TRACE("dtor end");
}

bool nonaApp::OnInit()
{
    SetAppName(wxT("nona_gui"));
    m_locale.Init(wxLANGUAGE_DEFAULT);
#if defined __WXMSW__
	wxString nonaExeDir = getExePath(argv[0]);
	
	wxString nonaRoot;
	wxFileName::SplitPath(nonaExeDir, &nonaRoot, NULL, NULL);
	// locale setup
    m_locale.AddCatalogLookupPathPrefix(nonaRoot + wxT("/share/locale"));
#elif (defined __WXMAC__ && defined MAC_SELF_CONTAINED_BUNDLE)
    // TODO: add localisation init
#else
    DEBUG_INFO("add locale path: " << INSTALL_LOCALE_DIR);
    m_locale.AddCatalogLookupPathPrefix(wxT(INSTALL_LOCALE_DIR));
#endif

    // set the name of locale recource to look for
    m_locale.AddCatalog(wxT("hugin"));
	
#if 0
#ifdef wxUSE_UNIX
    wxLog *logger=new wxLogStream(&cerr);
    wxLog::SetActiveTarget(logger);
#endif
#endif

    // parse arguments
    static const wxCmdLineEntryDesc cmdLineDesc[] =
    {
      { wxCMD_LINE_SWITCH, wxT("h"), wxT("help"), wxT("show this help message"),
        wxCMD_LINE_VAL_NONE, wxCMD_LINE_OPTION_HELP },
      { wxCMD_LINE_OPTION, wxT("o"), wxT("output"),  wxT("output file") },
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
#endif
    if( parser.GetParamCount() == 0 && wxIsEmpty(scriptFile)) 
    {
        wxFileDialog dlg(0,_("Specify project source project file"),
                        wxConfigBase::Get()->Read(wxT("/actualPath"),wxT("")),
                        wxT(""), wxT(""),
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
        if ( parser.GetParamCount() > 1) {
          // load images.
          imgsFromCmdline = true;
        }
    }

    DEBUG_DEBUG("input file is " << (const char *)scriptFile.mb_str(wxConvLocal))

    wxFileName fname(scriptFile);
    if ( !fname.FileExists() ) {
      wxLogError( _("Could not find project file:") + scriptFile);
      return false;
    }
    wxString path = fname.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);

    wxString outname;

    if ( !parser.Found(wxT("o"), &outname) ) {
        // ask for output.
        wxFileDialog dlg(0,_("Specify output image filename"),
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
    DEBUG_DEBUG("output file specified is " << (const char *)outname.mb_str(wxConvLocal));

    long nThreads = wxThread::GetCPUCount();
    parser.Found(wxT("t"), & nThreads);
    if (nThreads <= 0) nThreads = 1;
    vigra_ext::ThreadManager::get().setNThreads((unsigned) nThreads);

    wxString basename;
    wxString outpath;
    wxFileName::SplitPath(outname, &outpath, &basename, NULL);

    //utils::StreamMultiProgressDisplay pdisp(cout);
    MyProgressDialog pdisp(_("Stitching Panorama"), wxT(""), NULL, wxPD_ELAPSED_TIME | wxPD_AUTO_HIDE | wxPD_APP_MODAL );

    Panorama pano;
    PanoramaMemento newPano;
    ifstream prjfile((const char *)scriptFile.mb_str(*wxConvFileName));
    if (prjfile.bad()) {
        wxLogError( wxString::Format(_("could not open script : %s"), scriptFile.c_str()) );
        return false;
    }
    int ptoVersion = 0;
    if (newPano.loadPTScript(prjfile, ptoVersion, (const char *)path.mb_str(*wxConvFileName))) {
      pano.setMemento(newPano);
    } else {
      wxLogError( wxString::Format(_("error while parsing panotools script: %s"), scriptFile.c_str()) );
      return false;
    }

    if (imgsFromCmdline) {
        if (parser.GetParamCount() -1 != pano.getNrOfImages()) {
            wxLogError(_("Wrong number of images specified on command line"));
            return false;
        }
        for (size_t i = 0; i < pano.getNrOfImages(); i++) {
            pano.setImageFilename(i, (const char *)parser.GetParam(i+1).mb_str(*wxConvFileName));
        }
    }

    PanoramaOptions  opts = pano.getOptions();

    // check for some options

    int w = opts.getWidth();
    int h = opts.getHeight();

    cout << (const char *)wxString::Format(wxT("%s"), _("output image size: ")).mb_str(wxConvLocal) << w << "x" << h << std::endl;
    wxString outfile;
    if ( outpath != wxT("") ) {
      outfile = outpath + wxFileName::GetPathSeparator() + basename;
    } else {
      outfile = basename;
    }
    DEBUG_DEBUG("output name: " << (const char *)outfile.mb_str(wxConvLocal));

    try {
        // stitch panorama
        UIntSet simgs = pano.getActiveImages();
        PT::stitchPanorama(pano, opts,
                           pdisp, (const char *)outfile.mb_str(*wxConvFileName), simgs);
    } catch (std::exception & e) {
        cerr << "caught exception: " << e.what() << std::endl;
        return false;
    }

    return false;
}


int nonaApp::OnExit()
{
    DEBUG_TRACE("");
    return 0;
}


#ifdef __WXMAC__
// wx calls this method when the app gets "Open file" AppleEvent 
void nonaApp::MacOpenFile(const wxString &fileName)
{
    m_macFileNameToOpenOnStart = fileName;
}
#endif

// make wxwindows use this class as the main application
IMPLEMENT_APP(nonaApp)
