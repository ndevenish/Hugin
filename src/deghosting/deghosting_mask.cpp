
/**
 * Tool for creating b/w alpha masks to eliminate ghosting artifacts
 * Copyright (C) 2009  Lukáš Jirkovský <l.jirkovsky@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 *Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <iostream>
#include <string>
#include <cstring>
#include <cstdio>

#include <hugin_config.h>
// for stripExtension
#include <hugin_utils/utils.h>
// for exportImage
#include <vigra/impex.hxx>

#include "deghosting.h"
#include "support.h"
#include "threshold.h"
#include "denoise.h"

// deghosting algorithms
#include "khan.h"

#include <getopt.h>
#ifdef _WIN32
    #define snprintf _snprintf
#endif

// options for otherFlags
static const uint16_t SAVE_GENWEIGHTS = 1;
static const uint16_t OTHER_GRAY = 2;

// globals containing settings
static int iterations = 4;
static double sigma = 30;
static uint16_t flags = deghosting::ADV_ONLYP + deghosting::ADV_MULTIRES;
static uint16_t otherFlags = 0;
static uint16_t otherThresholdFlags = 0;
static uint16_t debugFlags = 0;
static deghosting::EMoR response(0.0f);
static int verbosity = 0;
static double thresholdLim = 150;
static double contrast = 1.3;

/** function handling advanced options
 */
void parseOptions_advanced(char* optarg) {
    for(char *c = optarg; *c; c++) {
        switch(*c) {
            case 'f':
                otherFlags += OTHER_GRAY;
                break;
            case 'g':
                flags += deghosting::ADV_GAMMA;
                break;
            case 'm':
                flags -= deghosting::ADV_MULTIRES;
                break;
            case 't':
                otherThresholdFlags += THRESHOLD_DONTCARE;
                break;
            case 'w':
                flags -= deghosting::ADV_ONLYP;
                break;
            default:
                std::cerr<< "Error: unknown option" << std::endl;
                exit(1);
        }
    }
}

/** function handling save options
 */
void parseOptions_save(char* optarg) {
    for(char *c = optarg; *c; c++) {
        switch(*c) {
            case 'i':
                debugFlags += deghosting::SAVE_INITWEIGHTS;
                break;
            case 'w':
                otherFlags += SAVE_GENWEIGHTS;
                break;
            default:
                std::cerr<< "Error: unknown option" << std::endl;
                exit(1);
        }
    }
}

static void usage()
{
    std::cerr << "deghosting_mask: creates mask for removing ghosting in images" << std::endl
         << "deghosting_mask version " << hugin_utils::GetHuginVersion() << std::endl
         << std::endl
         << "Usage: deghosting_mask [options] inputfile(s) " << std::endl
         << "   option are: " << std::endl
         << "     -o, --output=PREFIX       prefix for output masks" << std::endl
         << "     -i, --iterations=ITER     number of iterations, default is (ITER > 0)" << std::endl
         << "                               default: " << iterations << std::endl
         << "     -s, --sigma=SIGMA         standard deviation of Gaussian weighting" << std::endl
         << "                               function (SIGMA > 0); default: " << sigma << std::endl
         //<< "     -r, --response=E:M:o:R    use camera response specified in EMoR format" << std::endl
         << "     -t, --threshold=THRESH    threshold; default: " << thresholdLim << std::endl
         << "     -c, --contrast=CONTR      change contrast before applying threshold;" << std::endl
         << "                               default: " << contrast << std::endl
         << "     -a, --advanced=SET        advanced settings. Possible options are:" << std::endl
         << "                               f   use gray images for computation. It's about two times faster" << std::endl
         << "                                   but it usually returns worse results." << std::endl
         << "                                   You also have to change threshold to smaller value (around 100)" << std::endl
         << "                               g   use gamma 2.2 correction instead of logarithm if input images are HDR" << std::endl
         << "                               m   do not scale image, NOTE: slows down process" << std::endl
         << "                               t   use simple threshold, may result in holes in images" << std::endl
         << "                               w   compute \"complete\" weights, not only probabilities" << std::endl
         << "     -w, --save=SET            advanced save settings" << std::endl
         << "                               i   save initial weights" << std::endl
         << "                               w   save generated weights" << std::endl
         << "     -h, --help                display this help" << std::endl
         << "     -v, --verbose             verbose, repeat for more verbose output" << std::endl;
}

