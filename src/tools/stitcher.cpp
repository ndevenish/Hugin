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

#include "common/utils.h"
#include "common/stl_utils.h"
#include "hugin/ImageCache.h"
#include "hugin/ImageProcessing.h"
#include "PT/Panorama.h"
#include "hugin/PanoToolsInterface.h"

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

    wxInitAllImageHandlers();

    char * scriptFile = argv[1];
    char * panoFile = argv[2];


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
        cerr << "error while parsing pano tool script: " << scriptFile << endl;
        exit(1);
    }

    PanoramaOptions  opts = pano.getOptions();
    
    opts.VFOV = 60;
    int w = opts.width;
    int h = opts.getHeight();

    cout << "output image size: " << w << "x" << h << endl;
    wxImage output(w,h);
    wxImage tmp(w,h);

    unsigned int nImg = pano.getNrOfImages();
    for (unsigned int i=0; i< nImg; i++) {
        PTools::mapImage(tmp, pano, i, opts);
        unsigned char * srcdata = tmp.GetData();
        unsigned char * dstdata = output.GetData();
        for(long i=0; i< w*h*3; i +=3) {
            if (srcdata[i] != 0 || srcdata[i+1] != 0 || srcdata[i+2] != 0) {
                dstdata[i] = srcdata[i];
                dstdata[i+1] = srcdata[i+1];
                dstdata[i+2] = srcdata[i+2];
            }
        }
    }

    if (!output.SaveFile(panoFile)) {
        DEBUG_FATAL("could not save output panorama");
        return 1;
    }
    return 0;
}


