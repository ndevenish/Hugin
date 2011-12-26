// -*- c-basic-offset: 4 -*-
/** @file pto2mk.cpp
 *
 *  @brief create a makefile out of a pto file
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: nona.cpp 2893 2008-02-18 18:11:56Z dangelo $
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
#include <../hugin1/hugin/config_defaults.h>

#include <fstream>
#include <sstream>

#include <algorithm>
#include <cctype>
#include <string>

#include <vigra/error.hxx>
#include <vigra/impex.hxx>

#ifdef WIN32
#include <getopt.h>
#else
#include <unistd.h>
#endif

#include <hugin_basic.h>
#include <hugin_utils/platform.h>
#include <algorithms/panorama_makefile/PanoramaMakefilelibExport.h>

#include <tiffio.h>


using namespace vigra;
using namespace HuginBase;
using namespace hugin_utils;
using namespace std;

static void usage(const char * name)
{
    cerr << name << ": create a makefile for stitching" << std::endl
            << std::endl
            << "pto2mk version " << DISPLAY_VERSION << std::endl
            << std::endl
            << "Usage: " << name  << " -o <output_makefile> -p <output_prefix> project_file" << std::endl
            << "  Options: " << std::endl
            << "      -o file           output makefile" << std::endl
            << "      -p output_prefix  prefix of output panorama" << std::endl
            << std::endl;
}

int main(int argc, char *argv[])
{

    const char * optstring = "ho:p:";
    int c;

    opterr = 0;
    std::string mkfile;
    std::string prefix;
    while ((c = getopt (argc, argv, optstring)) != -1)
    {
        switch (c) {
            case 'o':
                mkfile = optarg;
                break;
            case 'p':
                prefix = optarg;
                break;
            case 'h':
                usage(argv[0]);
                return 0;
            default:
                usage(argv[0]);
                abort ();
        }
    }

    if (prefix == "" || mkfile == "") {
        std::cerr << "Please specify output makefile and prefix" << std::endl;
        usage(argv[0]);
        return 1;
    }

    unsigned nCmdLineImgs = argc - optind;
    cout << "number of cmdline args: " << nCmdLineImgs << endl;
    if ( nCmdLineImgs != 1) {
        std::cerr << "No project file given" << std::endl;
        usage(argv[0]);
        return 1;
    }

    const char * ptoFile = argv[optind];
    Panorama pano;
    ifstream prjfile(ptoFile);
    if (prjfile.bad()) {
        cerr << "could not open script : " << ptoFile << std::endl;
        exit(1);
    }
    pano.setFilePrefix(hugin_utils::getPathPrefix(ptoFile));
    AppBase::DocumentData::ReadWriteError err = pano.readData(prjfile);
    if (err != AppBase::DocumentData::SUCCESSFUL) {
        cerr << "error while parsing panos tool script: " << ptoFile << std::endl;
        exit(1);
    }

    // todo: populate from user preferences?
    HuginBase::PanoramaMakefilelibExport::PTPrograms progs;

    progs.exiftool_opts = HUGIN_EXIFTOOL_COPY_ARGS;
    // stitch only active images
    UIntSet activeImgs = pano.getActiveImages();

    PanoramaOptions  opts = pano.getOptions();

    std::ofstream makeFileStream(mkfile.c_str());
    if (!makeFileStream.good()) {
        std::cerr << "Could not open output makefile" << std::endl;
        return 1;
    }

    std::vector<std::string> outputFiles;
    HuginBase::PanoramaMakefilelibExport::createMakefile(pano,
            activeImgs,
            ptoFile,
            prefix,
            progs,
            "",
            outputFiles,
            makeFileStream,
            "",
            true,
            0);

    return 0;
}
