// -*- c-basic-offset: 4 -*-

/** @file hugin_hdrmerge.cpp
 *
 *  @brief merge images
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: hugin_hdrmerge.cpp,v 1.3 2007/04/18 22:21:42 dangelo Exp $
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
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>

#include <boost/shared_ptr.hpp>

#include <vigra/error.hxx>
#include <vigra/functorexpression.hxx>
#include <vigra_impex/codecmanager.hxx>

#include <hugin_utils/utils.h>

#include <vigra_ext/impexalpha.hxx>
#include <vigra_ext/HDRUtils.h>
#include <vigra_ext/ReduceOpenEXR.h>

//#include <PT/PTOptimise.h>

#ifdef WIN32
 #include <getopt.h>
#else
 #include <unistd.h>
#endif

#include "khan.h"

using namespace std;

using namespace hugin_utils;
using namespace vigra;
using namespace vigra::functor;
using namespace vigra_ext;

int g_verbose = 0;


// load all images and apply a weighted average merge, with
// special cases for completely over or underexposed pixels.
bool mergeWeightedAverage(vector<string> inputFiles, FRGBImage & output)
{
    // load all images into memory
    vector<ImagePtr> images;
    vector<BImagePtr> weightImages;

    for (size_t i=0; i < inputFiles.size(); i++)
    {
        ImagePtr img = ImagePtr(new ImageType());
        BImagePtr weight = BImagePtr(new BImage());

        if (g_verbose > 0) {
            std::cout << "Loading image: " << inputFiles[i] << std::endl;
        }
        vigra::ImageImportInfo info(inputFiles[i].c_str());
        img->resize(info.size());
        weight->resize(info.size());
        vigra::importImageAlpha(info, vigra::destImage(*img), destImage(*weight));
        images.push_back(img);
        weightImages.push_back(weight);
    }

    // ensure all images have the same size (cropped images not supported yet)
    int width=images[0]->width();
    int height=images[0]->height();
    for (unsigned i=1; i < images.size(); i++) {
        if (images[i]->width() != width || images[i]->height() != height) {
            std::cerr << "Error: Input images need to be of the same size" << std::endl;
            return false;
        }
    }
    output.resize(width,height);
    if (g_verbose > 0) {
        std::cout << "Calculating weighted average " << std::endl;
    }
    // apply weighted average functor with
    // heuristic to deal with pixels that are overexposed in all images
    ReduceToHDRFunctor<ImageType::value_type> waverage;

    // loop over all pixels in the image (very low level access)
    for (int y=0; y < height; y++) {
        for (int x=0; x < width; x++) {
            waverage.reset();
            // loop over all exposures
            for (unsigned imgNr=0; imgNr < images.size(); imgNr++) {
                // add pixel to weighted average
                waverage( (*images[imgNr])(x,y), (*weightImages[imgNr])(x,y) );
            }
            // get result
            output(x,y) = waverage();
        }
    }
    return true;
}

static void usage(const char * name)
{
    cerr << name << ": merge overlapping images" << std::endl
         << std::endl
         << "Usage: " << name  << " [options] -o output.exr <input-files>" << std::endl
         << "Valid options are:" << std::endl
         << "  -o prefix output file" << std::endl
         << "  -m mode   merge mode, can be one of: avg, avg_slow, khan (default), if avg, no" << std::endl
		 << "            -i, -s, or -d options apply" << std::endl
		 << "  -i iter   number of iterations to execute (default is 1)" << std::endl
         << "  -c        Only consider pixels that are defined in all images (avg mode only)" << std::endl
		 << "  -s file   debug files to save each iteration, can be one of:" << std::endl
		 << "            a - all debug files (can only be used alone)" << std::endl
		 << "            w - calculated weights from each iteration" << std::endl
		 << "            r - result image from each iteration" << std::endl
		 << "            s - source images before processing" << std::endl
		 << "            if verbose >= 3, all debug files are output unless specified" << std::endl
		 << "  -a calcs  apply one or more advanced caculations, can be one or more of:" << std::endl
		 << "            b - biasing weights logarithmically" << std::endl
		 << "            c - choose pixels with heighest weight instead of averaging" << std::endl
		 << "                (overrides options -a b and -a d)" << endl
		 << "            d - choose a pixel with the heighest weight instead of" << endl
		 << "                averaging when all pixel weights are within 10% of eachother" << endl
		 << "            h - favor a high signal to noise ratio" << std::endl
		 << "            i - ignore alpha channel" << endl
/*		 << "            m - multi-scale calculation of weights" << std::endl
		 << "            s - favor choosing from the same image" << std::endl
		 << "            u - use joint bilateral upscaling" << std::endl
		 << "            ex: -d hms" << std::endl
*/		 << "  -e        export each initial weight to <input_file_paths>_iw.<ext>" << std::endl
		 << "  -l        load a previously exported initial weight with respect " << endl
		 << "            to the input file names" << std::endl
		 << "            NOTE: if both -e and -l options are on, the program will " << endl
		 << "                  calculate and save the initial weights, then wait " << endl
		 << "                  until user indicates that it can continue by loading " << endl
		 << "                  the previously saved weights" << endl
         << "  -v        Verbose, print progress messages, repeat for" << std::endl
         << "            even more verbose output" << std::endl
         << "  -h        Display help (this text)" << std::endl
         << std::endl;
}


