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

#include <getopt.h>
#ifndef WIN32
#include <unistd.h>
#endif

#include <hugin_basic.h>
#include <hugin_utils/platform.h>
#include <algorithms/nona/NonaFileStitcher.h>
#include <vigra_ext/ImageTransformsGPU.h>
#include "hugin_utils/stl_utils.h"

#include <tiffio.h>

using namespace vigra;
using namespace HuginBase;
using namespace hugin_utils;
using namespace std;

static void usage(const char* name)
{
    cerr << name << ": stitch a panorama image" << std::endl
         << std::endl
         << "nona version " << DISPLAY_VERSION << std::endl
         << std::endl
         << " It uses the transform function from PanoTools, the stitching itself" << std::endl
         << " is quite simple, no seam feathering is done." << std::endl
         << " only the non-antialiasing interpolators of panotools are supported" << std::endl
         << std::endl
         << " The following output formats (n option of panotools p script line)" << std::endl
         << " are supported:"<< std::endl
         << std::endl
         << "  JPG, TIFF, PNG  : Single image formats without feathered blending:"<< std::endl
         << "  JPG_m, TIFF_m, PNG_m : multiple image files"<< std::endl
         << "  TIFF_multilayer : Multilayer tiff files, readable by The Gimp 2.0" << std::endl
         << std::endl
         << "Usage: " << name  << " [options] -o output project_file (image files)" << std::endl
         << "  Options: " << std::endl
         << "      -c         create coordinate images (only TIFF_m output)" << std::endl
         << "      -v         quiet, do not output progress indicators" << std::endl
         << "      -d         print detailed output for gpu processing" << std::endl
         << "      -g         perform image remapping on the GPU" << std::endl
         << std::endl
         << "  The following options can be used to override settings in the project file:" << std::endl
         << "      -i num     remap only image with number num" << std::endl
         << "                   (can be specified multiple times)" << std::endl
         << "      -m str     set output file format (TIFF, TIFF_m, TIFF_multilayer," << std::endl
         << "                    EXR, EXR_m, JPEG_m, PNG_m)" << std::endl
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
         << "                   PACKBITS  packbits compression" << std::endl
         << "                   LZW       lzw compression" << std::endl
         << "                   DEFLATE   deflate compression" << std::endl
         << "      --ignore-exposure  don't correct exposure" << std::endl
         << "                   (this does not work with -e switch together)" << std::endl
         << std::endl;
}

