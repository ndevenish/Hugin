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

#if defined MAC_SELF_CONTAINED_BUNDLE
 #define COULD_EXECUTE_EXIFTOOL_WITH_PERL
#endif

namespace HuginBase {

using namespace std;
using namespace hugin_utils;
using namespace vigra;


// should be moved somewhere else (will be after GSOC anyway)
vector<UIntSet> getHDRStacks(const PanoramaData & pano, UIntSet allImgs)
{
    vector<UIntSet> result;

    // if no images are available, return empty result vector
    if ( allImgs.empty() )
    {
        return result;
    }

    UIntSet stack;

    do {
        unsigned srcImg = *(allImgs.begin());
        stack.insert(srcImg);
        allImgs.erase(srcImg);

        // find all images that have a suitable overlap.
        SrcPanoImage simg = pano.getSrcImage(srcImg);
        double maxShift = simg.getHFOV() / 10.0;
        double minShift = 360.0 - maxShift;
        for (UIntSet::iterator it = allImgs.begin(); it !=  allImgs.end(); ) {
            unsigned srcImg2 = *it;
            it++;
            SrcPanoImage simg2 = pano.getSrcImage(srcImg2);
            if ( (fabs(simg.getYaw() - simg2.getYaw()) < maxShift
                || fabs(simg.getYaw() - simg2.getYaw()) > minShift)
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

    // if no images are available, return empty result vector
    if ( allImgs.empty() )
    {
        return result;
    }

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


// should be moved somewhere else (will be after GSOC anyway)
UIntSet getImagesinROI (const PanoramaData& pano, const UIntSet activeImages)
{
    UIntSet images;
    PanoramaOptions opts = pano.getOptions();
    for (UIntSet::const_iterator it = activeImages.begin(); it != activeImages.end(); ++it)
    {
        Rect2D roi = estimateOutputROI(pano, opts, *it);
        if (! (roi.isEmpty())) {
            images.insert(*it);
        }
    }
    return images;
}


void PanoramaMakefileExport::createMakefile(const PanoramaData& pano,
                                            const UIntSet& rimages,
                                            const std::string& ptofile,
                                            const std::string& outputPrefix,
                                            const PTPrograms& progs,
                                            const std::string& includePath,
                                            std::vector<std::string> & outputFiles,
                                            std::ostream& o,
                                            const std::string& tmpDir)
{
    PanoramaOptions opts = pano.getOptions();
#ifdef __unix__
    // set numeric locale to C, for correct number output
    char * t = setlocale(LC_NUMERIC,NULL);
    char * old_locale = (char*) malloc(strlen(t)+1);
    strcpy(old_locale, t);
    setlocale(LC_NUMERIC,"C");
#endif

#ifdef __unix__
 std::string NULL_DEVICE("/dev/null");
#else // WINDOWS
 std::string NULL_DEVICE("NUL");
#endif

    // output only images in current ROI
    UIntSet images = getImagesinROI(pano,rimages);

    // execute exiftool with perl if necessary
#ifdef COULD_EXECUTE_EXIFTOOL_WITH_PERL
    bool executeWithPerl = false;
    std::string perlCommand = "";
#if defined MAC_SELF_CONTAINED_BUNDLE
    // if exiftool is inside the bundle (not the best def, but works)
    if(progs.exiftool.find(".app") != std::string::npos)
    {
        executeWithPerl = true;
        perlCommand += "perl -w";
    }
#endif
#endif

    o << "# makefile for panorama stitching, created by hugin " << endl
      << endl;

    // pass settings for different temporary directory
    if (tmpDir != "") {
        o << "# set temporary directory" << endl;
#ifdef __unix__
        o << "export TMPDIR=" << quoteStringShell(tmpDir) << endl;
#else // WINDOWS
        o << "export TEMP=" << quoteStringShell(tmpDir) << endl
          << "export TMP=" << quoteStringShell(tmpDir) << endl;
#endif
    }

    o << endl
      << endl
      << "# Tool configuration" << endl
      << "NONA=" << quoteStringShell(progs.nona) << endl
      << "PTSTITCHER=" << quoteStringShell(progs.PTStitcher) << endl
      << "PTMENDER=" << quoteStringShell(progs.PTmender) << endl
      << "PTBLENDER=" << quoteStringShell(progs.PTblender) << endl
      << "PTMASKER=" << quoteStringShell(progs.PTmasker) << endl
      << "PTROLLER=" << quoteStringShell(progs.PTroller) << endl
      << "ENBLEND=" << quoteStringShell(progs.enblend) << endl
      << "ENFUSE=" << quoteStringShell(progs.enfuse) << endl
      << "SMARTBLEND=" << quoteStringShell(progs.smartblend) << endl
      << "HDRMERGE=" << quoteStringShell(progs.hdrmerge) << endl
      << "RM=rm" << endl
#ifdef COULD_EXECUTE_EXIFTOOL_WITH_PERL
      << "EXIFTOOL=" << (executeWithPerl? perlCommand+" " : "") << quoteStringShell(progs.exiftool) << endl
#else
      << "EXIFTOOL=" << quoteStringShell(progs.exiftool) << endl
#endif
      << endl

      << "# Project parameters" << endl
      << "HUGIN_PROJECTION=" << opts.getProjection() << endl
      << "HUGIN_HFOV=" << opts.getHFOV() << endl
      << "HUGIN_WIDTH=" << opts.getWidth() << endl
      << "HUGIN_HEIGHT=" << opts.getHeight() << endl
      << endl

      << "# options for the programs" << endl << endl;

    // set remapper specific settings
    switch(opts.remapper) {
        case PanoramaOptions::NONA:
            {
                o << "NONA_LDR_REMAPPED_COMP=";
                if (opts.outputImageType == "tif" && opts.outputLayersCompression.size() != 0) {
                    o << "-z " << opts.outputLayersCompression;
                } else if (opts.outputImageType == "jpg") {
                    o << "-z PACKBITS ";
                }
                o << endl;
            }
            break;
        case PanoramaOptions::PTMENDER:
            break;
    }

    // set blender specific settings
    switch(opts.blendMode) {
        case PanoramaOptions::ENBLEND_BLEND:
            {
                o << "ENBLEND_OPTS=" << opts.enblendOptions;
                if (opts.getHFOV() == 360.0) {
                    // blend over the border
                    o << " -w";
                }
                vigra::Rect2D roi = opts.getROI();
                if (roi.top() != 0 || roi.left() != 0 ) {
                    o << " -f" << roi.width() << "x" << roi.height() << "+" << roi.left() << "+" << roi.top();
                } else {
                    o << " -f" << roi.width() << "x" << roi.height();
                }
                o << endl;

                o << "ENBLEND_LDR_COMP=";
                if (opts.outputImageType == "tif" && opts.outputImageTypeCompression.size() != 0) {
                    o << "--compression " << opts.outputImageTypeCompression;
                } else if (opts.outputImageType == "jpg") {
                    o << "--compression " << opts.quality;
                }
                o << endl;

                o << "ENBLEND_HDR_COMP=";
                if (opts.outputImageType == "tif" && opts.outputImageTypeHDRCompression.size() != 0) {
                    o << "--compression " << opts.outputImageTypeHDRCompression;
                }
                o << endl;
            }
            break;
        case PanoramaOptions::PTBLENDER_BLEND:
            {
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
            }
            break;
        case PanoramaOptions::SMARTBLEND_BLEND:
            {
                o << "SMARTBLEND_OPTS=" << progs.smartblend_opts;
                if (opts.getHFOV() == 360.0) {
                    // blend over the border
                    o << " -w";
                }
                o << endl;
                // TODO: build smartblend command line from given images. (requires additional program)
            }
            break;
    }

    o << "ENFUSE_OPTS=" << opts.enfuseOptions;
    // TODO: blend only over border if this is indeed a
    // image with 360 deg overlap
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
    std::string sLDR_BLENDED = output + ldrExt;
    std::string sLDR_STACKED_BLENDED = output + "_fused" + ldrExt;
    std::string sHDR_BLENDED = output + "_hdr" + hdrExt;

    o << "# the output panorama" << endl
      << "LDR_REMAPPED_PREFIX=" << escapeStringMake(output) << endl
      << "LDR_REMAPPED_PREFIX_SHELL=" << quoteStringShell(output) << endl

    << "HDR_STACK_REMAPPED_PREFIX=" << escapeStringMake(output + "_hdr_") << endl
    << "HDR_STACK_REMAPPED_PREFIX_SHELL=" << quoteStringShell(output + "_hdr_") << endl

    << "LDR_EXPOSURE_REMAPPED_PREFIX=" << escapeStringMake(output + "_exposure_layers_") << endl
    << "LDR_EXPOSURE_REMAPPED_PREFIX_SHELL=" << quoteStringShell(output + "_exposure_layers_") << endl

    << "PROJECT_FILE=" << escapeStringMake(ptofile) << endl
    << "PROJECT_FILE_SHELL=" << quoteStringShell(ptofile) << endl

    << "LDR_BLENDED=" << escapeStringMake(sLDR_BLENDED) << endl
    << "LDR_BLENDED_SHELL=" << quoteStringShell(sLDR_BLENDED) << endl

    << "LDR_STACKED_BLENDED=" << escapeStringMake(sLDR_STACKED_BLENDED) << endl
    << "LDR_STACKED_BLENDED_SHELL=" << quoteStringShell(sLDR_STACKED_BLENDED) << endl

    << "HDR_BLENDED=" << escapeStringMake(sHDR_BLENDED) << endl
    << "HDR_BLENDED_SHELL=" << quoteStringShell(sHDR_BLENDED) << endl
    << endl
    << "# first input image" << endl
    << "INPUT_IMAGE_1="  << escapeStringMake(pano.getImage(0).getFilename()) << endl
    << "INPUT_IMAGE_1_SHELL="  << quoteStringShell(pano.getImage(0).getFilename()) << endl

    << "# all input images" << endl
    << "INPUT_IMAGES=";
    for (unsigned int i=0; i < pano.getNrOfImages(); i++) {
        o << escapeStringMake(pano.getImage(i).getFilename());
        if (i+1 != pano.getNrOfImages()) o << "\\" << endl;
    }
    o << endl << endl;

    o << "INPUT_IMAGES_SHELL=";
    for (unsigned int i=0; i < pano.getNrOfImages(); i++) {
        o << quoteStringShell(pano.getImage(i).getFilename());
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

    o << endl << endl
      << "LDR_LAYERS_SHELL=";
    for(int i=0; i < (int) remappedImages.size(); i++) {
        o << quoteStringShell(remappedImages[i]);
        if (i != remappedImages.size() -1) {
            o << "\\" << endl;
        }
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
    o << endl << endl
      << "HDR_LAYERS_SHELL=";
    for(int i=0; i < (int) remappedHDRImages.size(); i++) {
        o << quoteStringShell(remappedHDRImages[i]);
        if (i != remappedHDRImages.size() - 1) {
            o << "\\" << endl;
        }
    }

    vector<string> remappedHDRImagesGray;
    o << endl
      << endl
      << "# remapped maxval images" << endl
      << "HDR_LAYERS_WEIGHTS=";
    for (UIntSet::iterator it = images.begin(); it != images.end();) {
        std::ostringstream fns;
        fns << output << "_hdr_" << std::setfill('0') << std::setw(4) << *it << "_gray.pgm";
        remappedHDRImagesGray.push_back(fns.str());
        o << escapeStringMake(fns.str()) << " ";
        ++it;
        if (it != images.end()) o << "\\" << endl;
    }
    o << endl << endl
      << "HDR_LAYERS_WEIGHTS_SHELL=";
    for(unsigned i=0; i < remappedHDRImagesGray.size(); i++) {
        o << quoteStringShell(remappedHDRImagesGray[i]);
        if (i != remappedHDRImagesGray.size() - 1) {
            o << "\\" << endl;
        }
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
        o << stackedImgVar.str() << "_SHELL = " << quoteStringShell(fns.str()) << endl;
        o << stackedImgVar.str() << "_INPUT = ";
        for (UIntSet::iterator it = stacks[i].begin(); it != stacks[i].end();) {
            std::ostringstream fns;
            fns << output << "_hdr_" << std::setfill('0') << std::setw(4) << *it << hdrRemappedExt;
            o << escapeStringMake(fns.str());
            ++it;
            if (it != stacks[i].end()) o << "\\" << endl;
        }
        o << endl << endl;
        o << stackedImgVar.str() << "_INPUT_SHELL = ";
        for (UIntSet::iterator it = stacks[i].begin(); it != stacks[i].end();) {
            std::ostringstream fns;
            fns << output << "_hdr_" << std::setfill('0') << std::setw(4) << *it << hdrRemappedExt;
            o << quoteStringShell(fns.str());
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
    o << "HDR_STACKS_SHELL = ";
    for (unsigned i=0; i < stacks.size(); i++)
        o << "$(HDR_STACK_" << i << "_SHELL) ";
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
        string destImg = fns.str();
        std::ostringstream expImgVar;
        expImgVar << "LDR_EXPOSURE_LAYER_" << i;
        o << expImgVar.str() << " = " << escapeStringMake(destImg) << endl;
        o << expImgVar.str() << "_SHELL = " << quoteStringShell(destImg) << endl;
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
        o << expImgVar.str() << "_INPUT_SHELL = ";
        for (UIntSet::iterator it = similarExposures[i].begin(); it != similarExposures[i].end();) {
            exposure += pano.getSrcImage(*it).getExposureValue();
            std::ostringstream fns;
            fns << output << "_exposure_layers_" << std::setfill('0') << std::setw(4) << *it << ldrRemappedExt;
            o << quoteStringShell(fns.str());
            ++it;
            if (it != similarExposures[i].end()) o << "\\" << endl;
        }
        o << endl << endl;
        o << expImgVar.str() << "_INPUT_PTMENDER = ";
        for (UIntSet::iterator it = similarExposures[i].begin(); it != similarExposures[i].end();) {
            std::ostringstream fns;
            fns << output << std::setfill('0') << std::setw(4) << *it << ldrRemappedExt;
            o << escapeStringMake(fns.str());
            ++it;
            if (it != similarExposures[i].end()) o << "\\" << endl;
        }

        o << endl << endl << expImgVar.str() << "_INPUT_PTMENDER_SHELL = ";
        for (UIntSet::iterator it = similarExposures[i].begin(); it != similarExposures[i].end();) {
            std::ostringstream fns;
            fns << output << std::setfill('0') << std::setw(4) << *it << ldrRemappedExt;
            o << quoteStringShell(fns.str());
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
    o << "LDR_EXPOSURE_LAYERS_SHELL = ";
    for (unsigned i=0; i < similarExposures.size(); i++)
        o << "$(LDR_EXPOSURE_LAYER_" << i << "_SHELL) ";
    o << endl;
    o << "LDR_EXPOSURE_LAYERS_REMAPPED = ";
    for (unsigned i=0; i < similarExposureRemappedImages.size(); i++)
    {
        o << escapeStringMake(similarExposureRemappedImages[i]);
        if (i+1 != similarExposureRemappedImages.size()) o << "\\" << endl;
    }
    o << endl << endl;
    o << "LDR_EXPOSURE_LAYERS_REMAPPED_SHELL = ";
    for (unsigned i=0; i < similarExposureRemappedImages.size(); i++)
    {
        o << quoteStringShell(similarExposureRemappedImages[i]);
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
        o << stackedImgVar.str() << "_SHELL = " << quoteStringShell(fns.str()) << endl;
        o << stackedImgVar.str() << "_INPUT = ";
        for (UIntSet::iterator it = stacks[i].begin(); it != stacks[i].end();) {
            std::ostringstream fns;
            fns << output << "_exposure_layers_" << std::setfill('0') << std::setw(4) << *it << ldrRemappedExt;
            o << escapeStringMake(fns.str());
            ++it;
            if (it != stacks[i].end()) o << "\\" << endl;
        }
        o << endl << endl;
        o << stackedImgVar.str() << "_INPUT_SHELL = ";
        for (UIntSet::iterator it = stacks[i].begin(); it != stacks[i].end();) {
            std::ostringstream fns;
            fns << output << "_exposure_layers_" << std::setfill('0') << std::setw(4) << *it << ldrRemappedExt;
            o << quoteStringShell(fns.str());
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
    o << "LDR_STACKS_SHELL = ";
    for (unsigned i=0; i < stacks.size(); i++)
        o << "$(LDR_STACK_" << i << "_SHELL) ";
    o << endl;


    // TODO: include custom makefile here
    if (includePath.size() > 0) {
        o << "include " <<  escapeStringMake(includePath) <<  endl << endl;
    } else {
        // create rules for all possible targets.

        std::string targets;
        std::string cleanTargets;

        // output all targets
        if (opts.outputLDRBlended) {
            targets += "$(LDR_BLENDED) ";
            outputFiles.push_back(sLDR_BLENDED);
            o << "DO_LDR_BLENDED = 1" << endl;
            // depends on remapped ldr images and stacked ldr images
            if (! opts.outputLDRLayers) {
                outputFiles.insert(outputFiles.end(), remappedImages.begin(), remappedImages.end());
                cleanTargets += "$(LDR_LAYERS_SHELL) ";
            }
        }

        if (opts.outputLDRLayers) {
            targets +=  "$(LDR_LAYERS) ";
            outputFiles.insert(outputFiles.end(), remappedImages.begin(), remappedImages.end());
        }

        if (opts.outputLDRExposureRemapped) {
            targets += "$(LDR_EXPOSURE_LAYERS_REMAPPED) ";
            outputFiles.insert(outputFiles.end(), similarExposureRemappedImages.begin(), similarExposureRemappedImages.end());
        }

        if (opts.outputLDRExposureLayers) {
            targets += " $(LDR_EXPOSURE_LAYERS) ";
            outputFiles.insert(outputFiles.end(), similarExposureImages.begin(), similarExposureImages.end());
            if (! opts.outputLDRExposureRemapped) {
                outputFiles.insert(outputFiles.end(), similarExposureRemappedImages.begin(), similarExposureRemappedImages.end());
                cleanTargets += "$(LDR_EXPOSURE_LAYERS_REMAPPED_SHELL) ";
            }
        }

        if (opts.outputLDRExposureBlended) {
            targets += " $(LDR_STACKED_BLENDED) ";
            outputFiles.push_back(sLDR_STACKED_BLENDED);
            o << "DO_LDR_STACKED_BLENDED = 1" << endl;
            outputFiles.insert(outputFiles.end(),ldrStackedImages.begin(), ldrStackedImages.end());
            // always clean temp files used by exposure stacks
            cleanTargets += "$(LDR_STACKS_SHELL) ";
            if (! opts.outputLDRExposureRemapped && ! opts.outputLDRExposureLayers) {
                outputFiles.insert(outputFiles.end(), similarExposureRemappedImages.begin(), similarExposureRemappedImages.end());
                cleanTargets += "$(LDR_EXPOSURE_LAYERS_REMAPPED_SHELL) ";
            }
        }

        if (opts.outputHDRLayers) {
            targets += "$(HDR_LAYERS) ";
            outputFiles.insert(outputFiles.end(),remappedHDRImages.begin(), remappedHDRImages.end());
            outputFiles.insert(outputFiles.end(),remappedHDRImagesGray.begin(), remappedHDRImagesGray.end());
        }

        if (opts.outputHDRStacks) {
            targets += "$(HDR_STACKS) ";
            outputFiles.insert(outputFiles.end(),stackedImages.begin(), stackedImages.end());
            if (!opts.outputHDRLayers) {
                outputFiles.insert(outputFiles.end(),remappedHDRImages.begin(), remappedHDRImages.end());
                outputFiles.insert(outputFiles.end(),remappedHDRImagesGray.begin(), remappedHDRImagesGray.end());
                cleanTargets += "$(HDR_LAYERS_SHELL) $(HDR_LAYERS_WEIGHTS_SHELL) ";
            }
        }

        if (opts.outputHDRBlended) {
            targets += "$(HDR_BLENDED) ";
            outputFiles.push_back(sHDR_BLENDED);
            o << "DO_HDR_BLENDED = 1" << endl;
            if (!opts.outputHDRStacks) {
                outputFiles.insert(outputFiles.end(),stackedImages.begin(), stackedImages.end());
                cleanTargets += "$(HDR_STACKS_SHELL) ";
                if (! opts.outputHDRLayers) {
                    outputFiles.insert(outputFiles.end(),remappedHDRImages.begin(), remappedHDRImages.end());
                    outputFiles.insert(outputFiles.end(),remappedHDRImagesGray.begin(), remappedHDRImagesGray.end());
                    cleanTargets += "$(HDR_LAYERS_SHELL) $(HDR_LAYERS_WEIGHTS_SHELL) ";
                }
            }
        }

        // targets and clean rule.

        o << "TEMP_FILES_SHELL = " << cleanTargets << endl
        << endl
        << "all: " << targets << endl << endl
        << "clean: " << endl
        << "\t-$(RM) $(TEMP_FILES_SHELL)" << endl
        << endl;

        // test rule
        o << "test: " << endl;
        // test remapper
        switch(opts.remapper) {
            case PanoramaOptions::NONA:
                o << "\t@echo -n 'Checking nona...'" << endl
                  << "\t@-$(NONA) --help > " << NULL_DEVICE << " 2>&1 && echo '[OK]'" << endl;
                break;
            case PanoramaOptions::PTMENDER:
                break;
        }
        // test blender
        switch(opts.blendMode) {
            case PanoramaOptions::ENBLEND_BLEND:
                o << "\t@echo -n 'Checking enblend...'" << endl
                  << "\t@-$(ENBLEND) -h > " << NULL_DEVICE << " 2>&1 && echo '[OK]'" << endl;
                break;
            case PanoramaOptions::PTBLENDER_BLEND:
                o << "\t@echo -n 'Checking PTblender...'" << endl
                  << "\t@-$(PTBLENDER) -h > " << NULL_DEVICE << " 2>&1 && echo '[OK]'" << endl;
                break;
            case PanoramaOptions::SMARTBLEND_BLEND:
                o << "\t@echo -n 'Checking smartblend...'" << endl
                  << "\t@-$(SMARTBLEND) > " << NULL_DEVICE << " 2>&1 && echo '[OK]'" << endl;
                break;
        }
        // test enfuse
        o << "\t@echo -n 'Checking enfuse...'" << endl
          << "\t@-$(ENFUSE) -h > " << NULL_DEVICE << " 2>&1 && echo '[OK]'" << endl;
        // test hugin_hdrmerge
        o << "\t@echo -n 'Checking hugin_hdrmerge...'" << endl
          << "\t@-$(HDRMERGE) -h > " << NULL_DEVICE << " 2>&1 && echo '[OK]'" << endl;
        // test exiftool
        o << "\t@echo -n 'Checking exiftool...'" << endl
          << "\t@-$(EXIFTOOL) -ver > " << NULL_DEVICE << " 2>&1 && echo '[OK]' || echo '[FAIL]'" << endl;
        // test rm
        o << "\t@echo -n 'Checking rm...'" << endl
          << "\t@-$(RM) --version > " << NULL_DEVICE << " 2>&1 && echo '[OK]' || echo '[FAIL]'" << endl;
        o << endl;

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
                        << "\t$(NONA) $(NONA_LDR_REMAPPED_COMP) -r ldr -m " << ldrRemappedMode << " -o $(LDR_REMAPPED_PREFIX_SHELL) -i " << *it << " $(PROJECT_FILE_SHELL)" << endl << endl;
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
                        << "\t$(NONA) -r hdr -m " << hdrRemappedMode << " -o $(HDR_STACK_REMAPPED_PREFIX_SHELL) -i " << *it << " $(PROJECT_FILE_SHELL)" << endl << endl;
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
                              << "\t$(NONA) $(NONA_LDR_REMAPPED_COMP) -r ldr -e " << pano.getSrcImage(*it).getExposureValue()
                              << " -m " << ldrRemappedMode << " -o $(LDR_EXPOSURE_REMAPPED_PREFIX_SHELL) -i " << *it
                              << " $(PROJECT_FILE_SHELL)" << endl << endl;
                            j++;
                        }
                    }
                }
                break;
            case PanoramaOptions::PTMENDER:
                    o << "$(LDR_LAYERS) : $(INPUT_IMAGES) $(PROJECT_FILE)" << endl
                    << "\t$(PTMENDER) -o $(LDR_REMAPPED_PREFIX_SHELL)  $(PROJECT_FILE_SHELL)" << endl << endl;
                break;
        }

        // ====================================
        // output rules for HDR merging

        // write rules for each HDR stack
        // only output pixes that are defined in all input images
        for (unsigned i=0; i < stacks.size(); i++) {
            o << "$(HDR_STACK_" << i << ") : $(HDR_STACK_" << i << "_INPUT)" << endl
            << "\t$(HDRMERGE) -m avg -c -o $(HDR_STACK_" << i << "_SHELL) $(HDR_STACK_" << i << "_INPUT_SHELL)" << endl
            << endl;
        }

        // ====================================
        // output rules for LDR stack merging

        for (unsigned i=0; i < stacks.size(); i++) {
            o << "$(LDR_STACK_" << i << ") : $(LDR_STACK_" << i << "_INPUT)" << endl
            << "\t$(ENFUSE) $(ENFUSE_OPTS) -o $(LDR_STACK_" << i << "_SHELL) $(LDR_STACK_" << i << "_INPUT_SHELL)" << endl
            << "\t- $(EXIFTOOL) -overwrite_original_in_place -TagsFromFile $(INPUT_IMAGE_1_SHELL) $(EXIFTOOL_COPY_ARGS) $(LDR_STACK_" << i << "_SHELL)" << endl
            << endl;
        }

        switch(opts.blendMode) {
            case PanoramaOptions::ENBLEND_BLEND:
                // write rules for blending with enblend
                o << "$(LDR_BLENDED) : $(LDR_LAYERS)" << endl;
                o << "\t$(ENBLEND) $(ENBLEND_LDR_COMP) $(ENBLEND_OPTS) -o $(LDR_BLENDED_SHELL) $(LDR_LAYERS_SHELL) " << endl;
                o << "\t- $(EXIFTOOL) -overwrite_original_in_place -TagsFromFile $(INPUT_IMAGE_1_SHELL) $(EXIFTOOL_COPY_ARGS) $(LDR_BLENDED_SHELL)" << endl << endl;

                // for LDR exposure blend planes
                for (unsigned i=0; i < similarExposures.size(); i++) {
                    o << "$(LDR_EXPOSURE_LAYER_" << i <<") : $(LDR_EXPOSURE_LAYER_" << i << "_INPUT)" << endl
                      << "\t$(ENBLEND) $(ENBLEND_LDR_COMP) $(ENBLEND_OPTS) -o $(LDR_EXPOSURE_LAYER_" << i <<"_SHELL) $(LDR_EXPOSURE_LAYER_" << i << "_INPUT_SHELL)" << endl
                      << "\t-$(EXIFTOOL) -overwrite_original_in_place -TagsFromFile $(INPUT_IMAGE_1_SHELL) $(EXIFTOOL_COPY_ARGS) $(LDR_EXPOSURE_LAYER_" << i <<"_SHELL)" << endl << endl;
                }

                // rules for enfuse blending
                o << "$(LDR_STACKED_BLENDED) : $(LDR_STACKS)" << endl
                  << "\t$(ENBLEND) $(ENBLEND_LDR_COMP) $(ENBLEND_OPTS) -o $(LDR_STACKED_BLENDED_SHELL) $(LDR_STACKS_SHELL) " << endl
                  << "\t- $(EXIFTOOL) -overwrite_original_in_place -TagsFromFile $(INPUT_IMAGE_1_SHELL) $(EXIFTOOL_COPY_ARGS) $(LDR_STACKED_BLENDED_SHELL)" << endl << endl;

                // rules for hdr blending
                o << "$(HDR_BLENDED) : $(HDR_STACKS)" << endl;
                o << "\t$(ENBLEND) $(ENBLEND_HDR_COMP) $(ENBLEND_OPTS) -o $(HDR_BLENDED_SHELL) $(HDR_STACKS_SHELL) " << endl << endl;

                break;
            case PanoramaOptions::NO_BLEND:
                o << "$(LDR_BLENDED) : $(LDR_LAYERS)" << endl
                  << "\t-$(RM) $(LDR_BLENDED_SHELL)" << endl
                  << "\t$(PTROLLER) -o $(LDR_BLENDED_SHELL) $(LDR_LAYERS_SHELL) " << endl
                  << "\t- $(EXIFTOOL) -overwrite_original_in_place -TagsFromFile $(INPUT_IMAGE_1_SHELL) $(EXIFTOOL_COPY_ARGS) $(LDR_BLENDED_SHELL)" << endl << endl;

                // for LDR exposure blend planes
                for (unsigned i=0; i < similarExposures.size(); i++) {
                    o << "$(LDR_EXPOSURE_LAYER_" << i <<") : $(LDR_EXPOSURE_LAYER_" << i << "_INPUT)" << endl
                      << "\t-$(RM) $(LDR_EXPOSURE_LAYER_" << i <<"_SHELL)" << endl
                      << "\t$(PTROLLER) -o $(LDR_EXPOSURE_LAYER_" << i <<"_SHELL) $(LDR_EXPOSURE_LAYER_" << i << "_INPUT_SHELL)" << endl
                      << "\t- $(EXIFTOOL) -overwrite_original_in_place -TagsFromFile $(INPUT_IMAGE_1_SHELL) $(EXIFTOOL_COPY_ARGS) $(LDR_EXPOSURE_LAYER_" << i <<"_SHELL)" << endl << endl;
                }

                o << "$(LDR_STACKED_BLENDED) : $(LDR_STACKS)" << endl
                  << "\t-$(RM) $(LDR_STACKED_BLENDED_SHELL)" << endl
                  << "\t$(PTROLLER) -o $(LDR_STACKED_BLENDED_SHELL) $(LDR_STACKS_SHELL)" << endl
                  << "\t- $(EXIFTOOL) -overwrite_original_in_place -TagsFromFile $(INPUT_IMAGE_1_SHELL) $(EXIFTOOL_COPY_ARGS) $(LDR_STACKED_BLENDED_SHELL)" << endl << endl;

                // rules for non-blended HDR panoramas
                o << "$(HDR_BLENDED) : $(HDR_LAYERS)" << endl;
                o << "\t$(HDRMERGE) -m avg -o $(HDR_BLENDED_SHELL) $(HDR_LAYERS_SHELL)"   << endl << endl;

                break;
            case PanoramaOptions::PTBLENDER_BLEND:
                // TODO: output PTBlender + PTmasker + PTroller rules
                break;
            case PanoramaOptions::SMARTBLEND_BLEND:
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
