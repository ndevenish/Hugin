// -*- c-basic-offset: 4 -*-
/** @file nona.cpp
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

#include <hugin_config.h>
#include <hugin_version.h>
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
#include <algorithms/nona/NonaFileStitcher.h>
#include <vigra_ext/MultiThreadOperations.h>

#include <tiffio.h>

using namespace vigra;
using namespace HuginBase;
using namespace hugin_utils;
using namespace std;

static void usage(const char * name)
{
    cerr << name << ": stitch a panorama image" << std::endl
    << std::endl
    << "nona version " << PACKAGE_VERSION << std::endl
    << std::endl
    << " It uses the transform function from PanoTools, the stitching itself" << std::endl
    << " is quite simple, no seam feathering is done." << std::endl
    << " only the non-antialiasing interpolators of panotools are supported" << std::endl
    << std::endl
    << " The following output formats (n option of panotools p script line)" << std::endl
    << " are supported:"<< std::endl
    << std::endl
    << "  JPG, TIFF, PNG  : Single image formats without feathered blending:"<< std::endl
    << "  TIFF_m          : multiple tiff files"<< std::endl
    << "  TIFF_multilayer : Multilayer tiff files, readable by The Gimp 2.0" << std::endl
    << std::endl
    << "Usage: " << name  << " [options] -o output project_file (image files)" << std::endl
    << "  Options: " << std::endl
    << "      -c         create coordinate images (only TIFF_m output)" << std::endl
    << "      -v         verbose, output progress indicators" << std::endl
    << "      -t num     number of threads to be used (default: nr of available cores)" << std::endl
    << std::endl
    << "  The following options can be used to override settings in the project file:" << std::endl
    << "      -i num     remap only image with number num" << std::endl
    << "                   (can be specified multiple times)" << std::endl
    << "      -m str     set output file format (TIFF, TIFF_m, TIFF_multilayer, EXR, EXR_m)" << std::endl
    << "      -r ldr/hdr set output mode." << std::endl
    << "                   ldr  keep original bit depth and response" << std::endl
    << "                   hdr  merge to hdr" << std::endl
    << "      -e exposure set exposure for ldr mode" << std::endl
    << "      -p TYPE    pixel type of the output. Can be one of:" << std::endl
    << "                  UINT8   8 bit unsigned integer" << std::endl
    << "                  UINT16  16 bit unsigned integer" << std::endl
    << "                  INT16   16 bit signed integer" << std::endl
    << "                  UINT32  32 bit unsigned integer" << std::endl
    << "                  INT32   32 bit signed integer" << std::endl
    << "                  FLOAT   32 bit floating point" << std::endl
    << "      -z         set compression type." << std::endl
    << "                  Possible options for tiff output:" << std::endl
    << "                   NONE      no compression" << std::endl
    << "                   LZW       lzw compression" << std::endl
    << "                   DEFLATE   deflate compression" << std::endl
    << std::endl;
}

int main(int argc, char *argv[])
{
    
    // parse arguments
    const char * optstring = "z:cho:i:t:m:p:r:e:v";
    int c;
    
    opterr = 0;
    
    int nThread = getCPUCount();
    if (nThread < 0) nThread = 1;
    bool doCoord = false;
    UIntSet outputImages;
    string basename;
    string outputFormat;
    bool overrideOutputMode = false;
    std::string compression("NONE");
    PanoramaOptions::OutputMode outputMode = PanoramaOptions::OUTPUT_LDR;
    bool overrideExposure = false;
    double exposure=0;
    int verbose = 0;
    string outputPixelType;
    
    while ((c = getopt (argc, argv, optstring)) != -1)
    {
        switch (c) {
            case 'o':
                basename = optarg;
                break;
            case 'c':
                doCoord = true;
                break;
            case 'i':
                outputImages.insert(atoi(optarg));
                break;
            case 'm':
                outputFormat = optarg;
                break;
            case 'p':
                outputPixelType = optarg;
                break;
            case 'r':
                if (string(optarg) == "ldr") {
                    overrideOutputMode = true;
                    outputMode = PanoramaOptions::OUTPUT_LDR;
                } else if (string(optarg) == "hdr") {
                    overrideOutputMode = true;
                    outputMode = PanoramaOptions::OUTPUT_HDR;
                } else {
                    usage(argv[0]);
                    return 1;
                }
                break;
            case 'e':
                overrideExposure = true;
                exposure = atof(optarg);
                break;
            case '?':
            case 'h':
                usage(argv[0]);
                return 1;
            case 't':
                nThread = atoi(optarg);
                break;
            case 'v':
                ++verbose;
                break;
            case 'z':
                compression = optarg;
                std::transform(compression.begin(), compression.end(), compression.begin(), (int(*)(int)) toupper);
                break;
            default:
                abort ();
        }
    }

    if (basename == "" || argc - optind <1) {
        usage(argv[0]);
        return 1;
    }
    unsigned nCmdLineImgs = argc -optind -1;

    if (nThread == 0) nThread = 1;
    vigra_ext::ThreadManager::get().setNThreads(nThread);

    const char * scriptFile = argv[optind];

    // suppress tiff warnings
    TIFFSetWarningHandler(0);

    AppBase::ProgressDisplay* pdisp = NULL;
    if(verbose > 0)
        pdisp = new AppBase::StreamProgressDisplay(cout);
    else
        pdisp = new AppBase::DummyProgressDisplay;

    Panorama pano;
    ifstream prjfile(scriptFile);
    if (prjfile.bad()) {
        cerr << "could not open script : " << scriptFile << std::endl;
        exit(1);
    }
    pano.setFilePrefix(hugin_utils::getPathPrefix(scriptFile));
    AppBase::DocumentData::ReadWriteError err = pano.readData(prjfile);
    if (err != AppBase::DocumentData::SUCCESSFUL) {
        cerr << "error while parsing panos tool script: " << scriptFile << std::endl;
        exit(1);
    }

    if ( nCmdLineImgs > 0) {
        if (nCmdLineImgs != pano.getNrOfImages()) {
            cerr << "not enought images specified on command line\nProject required " << pano.getNrOfImages() << " but only " << nCmdLineImgs << " where given" << std::endl;
            exit(1);
        }
        for (unsigned i=0; i < pano.getNrOfImages(); i++) {
            pano.setImageFilename(i, argv[optind+i+1]);
        }

    }
    PanoramaOptions  opts = pano.getOptions();

    opts.tiffCompression=compression;

    // save coordinate images, if requested
    opts.saveCoordImgs = doCoord;
    if (outputFormat == "TIFF_m") {
        opts.outputFormat = PanoramaOptions::TIFF_m;
    } else if (outputFormat == "TIFF") {
        opts.outputFormat = PanoramaOptions::TIFF;
    } else if (outputFormat == "TIFF_multilayer") {
        opts.outputFormat = PanoramaOptions::TIFF_multilayer;
    } else if (outputFormat == "EXR_m") {
        opts.outputFormat = PanoramaOptions::EXR_m;
    } else if (outputFormat == "EXR") {
        opts.outputFormat = PanoramaOptions::EXR;
    } else if (outputFormat != "") {
        cerr << "Error: unknown output format: " << outputFormat << endl;
        return 1;
    }

    if (outputPixelType.size() > 0) {
        opts.outputPixelType = outputPixelType;
    }
    
    if (overrideOutputMode) {
        opts.outputMode = outputMode;
    }
    
    if (overrideExposure) {
        opts.outputExposureValue = exposure;
    }
    
    DEBUG_DEBUG("output basename: " << basename);
    
    pano.setOptions(opts);
    
    try {
        // stitch panorama
        if (outputImages.size() == 0 ) {
            outputImages = pano.getActiveImages();
        }
        NonaFileOutputStitcher(pano, pdisp, opts, outputImages, basename).run();
        // add a final newline, after the last progress message
        if (verbose > 0) {
            cout << std::endl;
        }

    } catch (std::exception & e) {
        cerr << "caught exception: " << e.what() << std::endl;
        return 1;
    }
    
    if(pdisp != NULL)
        delete pdisp;

    return 0;
}
