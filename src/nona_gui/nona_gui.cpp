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

#include <fstream>
#include <sstream>

#include <vigra/impex.hxx>
#include <vigra/error.hxx>

#include <unistd.h>

#include "panoinc.h"
#include "PT/SimpleStitcher.h"

#include <wx/app.h>

#include <wx/config.h>              // wx config classes for all systems
#include <wx/image.h>               // wxImage
#include <wx/xrc/xmlres.h>          // XRC XML resouces
#include <wx/filename.h>            // files and dirs
#include <wx/file.h>

#include "hugin/MyProgressDialog.h"


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
    DEBUG_TRACE("ctor");
}

nonaApp::~nonaApp()
{
    DEBUG_TRACE("dtor");
    DEBUG_TRACE("dtor end");
}

bool nonaApp::OnInit()
{
    SetAppName("nona");

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

    if (basename == "" || argc - optind <1) {
        usage(argv[0]);
        return false;
    }

    // strip any extension from output file
    std::string::size_type idx = basename.rfind('.');
    if (idx != std::string::npos) {
        basename = basename.substr(0, idx);
    }

    const char * scriptFile = argv[optind];


    //utils::StreamMultiProgressDisplay pdisp(cout);
    MyProgressDialog pdisp(_("Stitching Panorama"), "", NULL, wxPD_ELAPSED_TIME | wxPD_AUTO_HIDE | wxPD_APP_MODAL );

    Panorama pano;
    PanoramaMemento newPano;
    ifstream prjfile(scriptFile);
    if (prjfile.bad()) {
        cerr << "could not open script : " << scriptFile << endl;
        return false;
    }
    if (newPano.loadPTScript(prjfile)) {
        pano.setMemento(newPano);
    } else {
        cerr << "error while parsing panos tool script: " << scriptFile << endl;
        return false;
    }

    PanoramaOptions  opts = pano.getOptions();

    string format = "jpg";
    bool savePartial = false;
    switch(opts.outputFormat) {
    case PanoramaOptions::JPEG:
        format = "jpg";
        break;
    case PanoramaOptions::PNG:
        format = "png";
        break;
    case PanoramaOptions::TIFF:
        format = "tif";
        break;
    case PanoramaOptions::TIFF_m:
        format = "tif";
        savePartial = true;
        break;
    case PanoramaOptions::TIFF_mask:
        format = "tif";
        break;
    default:
        DEBUG_ERROR("unsupported file format, using jpeg");
        format = "jpg";
    }

    // check for some options

    int w = opts.width;
    int h = opts.getHeight();

    cout << "output image size: " << w << "x" << h << endl;

    try {
        BRGBImage dest;
        // stitch panorama
        PT::stitchPanoramaSimple(pano, pano.getOptions(), dest,
                                 pdisp, basename, format, savePartial);
    } catch (std::exception & e) {
        cerr << "caught exception: " << e.what() << endl;
        return false;
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
    cerr << name << ": stitch a panorama image" << endl
         << endl
         << " It uses the transform function from PanoTools, the stitching itself" << endl
         << " is quite simple, no seam feathering is done." << endl
         << " all interpolators of panotools are supported" << endl
         << endl
         << " the \"TIFF_mask\" output will produce a multilayer TIFF file" << endl
         << endl
         << "Usage: " << name  << " -o output project_file" << endl;
}


// make wxwindows use this class as the main application
IMPLEMENT_APP(nonaApp)
