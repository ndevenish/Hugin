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

#include "panoinc_WX.h"
#include "panoinc.h"

#include <fstream>
#include <sstream>

#include <vigra/error.hxx>

#include <unistd.h>

#include "PT/Stitcher.h"

#include "hugin/MyProgressDialog.h"

#include <tiffio.h>

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

    void usage(const char * name);


private:
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
    SetAppName("nona_gui");

    // parse arguments
    const char * optstring = "ho:";
    int c;

    opterr = 0;

    string basename;

    while ((c = getopt (argc, argv, optstring)) != -1)
        switch (c) {
        case 'o':
            basename = optarg;
            break;
        case '?':
        case 'h':
            usage(argv[0]);
            return false;
        default:
            return false;
        }

    string scriptFile;
    if (argc - optind <1) {
        // ask for project file
        wxFileDialog dlg(0,_("Specify project source project file"),
                         wxConfigBase::Get()->Read("actualPath",""),
                         "", "",
                         wxOPEN, wxDefaultPosition);
        if (dlg.ShowModal() == wxID_OK) {
            wxConfig::Get()->Write("actualPath", dlg.GetDirectory());  // remember for later
            scriptFile = dlg.GetPath().c_str();
        } else {
            usage(argv[0]);
            return 1;
        }
    } else {
        scriptFile = argv[optind];
    }

    wxFileName fname(scriptFile.c_str());
    wxString path = fname.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);


    if (basename == "") {
        // ask for output.
        wxFileDialog dlg(0,_("Specify output image filename"),
                         wxConfigBase::Get()->Read("actualPath",""),
                         "", "",
                         wxSAVE, wxDefaultPosition);
        if (dlg.ShowModal() == wxID_OK) {
            wxConfig::Get()->Write("actualPath", dlg.GetDirectory());  // remember for later
            basename = dlg.GetPath().c_str();
        } else {
            usage(argv[0]);
            return 1;
        }
    }

    basename = utils::stripExtension(basename);

    //utils::StreamMultiProgressDisplay pdisp(cout);
    MyProgressDialog pdisp(_("Stitching Panorama"), "", NULL, wxPD_ELAPSED_TIME | wxPD_AUTO_HIDE | wxPD_APP_MODAL );

    Panorama pano;
    PanoramaMemento newPano;
    ifstream prjfile(scriptFile.c_str());
    if (prjfile.bad()) {
        ostringstream error;
        error << _("could not open script : ") << scriptFile << std::endl;
        wxMessageBox(error.str().c_str() , _("Error"), wxCANCEL | wxICON_ERROR);
        exit(1);
    }
    if (newPano.loadPTScript(prjfile, path.c_str())) {
        pano.setMemento(newPano);
    } else {
        ostringstream error;
        error << _("error while parsing panos tool script: ") << scriptFile << std::endl;

        wxMessageBox(error.str().c_str() , _("Error"), wxCANCEL | wxICON_ERROR);
        exit(1);
    }

    PanoramaOptions  opts = pano.getOptions();

    // check for some options

    int w = opts.width;
    int h = opts.getHeight();

    cout << "output image size: " << w << "x" << h << std::endl;

    DEBUG_DEBUG("output basename: " << basename);

    try {
        // stitch panorama
        PT::stitchPanorama(pano, opts,
                           pdisp, basename);
    } catch (std::exception & e) {
        cerr << "caught exception: " << e.what() << std::endl;
        return 1;
    }

    return false;
}


int nonaApp::OnExit()
{
    DEBUG_TRACE("");
    return 0;
}

void nonaApp::usage(const char * name)
{
    ostringstream o;
    o    << name << ": stitch a panorama image" << std::endl
         << std::endl
         << " It uses the transform function from PanoTools, the stitching itself" << std::endl
         << " is quite simple, no seam feathering is done." << std::endl
         << " all interpolators of panotools are supported" << std::endl
         << std::endl
         << " the \"TIFF_mask\" output will produce a multilayer TIFF file" << std::endl
         << std::endl
         << "Usage: " << name  << " -o output project_file" << std::endl;
    wxMessageBox(o.str().c_str(), _("Error using standalone stitcher"));
}


// make wxwindows use this class as the main application
IMPLEMENT_APP(nonaApp)
