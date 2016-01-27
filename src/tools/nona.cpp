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
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <hugin_config.h>
#include <fstream>
#include <sstream>

#include <algorithm>
#include <cctype>
#include <string>

#include <vigra/error.hxx>

#include <getopt.h>
#ifndef _WIN32
#include <unistd.h>
#endif

#include <hugin_basic.h>
#include "hugin_base/algorithms/basic/LayerStacks.h"
#include <hugin_utils/platform.h>
#include <algorithms/nona/NonaFileStitcher.h>
#include <vigra_ext/ImageTransformsGPU.h>
#include "hugin_utils/stl_utils.h"
#include "nona/StitcherOptions.h"

#include <tiffio.h>

static void usage(const char* name)
{
    std::cerr << name << ": stitch a panorama image" << std::endl
         << std::endl
         << "nona version " << hugin_utils::GetHuginVersion() << std::endl
         << std::endl
         << " It uses the transform function from PanoTools, the stitching itself" << std::endl
         << " is quite simple, no seam feathering is done." << std::endl
         << " only the non-antialiasing interpolators of panotools are supported" << std::endl
         << std::endl
         << " The following output formats (n option of panotools p script line)" << std::endl
         << " are supported:"<< std::endl
         << std::endl
         << "  JPEG, TIFF, PNG  : Single image formats with internal blender"<< std::endl
         << "  JPEG_m, TIFF_m, PNG_m : multiple image files"<< std::endl
         << "  TIFF_multilayer : Multilayer tiff files, readable by The Gimp 2.0" << std::endl
         << std::endl
         << "Usage: " << name  << " [options] -o output project_file (image files)" << std::endl
         << "  Options: " << std::endl
         << "      -c         create coordinate images (only TIFF_m output)" << std::endl
         << "      -v         verbose output" << std::endl
         << "      -d         print detailed output for gpu processing" << std::endl
         << "      -g         perform image remapping on the GPU" << std::endl
         << std::endl
         << "  The following options can be used to override settings in the project file:" << std::endl
         << "      -i num     remap only image with number num" << std::endl
         << "                   (can be specified multiple times)" << std::endl
         << "      -m str     set output file format (TIFF, TIFF_m, TIFF_multilayer," << std::endl
         << "                    EXR, EXR_m, JPEG, JPEG_m, PNG, PNG_m)" << std::endl
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
         << "      -z|--compression set compression type." << std::endl
         << "                  Possible options for tiff output:" << std::endl
         << "                   NONE      no compression" << std::endl
         << "                   PACKBITS  packbits compression" << std::endl
         << "                   LZW       lzw compression" << std::endl
         << "                   DEFLATE   deflate compression" << std::endl
         << "                  For jpeg output set quality number" << std::endl
         << "      --ignore-exposure  don't correct exposure" << std::endl
         << "                   (this does not work with -e switch together)" << std::endl
         << "      --save-intermediate-images  saves also the intermediate" << std::endl
         << "                   images (only when output is TIFF, PNG or JPEG)" << std::endl
         << "      --intermediate-suffix=SUFFIX  suffix for intermediate images" << std::endl
         << "      --create-exposure-layers  create all exposure layers" << std::endl
         << "                   (this will always use TIFF)" << std::endl
         << "      --clip-exposure[=lower cutoff:upper cutoff]" << std::endl
         << "                   mask automatically all dark and bright pixels" << std::endl
         << "                   optionally you can specifiy the limits for the" << std::endl
         << "                   lower and upper cutoff (specify in range 0...1," << std::endl
         << "                   relative the full range)" << std::endl
         << std::endl;
}

