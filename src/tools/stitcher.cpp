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

#include "common/utils.h"
#include "common/stl_utils.h"
#include "PT/Panorama.h"
#include "PT/SimpleStitcher.h"

#include <vigra/impex.hxx>
#include <vigra/error.hxx>

using namespace vigra;
using namespace PT;
using namespace std;

void usage(char * name)
{
    cerr << name << ": stitch a panorama image" << endl
         << endl
         << "Usage: " << name  << " PT_script_file outputimage" << endl;
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        usage(argv[0]);
        exit(1);
    }

    char * scriptFile = argv[1];
    // output settings
    string basename(argv[2]);
    string format = "tif";


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

    int w = opts.width;
    int h = opts.getHeight();

    cout << "output image size: " << w << "x" << h << endl;

    try {

        BRGBImage dest;
        // stitch panorama
        PTools::stitchPanoramaSimple(pano, pano.getOptions(), dest);
        // save final panorama
        string filename = basename + ".png";
        exportImage(srcImageRange(dest), vigra::ImageExportInfo(filename.c_str()));

    } catch (std::runtime_error & e) {
        cerr << "caught runtime_error: " << e.what() << endl;
        return 1;
    }

    return 0;
}


