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
#include "PT/SimpleStitcher.h"

#include "common/utils.h"
#include "common/stl_utils.h"
#include "PT/Panorama.h"


using namespace vigra;
using namespace PT;
using namespace std;

void usage(char * name)
{
    cerr << name << ": stitch a panorama image, with bilinear interpolation" << endl
         << endl
         << " It uses the transform function from PanoTools, the stitching itself" << endl
         << " is quite simple, no seam feathering is done. Seams are placed" << endl
         << endl
         << "Usage: " << name  << " hugin_project outputimageprefix" << endl;
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        usage(argv[0]);
        exit(1);
    }

    utils::CoutProgressDisplay pdisp;

    char * scriptFile = argv[1];
    // output settings
    string basename(argv[2]);

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
    case PanoramaOptions::TIFF_nomask:
        format = "tif";
        savePartial = true;
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
        if (!savePartial) {
            PTools::stitchPanoramaSimple(pano, pano.getOptions(), dest,
                                         pdisp);
        } else {
            PTools::stitchPanoramaSimple(pano, pano.getOptions(), dest,
                                         pdisp, basename);
        }
        // save final panorama
        string filename = basename + "." + format;
	DEBUG_DEBUG("saving output file: " << filename);
        vigra::ImageExportInfo exinfo(filename.c_str());
        if (format == "jpg") {
            ostringstream jpgqual;
            jpgqual << opts.quality;
            exinfo.setCompression(jpgqual.str().c_str());
        }
        exportImage(srcImageRange(dest), exinfo);
    } catch (std::exception & e) {
        cerr << "caught exception: " << e.what() << endl;
        return 1;
    }

    return 0;
}


