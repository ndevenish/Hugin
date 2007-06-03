// -*- c-basic-offset: 4 -*-

/** @file utils.cpp
 *
 *  @brief various utility functions
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: utils.cpp 1814 2006-12-31 14:37:05Z dangelo $
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

#include <config.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <set>
#include <iterator>
#include <algorithm>
#include <locale.h>
#include <iomanip>

#include <stdio.h>
#include <math.h>
#include <limits.h>

#include <common/stl_utils.h>
#include <common/Matrix3.h>
#include <common/lu.h>
#include <common/eig_jacobi.h>

#include <PT/utils.h>
#include <PT/Panorama.h>


using namespace PT;
using namespace std;
using namespace vigra;
using namespace utils;

void PT::createMakefile(const Panorama & pano,
                        const std::string & ptofile,
                        const std::string & outputPrefix,
                        const PTPrograms & progs,
                        const std::string & includePath,
                        std::ostream & o)

{
    PanoramaOptions opts = pano.getOptions();
#ifdef __unix__
    // set numeric locale to C, for correct number output
    char * t = setlocale(LC_NUMERIC,NULL);
    char * old_locale = (char*) malloc(strlen(t)+1);
    strcpy(old_locale, t);
    setlocale(LC_NUMERIC,"C");
#endif
    o << "# makefile for panorama stitching, created by hugin " << endl
      << endl;

    // this function supports only multiple tiff output.
    // for example using nona or PTmender (or whatever it will be called)

    // set a suitable target file. (only tiff is supported so far)
    std::string output = quoteString(outputPrefix);
    std::string final_output = output + ".tif";

    bool externalBlender = false;
    bool remapToMultiple = false;

    if (opts.blendMode == PT::PanoramaOptions::NO_BLEND) {
        // just remapping or simple blending
        if (opts.outputFormat == PT::PanoramaOptions::TIFF_m) {
            remapToMultiple = true;
        }
    } else {
        externalBlender = true;
        remapToMultiple = true;
    }

    o << "# the output panorama" << endl
            << "TARGET_PREFIX=" << output << endl
            << "TARGET=" << final_output << endl
            << "PROJECT_FILE=" << quoteString(ptofile) << endl
            << endl
      << "# Input images" << endl
      << "INPUT_IMAGES=";
    for (unsigned int i=0; i < pano.getNrOfImages(); i++) {
        o << quoteString(pano.getImage(i).getFilename()) << " ";
    }
    o << endl
      << endl
      << "# remapped images" << endl
      << "REMAPPED_IMAGES=";
    for (unsigned int i=0; i < pano.getNrOfImages(); i++) {
        std::ostringstream fns;
        fns << output << std::setfill('0') << std::setw(4) << i << ".tif";
        o << fns.str() << " ";
    }

    o << endl 
      << endl
      << "# Tool configuration" << endl
      << "NONA=" << quoteString(progs.nona) << endl
      << "PTSTITCHER=" << quoteString(progs.PTStitcher) << endl
      << "PTMENDER=" << quoteString(progs.PTmender) << endl
      << "PTBLENDER=" << quoteString(progs.PTblender) << endl
      << "PTMASKER=" << quoteString(progs.PTmasker) << endl
      << "PTROLLER=" << quoteString(progs.PTroller) << endl
      << "ENBLEND=" << quoteString(progs.enblend) << endl
      << "SMARTBLEND=" << quoteString(progs.smartblend) << endl
      << endl
      << "# options for the programs" << endl;

    string remapper;
    switch(opts.remapper) {
        case PanoramaOptions::NONA:
            remapper = "nona";
            break;
        case PanoramaOptions::PTMENDER:
            remapper = "ptmender";
            break;
    }

    string blender;
    switch(opts.blendMode) {
        case PanoramaOptions::NO_BLEND:
            blender="multilayer";
            break;
        case PanoramaOptions::PTBLENDER_BLEND:
            blender="ptblender";
            // TODO: add options here!
            o << "PTBLENDER_OPTS=";
            switch (opts.colorCorrection) {
                case PanoramaOptions::NONE:
                    blender = "ptroller";
                    break;
                case PanoramaOptions::BRIGHTNESS_COLOR:
                    o << " -k " << opts.colorReferenceImage;
                    break;
                case PanoramaOptions::BRIGHTNESS:
                    o << " -k " << opts.colorReferenceImage;
                    break;
                case PanoramaOptions::COLOR:
                    o << " -k " << opts.colorReferenceImage;
                    break;
            }
            o << endl;
            break;
        case PanoramaOptions::ENBLEND_BLEND:
            blender = "enblend";
            o << "ENBLEND_OPTS=" << progs.enblend_opts;
            if (opts.getHFOV() == 360.0) {
                    // blend over the border
                o << " -w";
            }
            if (opts.tiffCompression == "LZW") {
                o << " -z";
            }
            o << " -f" << opts.getWidth() << "x" << opts.getHeight() << endl;
            o << endl;
            break;
        case PanoramaOptions::SMARTBLEND_BLEND:
            blender = "smartblend";
            o << "SMARTBLEND_OPTS=" << progs.smartblend_opts;
            if (opts.getHFOV() == 360.0) {
                // blend over the border
                o << " -w";
            }
            o << endl;
            // todo: build smartblend command line from given images. (requires additional program)
            break;
    }

    std::string includefile = includePath + remapper + "_" + blender + ".mk";
    o << endl
      << "# including template " << includefile
      << endl;

    std::ifstream templ(includefile.c_str());
    while(templ.good() && (!templ.eof())) {
        std::string line;
        std::getline(templ, line);
        o << line << endl;
    }

#ifdef __unix__
    // reset locale
    setlocale(LC_NUMERIC,old_locale);
    free(old_locale);
#endif

}