int main(int argc, char *argv[])
{

    // parse arguments
    const char * optstring = "chvo:m:i:s:a:el";
    int c;

    opterr = 0;

    g_verbose = 0;
    std::string outputFile = "merged.hdr";
    std::string mode = "khan";
    bool onlyCompleteOverlap = false;
	int num_iters = 1;
	char save = 0;
	char adv = 0;
	char ui = 0;
	
    string basename;
    while ((c = getopt (argc, argv, optstring)) != -1) {
        switch (c) {
        case 'm':
            mode = optarg;
            break;
        case 'c':
            onlyCompleteOverlap = true;
            break;
		case 'i':
			num_iters = atoi(optarg);
			break;
		case 'e':
			ui += UI_EXPORT_INIT_WEIGHTS;
			if(g_verbose > 0)
				cout << "Exporting initial weights" << endl;
			break;
		case 'l':
			ui += UI_IMPORT_INIT_WEIGHTS;
			if(g_verbose > 0)
				cout << "Importing initial weights" << endl;
			break;
		case 's':
			for(char *c = optarg; *c; c++) {
				switch(*c) {
				case 'w':
					save += SAVE_WEIGHTS;
					if(g_verbose > 0)
						cout << "Saving weights from each iteration" << endl;
					break;
				case 'r':
					save += SAVE_RESULTS;
					if(g_verbose > 0)
						cout << "Saving results from each iteration" << endl;
					break;
				case 's':
					save += SAVE_SOURCES;
					if(g_verbose > 0)
						cout << "Saving sources after loading" << endl;
					break;
 				case 'a':
					save = SAVE_ALL;
					if(g_verbose > 0)
						cout << "Saving all debug outputs" << endl;
					break;
				default:
					cerr << "Invalid argument for option -s: " << *c << std::endl;
					usage(argv[0]);
					return 1;
				}
			} 
			break;
		case 'a':
			for(char *c = optarg; *c; c++) {
				switch(*c) {
				case 'b':
					if(adv & ADV_UNAVG) {
						cerr << "Cannot use b with c in option -a" << endl;
						usage(argv[0]);
						return 1;
					}
					adv += ADV_BIAS;
					if(g_verbose > 0)
						cout << "Applying: logarithmic bias of weights" << endl;
					break;
				case 'c':
					if(adv & ADV_BIAS) {
						cout << "Warning: overriding log bias of weights "
						<< "width option -a c" << endl;
						adv -= ADV_BIAS;
					}
					if(adv & ADV_UNAVG2) {
						cout << "Warning: overriding option -a d with option "
								<< "-a c" << endl;
						adv-= ADV_UNAVG2;
					}
					adv += ADV_UNAVG;
					if(g_verbose > 0) {
						cout << "Applying: choose pixel with largest weight" << endl;
					}
					break;
				case 'd':
					if(adv & ADV_UNAVG) {
						cout << "Warning: overriding option -a d with option "
								<< "-a c" << endl;
					}
					else {
						adv+= ADV_UNAVG2;
						cout << "Applying: choosing pixel with the largest weight "
								<< "when weights are similar" << endl;
					}
					break;
				case 'h':
					adv += ADV_SNR;
					if(g_verbose > 0)
						cout << "Applying: favoring high signal to noise ratio"
								<< endl;
					break;
				case 'i':
					adv += ADV_ALPHA;
					if(g_verbose > 0)
						cout << "Applying: ignore alpha channel" << endl;
					break;
/*				case 'm':
					adv += ADV_MULTI;
					if(g_verbose > 0)
						cout << "Applying: multi-scaling" << endl;
					break;
				case 's':
					adv += ADV_SAME;
					if(g_verbose > 0)
						cout << "Applying: favor choosing from the same image" << endl;
					break;
				case 'u':
					adv += ADV_JBU;
					if(g_verbose > 0) {
						cout << "Applying: use joint bilateral upsampling"
						<< endl;
					}
					break;
*/				default:
					cerr << "Invalid argument for option -a: " << *c << std::endl;
					usage(argv[0]);
					return 1;
				}
			}
			break;
       case 'o':
            outputFile = optarg;
            break;
        case 'v':
            g_verbose++;
            break;
        case 'h':
            usage(argv[0]);
            return 1;
        default:
            cerr << "Invalid parameter: " << optarg << std::endl;
            usage(argv[0]);
            return 1;
        }
	}//end while
	
	cout << endl;
	
	if(g_verbose > 2)
		save = SAVE_ALL;

    unsigned nFiles = argc - optind;
    if (nFiles < 2) {
        std::cerr << std::endl << "Error: at least two files need to be specified" << std::endl <<std::endl;
        usage(argv[0]);
        return 1;
    }

    // load all images
    vector<string> inputFiles;
    for (size_t i=optind; i < (size_t)argc; i++)
    {
        inputFiles.push_back(argv[i]);
    }

    // output image
    ImageType output;
    try {
        if (mode == "avg_slow") {
            // use a weighted average, with special consideration of pixels
            // that are completely over or underexposed in all exposures.
            if (g_verbose > 0) {
                cout << "Running simple weighted avg algorithm" << std::endl;
            }

            mergeWeightedAverage(inputFiles, output);
            // save output file
            if (g_verbose > 0) {
                std::cout << "Writing " << outputFile << std::endl;
            }
            ImageExportInfo exinfo(outputFile.c_str());
            BImage alpha(output.width(), output.height(), 255);
            exportImageAlpha(srcImageRange(output), srcImage(alpha), exinfo);
        } else if (mode == "avg") {
            // apply weighted average functor with
            // heuristic to deal with pixels that are overexposed in all images
            ReduceToHDRFunctor<ImageType::value_type> waverage;
            // calc weighted average without loading the whole images into memory
            reduceFilesToHDR(inputFiles, outputFile, onlyCompleteOverlap, waverage);

        } else if (mode == "khan") {
            if (g_verbose > 0) {
                cout << "Running Khan algorithm" << std::endl;
            }
            BImage mask;
            khanMain(inputFiles, output, mask, num_iters, save, adv, ui);
            // save output file
            if (g_verbose > 0) {
                std::cout << "Writing " << outputFile << std::endl;
            }
            ImageExportInfo exinfo(outputFile.c_str());
            exportImageAlpha(srcImageRange(output), srcImage(mask), exinfo);
        } else {
            std::cerr << "Unknown merge mode, see help for a list of possible modes" << std::endl;
            return 1;
        }
    } catch (std::exception & e) {
        cerr << "caught exception: " << e.what() << std::endl;
        abort();
    }
	
    return 0;
}