int main(int argc, char* argv[])
{

    // parse arguments
    const char* optstring = "z:cho:i:t:m:p:r:e:vgd";
    int c;

    opterr = 0;

    bool doCoord = false;
    HuginBase::UIntSet outputImages;
    std::string basename;
    std::string outputFormat;
    bool overrideOutputMode = false;
    std::string compression;
    HuginBase::PanoramaOptions::OutputMode outputMode = HuginBase::PanoramaOptions::OUTPUT_LDR;
    bool overrideExposure = false;
    double exposure=0;
    HuginBase::Nona::AdvancedOptions advOptions;
    int verbose = 0;
    bool useGPU = false;
    std::string outputPixelType;
    bool createExposureLayers = false;

    enum
    {
        IGNOREEXPOSURE=1000,
        SAVEINTERMEDIATEIMAGES,
        INTERMEDIATESUFFIX,
        EXPOSURELAYERS,
        MASKCLIPEXPOSURE
    };
    static struct option longOptions[] =
    {
        { "ignore-exposure", no_argument, NULL, IGNOREEXPOSURE },
        { "save-intermediate-images", no_argument, NULL, SAVEINTERMEDIATEIMAGES },
        { "intermediate-suffix", required_argument, NULL, INTERMEDIATESUFFIX },
        { "compression", required_argument, NULL, 'z' },
        { "create-exposure-layers", no_argument, NULL, EXPOSURELAYERS },
        { "clip-exposure", optional_argument, NULL, MASKCLIPEXPOSURE },
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
                if (std::string(optarg) == "ldr")
                {
                    overrideOutputMode = true;
                    outputMode = HuginBase::PanoramaOptions::OUTPUT_LDR;
                }
                else if (std::string(optarg) == "hdr")
                {
                    overrideOutputMode = true;
                    outputMode = HuginBase::PanoramaOptions::OUTPUT_HDR;
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
                HuginBase::Nona::SetAdvancedOption(advOptions, "ignoreExposure", true);
                break;
            case SAVEINTERMEDIATEIMAGES:
                HuginBase::Nona::SetAdvancedOption(advOptions, "saveIntermediateImages", true);
                break;
            case INTERMEDIATESUFFIX:
                HuginBase::Nona::SetAdvancedOption(advOptions, "saveIntermediateImagesSuffix", std::string(optarg));
                break;
            case EXPOSURELAYERS:
                createExposureLayers = true;
                break;
            case MASKCLIPEXPOSURE:
                HuginBase::Nona::SetAdvancedOption(advOptions, "maskClipExposure", true);
                if (optarg != NULL && *optarg != 0)
                {
                    // optional argument given, check if valid
                    std::vector<std::string> tokens = hugin_utils::SplitString(std::string(optarg), ":");
                    if (tokens.size() == 2)
                    {
                        double lowerCutoff;
                        double upperCutoff;
                        if (hugin_utils::stringToDouble(tokens[0], lowerCutoff) && hugin_utils::stringToDouble(tokens[1], upperCutoff))
                        {
                            if (lowerCutoff < 0 || lowerCutoff>1)
                            {
                                std::cerr << "nona: Argument \"" << tokens[0] << "\" is not a valid number for lower cutoff." << std::endl
                                    << "      Aborting." << std::endl;
                                return 1;
                            };
                            if (upperCutoff < 0 || upperCutoff>1)
                            {
                                std::cerr << "nona: Argument \"" << tokens[1] << "\" is not a valid number for upper cutoff." << std::endl
                                    << "      Aborting." << std::endl;
                                return 1;
                            };
                            if (lowerCutoff >= upperCutoff)
                            {
                                std::cerr << "nona: Lower cutoff \"" << tokens[0] << "\" is higher than upper cutoff" << std::endl
                                    << "     \"" << tokens[1] << "\". This is no valid input." << std::endl
                                    << "      Aborting." << std::endl;
                                return 1;
                            };
                            HuginBase::Nona::SetAdvancedOption(advOptions, "maskClipExposureLowerCutoff", static_cast<float>(lowerCutoff));
                            HuginBase::Nona::SetAdvancedOption(advOptions, "maskClipExposureUpperCutoff", static_cast<float>(upperCutoff));
                        }
                        else
                        {
                            std::cerr << "nona: Argument \"" << optarg << "\" is not valid number for --clip-exposure" << std::endl
                                << "      Expected --clip-exposure=lower cutoff:upper cutoff" << std::endl
                                << "      Both should be numbers between 0 and 1."
                                << "      Aborting." << std::endl;
                            return 1;
                        }
                    }
                    else
                    {
                        std::cerr << "nona: Argument \"" << optarg << "\" is not valid for --clip-exposure" << std::endl
                            << "      Expected --clip-exposure=lower cutoff:upper cutoff" << std::endl
                            << "      Aborting." << std::endl;
                        return 1;
                    };
                }
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

    HuginBase::Panorama pano;
    std::ifstream prjfile(scriptFile);
    if (prjfile.bad())
    {
        std::cerr << "could not open script : " << scriptFile << std::endl;
        exit(1);
    }
    pano.setFilePrefix(hugin_utils::getPathPrefix(scriptFile));
    AppBase::DocumentData::ReadWriteError err = pano.readData(prjfile);
    if (err != AppBase::DocumentData::SUCCESSFUL)
    {
        std::cerr << "error while parsing panos tool script: " << scriptFile << std::endl;
        exit(1);
    }

    if ( nCmdLineImgs > 0)
    {
        if (nCmdLineImgs != pano.getNrOfImages())
        {
            std::cerr << "Incorrect number of images specified on command line\nProject required " << pano.getNrOfImages() << " but " << nCmdLineImgs << " where given" << std::endl;
            exit(1);
        }
        for (unsigned i=0; i < pano.getNrOfImages(); i++)
        {
            pano.setImageFilename(i, argv[optind+i+1]);
        }

    }
    HuginBase::PanoramaOptions  opts = pano.getOptions();

    // save coordinate images, if requested
    opts.saveCoordImgs = doCoord;
    if (createExposureLayers)
    {
        if (!outputFormat.empty())
        {
            std::cout << "Warning: Ignoring output format " << outputFormat << std::endl
                << "         Switch --create-exposure-layers will enforce TIFF_m output." << std::endl;
        };
        outputFormat = "TIFF";
        if (!outputImages.empty())
        {
            std::cout << "Warning: Ignoring specified output images." << std::endl
                << "         Switch --create-exposure-layers will always work on all active images." << std::endl;
            outputImages.clear();
        };
    };
    if (outputFormat == "TIFF_m")
    {
        opts.outputFormat = HuginBase::PanoramaOptions::TIFF_m;
        opts.outputImageType = "tif";
    }
    else if (outputFormat == "JPEG_m")
    {
        opts.outputFormat = HuginBase::PanoramaOptions::JPEG_m;
        opts.tiff_saveROI = false;
        opts.outputImageType = "jpg";
    }
    else if (outputFormat == "JPEG")
    {
        opts.outputFormat = HuginBase::PanoramaOptions::JPEG;
        opts.tiff_saveROI = false;
        opts.outputImageType = "jpg";
    }
    else if (outputFormat == "PNG_m")
    {
        opts.outputFormat = HuginBase::PanoramaOptions::PNG_m;
        opts.tiff_saveROI = false;
        opts.outputImageType = "png";
    }
    else if (outputFormat == "PNG")
    {
        opts.outputFormat = HuginBase::PanoramaOptions::PNG;
        opts.tiff_saveROI = false;
        opts.outputImageType = "png";
    }
    else if (outputFormat == "TIFF")
    {
        opts.outputFormat = HuginBase::PanoramaOptions::TIFF;
        opts.outputImageType = "tif";
    }
    else if (outputFormat == "TIFF_multilayer")
    {
        opts.outputFormat = HuginBase::PanoramaOptions::TIFF_multilayer;
        opts.outputImageType = "tif";
    }
    else if (outputFormat == "EXR_m")
    {
        opts.outputFormat = HuginBase::PanoramaOptions::EXR_m;
        opts.outputImageType = "exr";
    }
    else if (outputFormat == "EXR")
    {
        opts.outputFormat = HuginBase::PanoramaOptions::EXR;
        opts.outputImageType = "exr";
    }
    else if (outputFormat != "")
    {
        std::cerr << "Error: unknown output format: " << outputFormat << endl;
        return 1;
    }

    if (!compression.empty())
    {
        if (opts.outputImageType == "tif")
        {
            opts.tiffCompression = compression;
        }
        else
        {
            if (opts.outputImageType == "jpg")
            {
                int q = atoi(compression.c_str());
                if (q > 0 && q <= 100)
                {
                    opts.quality = q;
                }
                else
                {
                    std::cerr << "WARNING: \"" << compression << "\" is not valid compression value for jpeg images." << std::endl
                        << "         Using value " << opts.quality << " found in pto file." << std::endl;
                };
            };
        };
    };

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
        if (HuginBase::Nona::GetAdvancedOption(advOptions, "ignoreExosure", false))
        {
            HuginBase::Nona::SetAdvancedOption(advOptions, "ignoreExposure", false);
            std::cout << "WARNING: Switches --ignore-exposure and -e can't to used together." << std::endl
                << "         Ignore switch --ignore-exposure." << std::endl;
        }
    }

    if (outputImages.empty())
    {
        outputImages = HuginBase::getImagesinROI(pano, pano.getActiveImages());
    }
    else
    {
        HuginBase::UIntSet activeImages = HuginBase::getImagesinROI(pano, pano.getActiveImages());
        for (HuginBase::UIntSet::const_iterator it = outputImages.begin(); it != outputImages.end(); ++it)
        {
            if(!set_contains(activeImages,*it))
            {
                std::cerr << "The project file does not contains an image with number " << *it << std::endl;
                return 1;
            };
        };
    };
    if(outputImages.empty())
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
        AppBase::ProgressDisplay* pdisp = NULL;
        if(verbose > 0)
        {
            pdisp = new AppBase::StreamProgressDisplay(std::cout);
        }
        else
        {
            pdisp = new AppBase::DummyProgressDisplay;
        }

        if (useGPU)
        {
            useGPU = hugin_utils::initGPU(&argc, argv);
        }
        opts.remapUsingGPU = useGPU;
        pano.setOptions(opts);

        if (createExposureLayers)
        {
            HuginBase::UIntSetVector exposureLayers = getExposureLayers(pano, outputImages, opts);
            if (exposureLayers.empty())
            {
                std::cerr << "ERROR: Could not determine exposure layers. Cancel execution." << std::endl;
            }
            else
            {
                // we need to pass the basename to the stitcher
                // because NonaFileOutputStitcher get already filename with numbers added
                HuginBase::Nona::SetAdvancedOption(advOptions, "basename", basename);
                for (size_t i = 0; i < exposureLayers.size(); ++i)
                {
                    HuginBase::PanoramaOptions modOptions(opts);
                    // set output exposure to exposure value of first image of layers
                    // normaly this this invoked with --ignore-exposure, so this has no effect
                    modOptions.outputExposureValue = pano.getImage(*(exposureLayers[i].begin())).getExposureValue();
                    // build filename
                    std::ostringstream filename;
                    filename << basename << std::setfill('0') << std::setw(4) << i;
                    HuginBase::NonaFileOutputStitcher(pano, pdisp, modOptions, exposureLayers[i], filename.str(), advOptions).run();
                }
            }
        }
        else
        {
            // stitch panorama
            HuginBase::NonaFileOutputStitcher(pano, pdisp, opts, outputImages, basename, advOptions).run();
        };
        // add a final newline, after the last progress message
        if (verbose > 0)
        {
            std::cout << std::endl;
        }

        if (useGPU)
        {
            hugin_utils::wrapupGPU();
        }

        if(pdisp != NULL)
        {
            delete pdisp;
            pdisp=NULL;
        }
    }
    catch (std::exception& e)
    {
        std::cerr << "caught exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
