// -*- c-basic-offset: 4 -*-

/** @file stitcher.cpp
 *
 *  @brief a simple test stitcher
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

using namespace vigra;
using namespace PT;
using namespace std;

static void usage(const char * name)
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

int main(int argc, char *argv[])
{

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
            return 1;
        default:
            abort ();
        }

    if (basename == "" || argc - optind <1) {
        usage(argv[0]);
        return 1;
    }

    // strip any extension from output file
    std::string::size_type idx = basename.rfind('.');
    if (idx != std::string::npos) {
        basename = basename.substr(0, idx);
    }

    const char * scriptFile = argv[optind];

    utils::CoutProgressDisplay pdisp;

    Panorama pano;
    PanoramaMemento newPano;
    ifstream prjfile(scriptFile);
    if (prjfile.bad()) {
        cerr << "could not open script : " << scriptFile << endl;
        exit(1);
    }
    if (newPano.loadPTScript(prjfile)) {
        pano.setMemento(newPano);
    } else {
        cerr << "error while parsing panos tool script: " << scriptFile << endl;
        exit(1);
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
        return 1;
    }

    return 0;
}


