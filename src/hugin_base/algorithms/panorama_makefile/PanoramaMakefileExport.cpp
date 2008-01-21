// -*- c-basic-offset: 4 -*-
/** @file PanoramaMakefileExport.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: utils.h 1814 2006-12-31 14:37:05Z dangelo $
 *
 *  This is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */


#include "PanoramaMakefileExport.h"

#include <fstream>
#include <iomanip>
#include <panodata/PanoramaData.h>
#include <hugin_utils/utils.h>
#include <algorithms/nona/ComputeImageROI.h>

namespace HuginBase {

using namespace std;
using namespace hugin_utils;
using namespace vigra;


// should be moved somewhere else (will be after GSOC anyway)
vector<UIntSet> getHDRStacks(const PanoramaData & pano, UIntSet allImgs)
{
    vector<UIntSet> result;
    if(pano.getNrOfImages() == 0) return result;
    UIntSet stack;

    do {
        unsigned srcImg = *(allImgs.begin());
        stack.insert(srcImg);
        allImgs.erase(srcImg);

        // find all images that have a suitable overlap.
        SrcPanoImage simg = pano.getSrcImage(srcImg);
        double maxShift = simg.getHFOV() / 10.0;
        for (UIntSet::iterator it = allImgs.begin(); it !=  allImgs.end(); ) {
            unsigned srcImg2 = *it;
            it++;
            SrcPanoImage simg2 = pano.getSrcImage(srcImg2);
            if ( fabs(simg.getYaw() - simg2.getYaw()) < maxShift
                && fabs(simg.getPitch() - simg2.getPitch()) < maxShift  )
            {
                stack.insert(srcImg2);
                allImgs.erase(srcImg2);
            }
        }
        result.push_back(stack);
        stack.clear();
    } while (allImgs.size() > 0);
    
    return result;
}
    
// should be moved somewhere else (will be after GSOC anyway)
vector<UIntSet> getExposureLayers(const PanoramaData & pano, UIntSet allImgs)
{
    vector<UIntSet> result;
    if(pano.getNrOfImages() == 0) return result;
    UIntSet stack;

    do {
        unsigned srcImg = *(allImgs.begin());
        stack.insert(srcImg);
        allImgs.erase(srcImg);

        // find all images that have a suitable overlap.
        SrcPanoImage simg = pano.getSrcImage(srcImg);
        double maxEVDiff = 0.2;
        for (UIntSet::iterator it = allImgs.begin(); it !=  allImgs.end(); ) {
            unsigned srcImg2 = *it;
            it++;
            SrcPanoImage simg2 = pano.getSrcImage(srcImg2);
            if ( fabs(simg.getExposureValue() - simg2.getExposureValue()) < maxEVDiff )
            {
                stack.insert(srcImg2);
                allImgs.erase(srcImg2);
            }
        }
        result.push_back(stack);
        stack.clear();
    } while (allImgs.size() > 0);

    return result;
}


void PanoramaMakefileExport::createMakefile(const PanoramaData& pano,
                                            const UIntSet& rimages,
                                            const std::string& ptofile,
                                            const std::string& outputPrefix,
                                            const PTPrograms& progs,
                                            const std::string& includePath,
                                            std::ostream& o)
{
    PanoramaOptions opts = pano.getOptions();
#ifdef __unix__
    // set numeric locale to C, for correct number output
    char * t = setlocale(LC_NUMERIC,NULL);
    char * old_locale = (char*) malloc(strlen(t)+1);
    strcpy(old_locale, t);
    setlocale(LC_NUMERIC,"C");
#endif

	// output only images in current ROI
	UIntSet images;
	for (UIntSet::const_iterator it = rimages.begin(); it != rimages.end(); ++it)
	{
		Rect2D roi = estimateOutputROI(pano, opts, *it);
		if (! (roi.isEmpty())) {
			images.insert(*it);
		}
	}
	
    o << "# makefile for panorama stitching, created by hugin " << endl
      << endl;

    o << endl
      << endl
      << "# Tool configuration" << endl
      << "NONA=" << escapeStringMake(progs.nona) << endl
      << "PTSTITCHER=" << escapeStringMake(progs.PTStitcher) << endl
      << "PTMENDER=" << escapeStringMake(progs.PTmender) << endl
      << "PTBLENDER=" << escapeStringMake(progs.PTblender) << endl
      << "PTMASKER=" << escapeStringMake(progs.PTmasker) << endl
      << "PTROLLER=" << escapeStringMake(progs.PTroller) << endl
      << "ENBLEND=" << escapeStringMake(progs.enblend) << endl
      << "ENFUSE=" << escapeStringMake(progs.enfuse) << endl
      << "SMARTBLEND=" << escapeStringMake(progs.smartblend) << endl
      << "HDRMERGE=" << escapeStringMake(progs.hdrmerge) << endl
      << "RM=rm" << endl
      << "EXIFTOOL=" << escapeStringMake(progs.exiftool) << endl
      << endl
      << "# options for the programs" << endl << endl;


    o << "ENBLEND_OPTS=" << progs.enblend_opts;
    if (opts.getHFOV() == 360.0) {
        // blend over the border
        o << " -w";
    }
    if (opts.tiffCompression == "LZW") {
        o << " -z";
    }
    vigra::Rect2D roi = opts.getROI();
    if (roi.top() != 0 || roi.left() != 0 ) {
        o << " -f" << roi.width() << "x" << roi.height() << "+" << roi.left() << "+" << roi.top();
    } else {
        o << " -f" << roi.width() << "x" << roi.height();
    }

    o << endl;

    o << "ENFUSE_OPTS=" << progs.enfuse_opts;
    if (opts.getHFOV() == 360.0) {
        // blend over the border
        o << " -w";
    }
    o << endl;

    o << "EXIFTOOL_COPY_ARGS=" << progs.exiftool_opts << endl;
    o << endl;

    string hdrExt = string(".") + opts.outputImageTypeHDR;
    string ldrExt = string(".") + opts.outputImageType;
    string ldrRemappedExt(".tif");
    string ldrRemappedMode("TIFF_m");
    string hdrRemappedExt = ".exr";
    string hdrRemappedMode = "EXR_m";

    // set a suitable target file.
    std::string output = outputPrefix;

//    bool externalBlender = false;
//    bool remapToMultiple = false;

    /*
    if (opts.blendMode == PT::PanoramaOptions::NO_BLEND) {
        // just remapping or simple blending
        if (opts.outputFormat == PT::PanoramaOptions::TIFF_m) {
            remapToMultiple = true;
        }
    } else {
        externalBlender = true;
        remapToMultiple = true;
    }
*/
    o << "# the output panorama" << endl
    << "LDR_REMAPPED_PREFIX=" << escapeStringMake(output) << endl
    << "HDR_STACK_REMAPPED_PREFIX=" << escapeStringMake(output + "_hdr_") << endl
    << "LDR_EXPOSURE_REMAPPED_PREFIX=" << escapeStringMake(output + "_exposure_layers_") << endl
    << "PROJECT_FILE=" << escapeStringMake(ptofile) << endl
    << "LDR_BLENDED=" << escapeStringMake(output + ldrExt) << endl
    << "LDR_STACKED_BLENDED=" << escapeStringMake(output + "_fused" + ldrExt) << endl
    << "HDR_BLENDED=" << escapeStringMake(output + "_hdr" + hdrExt) << endl
    << endl
    << "# first input image" << endl
    << "INPUT_IMAGE_1="  << escapeStringMake(pano.getImage(0).getFilename()) << endl
    << "# all input images" << endl
    << "INPUT_IMAGES=";

    for (unsigned int i=0; i < pano.getNrOfImages(); i++) {
        o << escapeStringMake(pano.getImage(i).getFilename());
        if (i+1 != pano.getNrOfImages()) o << "\\" << endl;
    }

    vector<string> remappedImages;
    o << endl
      << endl
      << "# remapped images" << endl
      << "LDR_LAYERS=";
    for (UIntSet::iterator it = images.begin(); it != images.end();) {
        std::ostringstream fns;
        fns << output << std::setfill('0') << std::setw(4) << *it << ldrRemappedExt;
        remappedImages.push_back(fns.str());
        o << escapeStringMake(fns.str());
        ++it;
        if (it != images.end()) o << "\\" << endl;
    }

    vector<string> remappedHDRImages;
    o << endl
    << endl
    << "# remapped images (hdr)" << endl
    << "HDR_LAYERS=";
    for (UIntSet::iterator it = images.begin(); it != images.end();) {
        std::ostringstream fns;
        fns << output << "_hdr_" << std::setfill('0') << std::setw(4) << *it << hdrRemappedExt;
        remappedHDRImages.push_back(fns.str());
        o << escapeStringMake(fns.str());
        ++it;
        if (it != images.end()) o << "\\" << endl;
    }

    o << endl
    << endl
    << "# remapped maxval images" << endl
    << "HDR_LAYERS_WEIGHTS=";
    for (UIntSet::iterator it = images.begin(); it != images.end();) {
        std::ostringstream fns;
        fns << output << "_hdr_" << std::setfill('0') << std::setw(4) << *it << "_gray.pgm";
        o << escapeStringMake(fns.str()) << " ";
        ++it;
        if (it != images.end()) o << "\\" << endl;
    }
    o << endl;


    vector<string> stackedImages;
    vector<UIntSet> stacks = getHDRStacks(pano, images);
    DEBUG_DEBUG( stacks.size() << " stacks found");
    o << endl
      << "# stacked images" << endl
      << "HDR_STACKS_NUMBERS = ";
    for (unsigned i=0; i < stacks.size(); i++)
        o << i << " ";
    o << endl;
    for (unsigned i=0; i < stacks.size(); i++) {
        std::ostringstream fns;
        fns << output << "_stack_hdr_" << std::setfill('0') << std::setw(4) << i << hdrRemappedExt;
        stackedImages.push_back(fns.str());
        std::ostringstream stackedImgVar;
        stackedImgVar << "HDR_STACK_" << i;
        o << stackedImgVar.str() << " = " << escapeStringMake(fns.str()) << endl;
        o << stackedImgVar.str() << "_INPUT = ";
        for (UIntSet::iterator it = stacks[i].begin(); it != stacks[i].end();) {
            std::ostringstream fns;
            fns << output << "_hdr_" << std::setfill('0') << std::setw(4) << *it << hdrRemappedExt;
            o << escapeStringMake(fns.str());
            ++it;
            if (it != stacks[i].end()) o << "\\" << endl;
        }
        o << endl << endl;
    }
    o << endl;
    o << "HDR_STACKS = ";
    for (unsigned i=0; i < stacks.size(); i++)
        o << "$(HDR_STACK_" << i << ") ";
    o << endl;

    // add support for exposure blending stuff...
    vector<string> similarExposureRemappedImages;
    vector<string> similarExposureImages;
    vector<UIntSet> similarExposures = getExposureLayers(pano, images);
    DEBUG_DEBUG( similarExposures.size() << " similar exposures found");
    o << endl
    << endl
    << "# number of image sets with similar exposure" << endl
    << "LDR_EXPOSURE_EXPOSURE_LAYERS_NUMBERS = ";
    for (unsigned i=0; i < similarExposures.size(); i++)
        o << i << " ";
    o << endl;
    for (unsigned i=0; i < similarExposures.size(); i++) {
        std::ostringstream fns;
        fns << output << "_exposure_" << std::setfill('0') << std::setw(2) << i << ldrExt;
        similarExposureImages.push_back(fns.str());
        string destImg = escapeStringMake(fns.str());
        std::ostringstream expImgVar;
        expImgVar << "LDR_EXPOSURE_LAYER_" << i;
        o << expImgVar.str() << " = " << destImg << endl;
        o << expImgVar.str() << "_INPUT = ";
        double exposure=0;
        for (UIntSet::iterator it = similarExposures[i].begin(); it != similarExposures[i].end();) {
            exposure += pano.getSrcImage(*it).getExposureValue();
            std::ostringstream fns;
            fns << output << "_exposure_layers_" << std::setfill('0') << std::setw(4) << *it << ldrRemappedExt;
            similarExposureRemappedImages.push_back(fns.str());
            o << escapeStringMake(fns.str());
            ++it;
            if (it != similarExposures[i].end()) o << "\\" << endl;
        }
        o << endl << endl;
        o << expImgVar.str() << " = " << destImg << endl;
        o << expImgVar.str() << "_INPUT_PTMENDER = ";
        for (UIntSet::iterator it = similarExposures[i].begin(); it != similarExposures[i].end();) {
            std::ostringstream fns;
            fns << output << std::setfill('0') << std::setw(4) << *it << ldrRemappedExt;
            o << escapeStringMake(fns.str());
            ++it;
            if (it != similarExposures[i].end()) o << "\\" << endl;
        }
        // calculate output exposure value for this set.
        o << endl
          << "LDR_EXPOSURE_LAYER_" << i << "_EXPOSURE = "
          << exposure / similarExposures[i].size() << endl;
    }
    o << endl;
    o << "LDR_EXPOSURE_LAYERS = ";
    for (unsigned i=0; i < similarExposures.size(); i++)
        o << "$(LDR_EXPOSURE_LAYER_" << i << ") ";
    o << endl;
    o << "LDR_EXPOSURE_LAYERS_REMAPPED = ";
    for (unsigned i=0; i < similarExposureRemappedImages.size(); i++)
    {
        o << escapeStringMake(similarExposureRemappedImages[i]);
        if (i+1 != similarExposureRemappedImages.size()) o << "\\" << endl;
    }
    o << endl << endl;

	
    vector<string> ldrStackedImages;
    o << endl
      << "# stacked images for enfuse or other automatic exposure blending tools" << endl
      << "LDR_STACKS_NUMBERS = ";
    for (unsigned i=0; i < stacks.size(); i++)
        o << i << " ";
    o << endl;
    for (unsigned i=0; i < stacks.size(); i++) {
        std::ostringstream fns;
        fns << output << "_stack_ldr_" << std::setfill('0') << std::setw(4) << i << ldrRemappedExt;
        ldrStackedImages.push_back(fns.str());
        std::ostringstream stackedImgVar;
        stackedImgVar << "LDR_STACK_" << i;
        o << stackedImgVar.str() << " = " << escapeStringMake(fns.str()) << endl;
        o << stackedImgVar.str() << "_INPUT = ";
        for (UIntSet::iterator it = stacks[i].begin(); it != stacks[i].end();) {
            std::ostringstream fns;
            fns << output << "_exposure_layers_" << std::setfill('0') << std::setw(4) << *it << ldrRemappedExt;
            o << escapeStringMake(fns.str());
            ++it;
            if (it != stacks[i].end()) o << "\\" << endl;
        }
        o << endl << endl;
    }
    o << endl;
    o << "LDR_STACKS = ";
    for (unsigned i=0; i < stacks.size(); i++)
        o << "$(LDR_STACK_" << i << ") ";
    o << endl;

	
    // TODO: include custom makefile here
    if (includePath.size() > 0) {
        o << "include " <<  escapeStringMake(includePath) <<  endl << endl;
    } else {
        // create rules for all possible targets.

        std::string targets;
        std::string cleanTargets;

        // output all targets
        if (opts.outputLDRBlended)
            targets += "$(LDR_BLENDED) ";
        else
            cleanTargets += "$(LDR_BLENDED) ";

        if (opts.outputLDRLayers)
            targets +=  "$(LDR_LAYERS) ";
        else
            cleanTargets +=  "$(LDR_LAYERS) ";

        if (opts.outputLDRExposureRemapped)
            targets += "$(LDR_EXPOSURE_LAYERS_REMAPPED) ";
        else
            cleanTargets += "$(LDR_EXPOSURE_LAYERS_REMAPPED) ";

        if (opts.outputLDRExposureLayers)
            targets += " $(LDR_EXPOSURE_LAYERS) ";
        else
            cleanTargets += "$(LDR_EXPOSURE_LAYERS) ";

        if (opts.outputLDRExposureBlended)
            targets += " $(LDR_STACKED_BLENDED) ";
        else
            cleanTargets += "$(LDR_STACKED_BLENDED) ";

        // always clean temp files used by exposure stacks
        cleanTargets += "$(LDR_STACKS) ";

        if (opts.outputHDRBlended)
            targets += "$(HDR_BLENDED) ";
        else
            cleanTargets += "$(HDR_BLENDED) $(HDR_LAYERS_WEIGHTS) ";

        if (opts.outputHDRLayers)
            targets += "$(HDR_LAYERS) ";
        else
            cleanTargets += "$(HDR_LAYERS) $(HDR_LAYERS_WEIGHTS) ";

        if (opts.outputHDRStacks)
            targets += "$(HDR_STACKS) ";
        else
            cleanTargets += "$(HDR_STACKS) $(HDR_LAYERS_WEIGHTS) ";

        // targets and clean rule.

        o << "TEMP_FILES = " << cleanTargets << endl
        << endl
        << "all: " << targets << endl << endl
        << "clean: " << endl
        << "\t-$(RM) $(TEMP_FILES)" << endl
        << endl;

        // ==============================
        // output rules for all targets.
        // remapped LDR images for exposure stacks.
        switch(opts.remapper) {
            case PanoramaOptions::NONA:
                // produce rules for remapping with nona:
                {
                    o << "# Rules for ordinary TIFF_m output" << endl;
                    int i=0;
                    for (UIntSet::iterator it = images.begin();
                        it != images.end(); ++it)
                    {
                        string destImg = escapeStringMake(remappedImages[i]);
                        string srcImg = escapeStringMake(pano.getImage(*it).getFilename());
                        o << destImg << ": " << srcImg << " $(PROJECT_FILE)" << endl
                        << "\t$(NONA) -r ldr -m " << ldrRemappedMode << " -o $(LDR_REMAPPED_PREFIX) -i " << *it << " $(PROJECT_FILE)" << endl << endl;
                        i++;
                    }

                    o << "# Rules for merge to hdr output" << endl;
                    i=0;
                    for (UIntSet::iterator it = images.begin();
                        it != images.end(); ++it)
                    {
                        string destImg = escapeStringMake(remappedHDRImages[i]);
                        string srcImg = escapeStringMake(pano.getImage(*it).getFilename());
                        o << destImg << ": " << srcImg << " $(PROJECT_FILE)" << endl
                        << "\t$(NONA) -r hdr -m " << hdrRemappedMode << " -o $(HDR_STACK_REMAPPED_PREFIX) -i " << *it << " $(PROJECT_FILE)" << endl << endl;
                        i++;
                    }

                    // rules for exposure sets.
                    o << "# Rules for exposure layer output" << endl;
                    int j=0;
                    for (unsigned i=0; i < similarExposures.size(); i++) {
                        for (UIntSet::iterator it = similarExposures[i].begin();
                            it != similarExposures[i].end(); ++it)
                        {
                            string destImg = escapeStringMake(similarExposureRemappedImages[j]);
                            string srcImg = escapeStringMake(pano.getImage(*it).getFilename());
							/*
                            o << destImg << ": " << srcImg << " $(PROJECT_FILE)" << endl
                              << "\t$(NONA) -r ldr -e $(LDR_EXPOSURE_LAYER_" << i << "_EXPOSURE) -m "
                              << ldrRemappedMode << " -o $(LDR_EXPOSURE_REMAPPED_PREFIX) -i " << *it
                              << " $(PROJECT_FILE)" << endl << endl;
							*/
                            o << destImg << ": " << srcImg << " $(PROJECT_FILE)" << endl
                              << "\t$(NONA) -r ldr -e " << pano.getSrcImage(*it).getExposureValue()
                              << " -m " << ldrRemappedMode << " -o $(LDR_EXPOSURE_REMAPPED_PREFIX) -i " << *it
                              << " $(PROJECT_FILE)" << endl << endl;
                            j++;
                        }
                    }
                }
                break;
            case PanoramaOptions::PTMENDER:
                    o << "$(LDR_LAYERS) : $(INPUT_IMAGES) $(PROJECT_FILE)" << endl
                    << "\t$(PTMENDER) -o $(LDR_REMAPPED_PREFIX)  $(PROJECT_FILE)" << endl << endl;
                break;
        }

        // ====================================
        // output rules for HDR merging

        // write rules for each HDR stack
        // only output pixes that are defined in all input images
        for (unsigned i=0; i < stacks.size(); i++) {
            o << "$(HDR_STACK_" << i << ") : $(HDR_STACK_" << i << "_INPUT)" << endl
            << "\t$(HDRMERGE) -m avg -c -o $(HDR_STACK_" << i << ") $(HDR_STACK_" << i << "_INPUT)" << endl
            << endl;
        }

        // ====================================
        // output rules for LDR stack merging

        for (unsigned i=0; i < stacks.size(); i++) {
            o << "$(LDR_STACK_" << i << ") : $(LDR_STACK_" << i << "_INPUT)" << endl
            << "\t$(ENFUSE) $(ENFUSE_OPTS) -o $(LDR_STACK_" << i << ") $(LDR_STACK_" << i << "_INPUT)" << endl
            << "\t$(EXIFTOOL) $(EXIFTOOL_COPY_ARGS) $(INPUT_IMAGE_1) $@" << endl
            << endl;
        }

        switch(opts.blendMode) {
            case PanoramaOptions::ENBLEND_BLEND:
                // write rules for blending with enblend
                o << "$(LDR_BLENDED) : $(LDR_LAYERS)" << endl;
                o << "\t$(ENBLEND) $(ENBLEND_OPTS) -o $(LDR_BLENDED) $(LDR_LAYERS) " << endl;
                o << "\t$(EXIFTOOL) $(EXIFTOOL_COPY_ARGS) $(INPUT_IMAGE_1) $@" << endl << endl;

                // for LDR exposure blend planes
                for (unsigned i=0; i < similarExposures.size(); i++) {
                    o << "$(LDR_EXPOSURE_LAYER_" << i <<") : $(LDR_EXPOSURE_LAYER_" << i << "_INPUT)" << endl
                      << "\t$(ENBLEND) $(ENBLEND_OPTS) -o $@ $^" << endl
                      << "\t$(EXIFTOOL) $(EXIFTOOL_COPY_ARGS) $(INPUT_IMAGE_1) $@" << endl << endl;
                }

		// rules for enfuse blending
                o << "$(LDR_STACKED_BLENDED) : $(LDR_STACKS)" << endl
                  << "\t$(ENBLEND) $(ENBLEND_OPTS) -o $(LDR_STACKED_BLENDED) $(LDR_STACKS) " << endl
                  << "\t$(EXIFTOOL) $(EXIFTOOL_COPY_ARGS) $(INPUT_IMAGE_1) $@" << endl << endl;

		// rules for hdr blending
                o << "$(HDR_BLENDED) : $(HDR_STACKS)" << endl;
                o << "\t$(ENBLEND) $(ENBLEND_OPTS) -o $(HDR_BLENDED) $(HDR_STACKS) " << endl << endl;

                break;
            case PanoramaOptions::NO_BLEND:
                o << "$(LDR_BLENDED) : $(LDR_LAYERS)" << endl
				  << "\t-$(RM) $@" << endl
                  << "\t$(PTROLLER) -o $@ $^ " << endl 
                  << "\t$(EXIFTOOL) $(EXIFTOOL_COPY_ARGS) $(INPUT_IMAGE_1) $@" << endl << endl;

                // for LDR exposure blend planes
                for (unsigned i=0; i < similarExposures.size(); i++) {
                    o << "$(LDR_EXPOSURE_LAYER_" << i <<") : $(LDR_EXPOSURE_LAYER_" << i << "_INPUT)" << endl
                      << "\t-$(RM) $@" << endl
                      << "\t$(PTROLLER) -o $@ $^ " << endl
                      << "\t$(EXIFTOOL) $(EXIFTOOL_COPY_ARGS) $(INPUT_IMAGE_1) $@" << endl << endl;
		}

                o << "$(LDR_STACKED_BLENDED) : $(LDR_STACKS)" << endl
                  << "\t-$(RM) $@" << endl
                  << "\t$(PTROLLER) -o $@ $^ " << endl 
                  << "\t$(EXIFTOOL) $(EXIFTOOL_COPY_ARGS) $(INPUT_IMAGE_1) $@" << endl << endl;

                // rules for non-blended HDR panoramas
                o << "$(HDR_BLENDED) : $(HDR_LAYERS)" << endl;
                o << "\t$(HDRMERGE) -m avg -o $@ $^"   << endl << endl;

                break;
            case PanoramaOptions::PTBLENDER_BLEND:
                o << "PTBLENDER_OPTS=";
                switch (opts.colorCorrection) {
                    case PanoramaOptions::NONE:
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
                // TODO: output PTBlender + PTmasker + PTroller rules
                break;
            case PanoramaOptions::SMARTBLEND_BLEND:
                o << "SMARTBLEND_OPTS=" << progs.smartblend_opts;
                if (opts.getHFOV() == 360.0) {
                    // blend over the border
                    o << " -w";
                }
                o << endl;
                // TODO: build smartblend command line from given images. (requires additional program)
                break;
            default:
                // TODO:
                break;
        }
    }

#ifdef __unix__
    // reset locale
    setlocale(LC_NUMERIC,old_locale);
    free(old_locale);
#endif

}

} //namespace