bool saveImages(std::vector<std::string> prep, std::string app,
				  const std::vector<FImagePtr> &images)
{
	if(!prep.size() || !images.size() || prep.size() != images.size()) {
		cout << "Error: Number of file names doesn't match number of images"
				<< endl;
		return false;
	}
	if(!app.size()) {
		cout << "Error: Cannot append empty string to file names"
				<<endl;
		return false;
	}
	
	for(unsigned i = 0; i < prep.size(); i++) {
		string tmp(prep.at(i));
		tmp.erase(tmp.rfind('.', tmp.length()-1));
		tmp.append(app);
		tmp.append(".jpg");
		ImageExportInfo exinfo(tmp.c_str());
		exportImage(srcImageRange(*images.at(i)), exinfo);
	}
	
	return true;
}

bool loadImages(std::vector<std::string> prep, std::string app,
				  std::vector<FImagePtr> *images)
{
	if(!app.size()) {
		cout << "Error: Cannot append empty string to file names"
				<<endl;
		return false;
	}
	
	for(unsigned i = 0; i < prep.size(); i++) {
		string tmp(prep.at(i));
		tmp.erase(tmp.rfind('.', tmp.length()-1));
		tmp.append(app);
		tmp.append(".jpg");
		ImageImportInfo info(tmp.c_str());
		FImagePtr img(new FImage(info.size()));
		importImage(info, destImage(*img));
		
		images->push_back(img);
	}
	
	return true;
}