int main(int argc, char* argv[])
{

    // parse arguments
    const char* optstring = "z:cho:i:t:m:p:r:e:vgd";
    int c;

    opterr = 0;

    bool doCoord = false;
    UIntSet outputImages;
    string basename;
    string outputFormat;
    bool overrideOutputMode = false;
    std::string compression;
    PanoramaOptions::OutputMode outputMode = PanoramaOptions::OUTPUT_LDR;
    bool overrideExposure = false;
    double exposure=0;
    bool ignoreExposure = false;
    int verbose = 0;
    bool useGPU = false;
    string outputPixelType;

    enum
    {
        IGNOREEXPOSURE=1000
    };
    static struct option longOptions[] =
    {
        { "ignore-exposure", no_argument, NULL, IGNOREEXPOSURE },
        0
    };
    
    int optionIndex = 0;
    while ((c = getopt_long(argc, argv, optstring, longOptions, &optionIndex)) != -1)
    {
        switch (c)
        {
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
                if (string(optarg) == "ldr")
                {
                    overrideOutputMode = true;
                    outputMode = PanoramaOptions::OUTPUT_LDR;
                }
                else if (string(optarg) == "hdr")
                {
                    overrideOutputMode = true;
                    outputMode = PanoramaOptions::OUTPUT_HDR;
                }
                else
                {
                    usage(hugin_utils::stripPath(argv[0]).c_str());
                    return 1;
                }
                break;
            case 'e':
                overrideExposure = true;
                exposure = atof(optarg);
                break;
            case IGNOREEXPOSURE:
                ignoreExposure = true;
                break;
            case '?':
            case 'h':
                usage(hugin_utils::stripPath(argv[0]).c_str());
                return 0;
            case 't':
                std::cout << "WARNING: Switch -t is deprecated. Set environment variable OMP_NUM_THREADS instead" << std::endl;
                break;
            case 'v':
                ++verbose;
                break;
            case 'z':
                compression = optarg;
                compression=hugin_utils::toupper(compression);
                break;
            case 'g':
                useGPU = true;
                break;
            case 'd':
                vigra_ext::SetGPUDebugMessages(true);
                break;
            default:
                usage(hugin_utils::stripPath(argv[0]).c_str());
                abort ();
        }
    }

    if (basename == "" || argc - optind <1)
    {
        usage(hugin_utils::stripPath(argv[0]).c_str());
        return 1;
    }
    unsigned nCmdLineImgs = argc -optind -1;

    const char* scriptFile = argv[optind];

    // suppress tiff warnings
    TIFFSetWarningHandler(0);

    AppBase::ProgressDisplay* pdisp = NULL;
    if(verbose > 0)
    {
        pdisp = new AppBase::StreamProgressDisplay(cout);
    }
    else
    {
        pdisp = new AppBase::DummyProgressDisplay;
    }

    Panorama pano;
    ifstream prjfile(scriptFile);
    if (prjfile.bad())
    {
        cerr << "could not open script : " << scriptFile << std::endl;
        exit(1);
    }
    pano.setFilePrefix(hugin_utils::getPathPrefix(scriptFile));
    AppBase::DocumentData::ReadWriteError err = pano.readData(prjfile);
    if (err != AppBase::DocumentData::SUCCESSFUL)
    {
        cerr << "error while parsing panos tool script: " << scriptFile << std::endl;
        exit(1);
    }

    if ( nCmdLineImgs > 0)
    {
        if (nCmdLineImgs != pano.getNrOfImages())
        {
            cerr << "Incorrect number of images specified on command line\nProject required " << pano.getNrOfImages() << " but " << nCmdLineImgs << " where given" << std::endl;
            exit(1);
        }
        for (unsigned i=0; i < pano.getNrOfImages(); i++)
        {
            pano.setImageFilename(i, argv[optind+i+1]);
        }

    }
    PanoramaOptions  opts = pano.getOptions();

    if (compression.size() > 0)
    {
        opts.tiffCompression=compression;
    }

    // save coordinate images, if requested
    opts.saveCoordImgs = doCoord;
    if (outputFormat == "TIFF_m")
    {
        opts.outputFormat = PanoramaOptions::TIFF_m;
    }
    else if (outputFormat == "JPEG_m")
    {
        opts.outputFormat = PanoramaOptions::JPEG_m;
        opts.tiff_saveROI = false;
    }
    else if (outputFormat == "PNG_m")
    {
        opts.outputFormat = PanoramaOptions::PNG_m;
        opts.tiff_saveROI = false;
    }
    else if (outputFormat == "TIFF")
    {
        opts.outputFormat = PanoramaOptions::TIFF;
    }
    else if (outputFormat == "TIFF_multilayer")
    {
        opts.outputFormat = PanoramaOptions::TIFF_multilayer;
    }
    else if (outputFormat == "EXR_m")
    {
        opts.outputFormat = PanoramaOptions::EXR_m;
    }
    else if (outputFormat == "EXR")
    {
        opts.outputFormat = PanoramaOptions::EXR;
    }
    else if (outputFormat != "")
    {
        cerr << "Error: unknown output format: " << outputFormat << endl;
        return 1;
    }

    if (outputPixelType.size() > 0)
    {
        opts.outputPixelType = outputPixelType;
    }

    if (overrideOutputMode)
    {
        opts.outputMode = outputMode;
    }

    if (overrideExposure)
    {
        opts.outputExposureValue = exposure;
        if (ignoreExposure)
        {
            ignoreExposure = false;
            std::cout << "WARNING: Switches --ignore-exposure and -e can't to used together." << std::endl
                << "         Ignore switch --ignore-exposure." << std::endl;
        }
    }

    if(outputImages.size()==0)
    {
        outputImages = pano.getActiveImages();
    }
    else
    {
        UIntSet activeImages=pano.getActiveImages();
        for(UIntSet::const_iterator it=outputImages.begin(); it!=outputImages.end(); ++it)
        {
            if(!set_contains(activeImages,*it))
            {
                std::cerr << "The project file does not contains an image with number " << *it << std::endl;
                return 1;
            };
        };
    };
    if(outputImages.size()==0)
    {
        std::cout << "Project does not contain active images." << std::endl
                  << "Nothing to do for nona." << std::endl;
        return 0;
    };
    if(useGPU)
    {
        switch(opts.getProjection())
        {
            // the following projections are not supported by nona-gpu
            case HuginBase::PanoramaOptions::BIPLANE:
            case HuginBase::PanoramaOptions::TRIPLANE:
            case HuginBase::PanoramaOptions::PANINI:
            case HuginBase::PanoramaOptions::EQUI_PANINI:
            case HuginBase::PanoramaOptions::GENERAL_PANINI:
                useGPU=false;
                std::cout << "Nona-GPU does not support this projection. Switch to CPU calculation."<<std::endl;
                break;
        };
    };

    DEBUG_DEBUG("output basename: " << basename);

    try
    {
        if (useGPU)
        {
            useGPU = initGPU(&argc, argv);
        }
        opts.remapUsingGPU = useGPU;
        pano.setOptions(opts);

        // stitch panorama
        NonaFileOutputStitcher(pano, pdisp, opts, outputImages, basename, ignoreExposure).run();
        // add a final newline, after the last progress message
        if (verbose > 0)
        {
            cout << std::endl;
        }

        if (useGPU)
        {
            wrapupGPU();
        }

    }
    catch (std::exception& e)
    {
        cerr << "caught exception: " << e.what() << std::endl;
        return 1;
    }

    if(pdisp != NULL)
    {
        delete pdisp;
    }

    return 0;
}