int main(int argc, char *argv[]) {
    try
    {
        const char * optstring = "o:i:s:r:t:c:a:w:hv";
        opterr = 0;
        int c;

        std::string outputPrefix = "weight";

        enum optionArgumentKind {
            NoArgument,
            StringArgument,
            DoubleArgument,
            IntegerArgument,
            ArrayArgument
        };

        enum optionId {
            outputID,
            iterationsID,
            sigmaID,
            responseID,
            thresholdID,
            contrastID,
            advancedID,
            saveID,
            helpID,
            verboseID
        };

        static struct option longOptions[] = {
            { "output", 1, 0, StringArgument },
            { "iterations", 1, 0, IntegerArgument },
            { "sigma", 1, 0, DoubleArgument },
            { "response", 1, 0, ArrayArgument },
            { "threshold", 1, 0, DoubleArgument },
            { "contrast", 1, 0, DoubleArgument },
            { "advanced", 1, 0, StringArgument },
            { "save", 1, 0, StringArgument },
            { "help", 0, 0, NoArgument },
            { "verbose", 0, 0, NoArgument },
            { 0, 0, 0, 0 }
        };

        // TEST
        // response for testing
        response.resize(5);
        response[0] = -3.59f;
        response[1] = -0.93f;
        response[2] = 0.11f;
        response[3] = -0.22f;
        response[4] = 0.34f;

        int optionIndex = 0;

        while ((c = getopt_long(argc, argv, optstring, longOptions, &optionIndex)) != -1) {
            switch (c) {
                case NoArgument: {
                        if (longOptions[optionIndex].flag != 0) break;
                        switch (optionIndex) {
                            case helpID:
                                usage();
                                return 0;
                            case verboseID:
                                verbosity++;
                                break;
                            default:
                                std::cerr << "There's a problem with parsing options" << std::endl;
                                return 1;
                        }
                        break;
                    }

                case StringArgument: {
                        if (longOptions[optionIndex].flag != 0) break;
                        switch (optionIndex) {
                            case outputID:
                                outputPrefix = optarg;
                                break;
                            case advancedID:
                                parseOptions_advanced(optarg);
                                break;
                            case saveID:
                                parseOptions_save(optarg);
                                break;
                            default:
                                std::cerr << "There's a problem with parsing options" << std::endl;
                                return 1;
                        }
                        break;
                    }

                case IntegerArgument: {
                        if (longOptions[optionIndex].flag != 0) break;
                        switch (optionIndex) {
                            case iterationsID:
                                iterations = atoi(optarg);
                                break;
                            default:
                                std::cerr << "There's a problem with parsing options" << std::endl;
                                return 1;
                        }
                        break;
                    }

                case DoubleArgument: {
                        if (longOptions[optionIndex].flag != 0) break;
                        switch (optionIndex) {
                            case sigmaID:
                                sigma = atof(optarg);
                                break;
                            case thresholdID:
                                thresholdLim = atof(optarg);
                                break;
                            case contrastID:
                                contrast = atof(optarg);
                                break;
                        }
                        break;
                    }

                case ArrayArgument: {
                        if (longOptions[optionIndex].flag != 0) break;
                        switch (optionIndex) {
                            case responseID:
                                // TODO
                                break;
                        }
                        break;
                    }

                case 'o':
                    outputPrefix = optarg;
                    break;
                case 'i':
                    iterations = atoi(optarg);
                    break;
                case 's':
                    sigma = atof(optarg);
                    break;
                case 'r':
                    // TODO
                    break;
                case 't':
                    thresholdLim = atof(optarg);
                    break;
                case 'c':
                    contrast = atof(optarg);
                    break;
                case 'a':
                    parseOptions_advanced(optarg);
                    break;
                case 'w':
                    parseOptions_save(optarg);
                    break;
                case 'h':
                    usage();
                    return 0;
                case 'v':
                    verbosity++;
                    break;
            }
        }

        std::cout << std::endl;

        unsigned nFiles = argc - optind;
        if (nFiles == 0)
        {
            std::cerr << std::endl << "Error: at least three input images needed" << std::endl << std::endl;
            usage();
            return 1;
        }
        else
        {
            if (nFiles < 3)
            {
                std::cout << std::endl << "Error: You have to specify at least three images." << std::endl;
                return 1;
            }
        }

        // load all images
        std::vector<std::string> inputFiles;
        for (size_t i = optind; i < (size_t)argc; i++)
        {
            inputFiles.push_back(argv[i]);
        }

        deghosting::Deghosting* deghoster = NULL;

        std::vector<deghosting::FImagePtr> weights;
        if (otherFlags & OTHER_GRAY)
        {
            deghosting::Khan<float> khanDeghoster(inputFiles, flags, debugFlags, iterations, sigma, verbosity);
            deghoster = &khanDeghoster;
            weights = deghoster->createWeightMasks();
        }
        else
        {
            deghosting::Khan<vigra::RGBValue<float> > khanDeghoster(inputFiles, flags, debugFlags, iterations, sigma, verbosity);
            deghoster = &khanDeghoster;
            weights = deghoster->createWeightMasks();
        }

        //deghoster->setCameraResponse(response);

        // save weights
        if (otherFlags & SAVE_GENWEIGHTS)
        {
            for (unsigned int i = 0; i<weights.size(); ++i)
            {
                char tmpfn[100];
                snprintf(tmpfn, 99, "%s_%u.tif", outputPrefix.c_str(), i);
                vigra::ImageExportInfo exWeights(tmpfn);
                vigra::exportImage(srcImageRange(*weights[i]), exWeights.setPixelType("UINT8"));
            }
        }

        // apply contrast functor
        for (unsigned int i = 0; i < weights.size(); ++i)
        {
            vigra::transformImage(vigra::srcImageRange(*(weights[i])), vigra::destImage(*(weights[i])), vigra::BrightnessContrastFunctor<vigra::FImage::PixelType>(1, contrast, 0, 255));
        }

        std::vector<deghosting::BImagePtr> thresholded = threshold(weights, thresholdLim, otherThresholdFlags);

        // save masks with treshold applied
        for (unsigned int i = 0; i<weights.size(); ++i)
        {
            char tmpfn[100];
            std::string fileName = hugin_utils::stripExtension(inputFiles[i]);
            fileName = hugin_utils::stripPath(fileName);
            snprintf(tmpfn, 99, "%s_mask.tif", fileName.c_str());
            vigra::ImageExportInfo exWeights(tmpfn);
            vigra::BImage outImg = vigra::BImage((*thresholded[i]).size());
            simpleDenoise(vigra::srcImageRange(*thresholded[i]), destImage(outImg));
            vigra::exportImage(vigra::srcImageRange(outImg), exWeights.setPixelType("UINT8"));
        }
    }
    catch (const std::exception & e)
    {
        std::cerr << "caught exception: " << e.what() << std::endl;
        return 1;
    } catch (...)
    {
        std::cerr << "caught unknown exception" << std::endl;
        return 1;
    }
    return 0;
}
