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

#include <vigra/error.hxx>
#include <vigra/impex.hxx>

#include <unistd.h>

#include "panoinc.h"
#include "PT/Stitcher.h"

#include <tiff.h>

using namespace vigra;
using namespace PT;
using namespace std;

static void usage(const char * name)
{
    cerr << name << ": stitch a panorama image" << std::endl
         << std::endl
         << " It uses the transform function from PanoTools, the stitching itself" << std::endl
         << " is quite simple, no seam feathering is done." << std::endl
         << " all interpolators of panotools are supported" << std::endl
         << std::endl
         << " the \"TIFF_mask\" output will produce a multilayer TIFF file" << std::endl
         << std::endl
         << "Usage: " << name  << " [options] -o output project_file" << std::endl
	 << std::endl
         << "  [options] can be: " << std::endl
         << "     -r          # save tiff files with ROI (saves a lot of space, " << std::endl
	 << "                   but doesn't work with most programs. " << std::endl
	 << "                   The Gimp 2.0 can handle these files" << std::endl
	 << std::endl;

}

int main(int argc, char *argv[])
{

    // parse arguments
    const char * optstring = "rho:";
    int c;

    opterr = 0;

    string basename;
    bool tiffROI = false;

    while ((c = getopt (argc, argv, optstring)) != -1)
        switch (c) {
        case 'o':
            basename = optarg;
            break;
	case 'r':
	    tiffROI = true;
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

    const char * scriptFile = argv[optind];


    // suppress tiff warnings
    TIFFSetWarningHandler(0);

    utils::StreamMultiProgressDisplay pdisp(cout);
    //utils::MultiProgressDisplay pdisp;

    Panorama pano;
    PanoramaMemento newPano;
    ifstream prjfile(scriptFile);
    if (prjfile.bad()) {
        cerr << "could not open script : " << scriptFile << std::endl;
        exit(1);
    }
    if (newPano.loadPTScript(prjfile)) {
        pano.setMemento(newPano);
    } else {
        cerr << "error while parsing panos tool script: " << scriptFile << std::endl;
        exit(1);
    }

    PanoramaOptions  opts = pano.getOptions();
    opts.tiff_saveROI = tiffROI;

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

    return 0;
}
