// -*- c-basic-offset: 4 -*-

/** @file fulla.cpp
 *
 *  @brief a tool to perform distortion, vignetting and chromatic abberation correction.
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

#include <config.h>
#include <fstream>
#include <sstream>

#include <vigra/error.hxx>
#include <vigra/impex.hxx>

#ifdef WIN32
 #include <getopt.h>
#else
 #include <unistd.h>
#endif

#include "panoinc.h"
#include "PT/Panorama.h"
#include "PT/Stitcher.h"

#include <tiff.h>

using namespace vigra;
using namespace PT;
using namespace std;


/** remap a single image
 *
 *  Be careful, might modify srcImg (vignetting and brightness correction)
 *
 */
template <class SrcImgType, class FlatImgType, class DestImgType>
void correctImage(SrcImgType & srcImg,
                  const FlatImgType & srcFlat,
                  const PT::SrcPanoImage & src,
                  vigra_ext::Interpolator interpolator,
                  DestImgType & destImg,
                  utils::MultiProgressDisplay & progress)
{
    typedef typename SrcImgType::value_type SrcPixelType;
    typedef typename DestImgType::value_type DestPixelType;

    typedef typename vigra::NumericTraits<SrcPixelType>::RealPromote RSrcPixelType;

    // prepare some information required by multiple types of vignetting correction
    bool vigCorrDivision = (src.getVigCorrMode() & PT::SrcPanoImage::VIGCORR_DIV)>0;

    RSrcPixelType ka,kb;
    bool doBrightnessConversion = convertKParams(src.getBrightnessFactor(),
            src.getBrightnessOffset(),
            ka, kb);
    bool dither = vigra_ext::ditheringNeeded(SrcPixelType());

    double gMaxVal = vigra_ext::VigCorrTraits<typename DestImgType::value_type>::max();

    if (src.getVigCorrMode() & SrcPanoImage::VIGCORR_FLATFIELD) {
        vigra_ext::flatfieldVigCorrection(vigra::srcImageRange(srcImg),
                                          vigra::srcImage(srcFlat), 
                                          vigra::destImage(srcImg), src.getGamma(), gMaxVal,
                                          vigCorrDivision, ka, kb, dither);
    } else if (src.getVigCorrMode() & SrcPanoImage::VIGCORR_RADIAL) {
        progress.setMessage(std::string("radial vignetting correction ") + utils::stripPath(src.getFilename()));

        vigra_ext::radialVigCorrection(srcImageRange(srcImg), destImage(srcImg),
                                       src.getGamma(), gMaxVal,
                                       src.getRadialVigCorrCoeff(), 
                                       src.getRadialVigCorrCenter(),
                                       vigCorrDivision, ka, kb, dither);
    } else if (src.getGamma() != 1.0 && doBrightnessConversion ) {
        progress.setMessage(std::string("inverse gamma & brightness corr") + utils::stripPath(src.getFilename()));
        vigra_ext::applyGammaAndBrightCorrection(srcImageRange(srcImg), destImage(srcImg),
                src.getGamma(), gMaxVal, ka,kb);
    } else if (doBrightnessConversion ) {
        progress.setMessage(std::string("brightness correction ") + utils::stripPath(src.getFilename()));
        vigra_ext::applyBrightnessCorrection(srcImageRange(srcImg), destImage(srcImg),
                                             ka,kb);
    } else if (src.getGamma() != 1.0 ) {
        progress.setMessage(std::string("inverse gamma correction ") + utils::stripPath(src.getFilename()));
        vigra_ext::applyGammaCorrection(srcImageRange(srcImg), destImage(srcImg),
                                        src.getGamma(), gMaxVal);
    }

    // hmm, dummy alpha image...
    BImage alpha(srcImg.size());
    // correct image
    if (src.getCorrectTCA())
    {
        // remap individual channels
        SpaceTransform transf;
        transf.InitCorrect(src, 0);
        if (transf.isIdentity()) {
            vigra::copyImage(srcIterRange(srcImg.upperLeft(), srcImg.lowerRight(), RedAccessor<SrcPixelType>()),
                             destIter(destImg.upperLeft(), RedAccessor<DestPixelType>()));
        } else {
            transformImage(srcIterRange(srcImg.upperLeft(), srcImg.lowerRight(), RedAccessor<SrcPixelType>()),
                        destIterRange(destImg.upperLeft(), destImg.lowerRight(), RedAccessor<DestPixelType>()),
                        destImage(alpha),
                        vigra::Diff2D(0,0),
                        transf,
                        false,
                        vigra_ext::INTERP_SPLINE_16,
                        progress);
        }

        transf.InitCorrect( src, 1);
        if (transf.isIdentity()) {
            vigra::copyImage(srcIterRange(srcImg.upperLeft(), srcImg.lowerRight(), GreenAccessor<SrcPixelType>()),
                             destIter(destImg.upperLeft(), GreenAccessor<DestPixelType>()));
        } else {
            transformImage(srcIterRange(srcImg.upperLeft(), srcImg.lowerRight(), GreenAccessor<SrcPixelType>()),
                       destIterRange(destImg.upperLeft(), destImg.lowerRight(), GreenAccessor<DestPixelType>()),
                       destImage(alpha),
                       vigra::Diff2D(0,0),
                       transf,
                       false,
                       vigra_ext::INTERP_SPLINE_16,
                       progress);
        }

        transf.InitCorrect(src, 2);
        if (transf.isIdentity()) {
            vigra::copyImage(srcIterRange(srcImg.upperLeft(), srcImg.lowerRight(), BlueAccessor<SrcPixelType>()),
                             destIter(destImg.upperLeft(), BlueAccessor<DestPixelType>()));
        } else {
            transformImage(srcIterRange(srcImg.upperLeft(), srcImg.lowerRight(), BlueAccessor<SrcPixelType>()),
                       destIterRange(destImg.upperLeft(), destImg.lowerRight(), BlueAccessor<DestPixelType>()),
                       destImage(alpha),
                       vigra::Diff2D(0,0),
                       transf,
                       false,
                       vigra_ext::INTERP_SPLINE_16,
                       progress);
        }
    } else {
        // remap with the same coefficient.
        SpaceTransform transf;
        transf.InitCorrect(src);
        if (transf.isIdentity()) {
            vigra::copyImage(srcImageRange(srcImg),
                             destImage(destImg));
        } else {
            transformImage(srcImageRange(srcImg),
                           destImageRange(destImg),
                           destImage(alpha),
                           vigra::Diff2D(0,0),
                           transf,
                           false,
                           vigra_ext::INTERP_SPLINE_16,
                           progress);
        }
    }
}

//void correctRGB(SrcImageInfo & src, ImageImportInfo & info, const char * outfile)
template <class PIXELTYPE>
void correctRGB(SrcPanoImage & src, ImageImportInfo & info, const char * outfile,
                utils::MultiProgressDisplay & progress)
{
    vigra::BasicImage<RGBValue<float> > srcImg(info.size());
    vigra::BasicImage<PIXELTYPE> output(info.size());
    importImage(info, destImage(srcImg));
    FImage flatfield;
    if (src.getVigCorrMode() & SrcPanoImage::VIGCORR_RADIAL) {
        ImageImportInfo finfo(src.getFlatfieldFilename().c_str());
        flatfield.resize(finfo.size());
        importImage(finfo, destImage(flatfield));
    }
    correctImage(srcImg, flatfield, src, vigra_ext::INTERP_SPLINE_16, output, progress);
    ImageExportInfo outInfo(outfile);
    outInfo.setICCProfile(info.getICCProfile());
    exportImage(srcImageRange(output), outInfo);
}

static void usage(const char * name)
{
    cerr << name << ": correct lens distortion, vignetting and chromatic abberation" << std::endl
         << std::endl
         << " The following output formats (n option of panotools p script line)" << std::endl
         << " are supported:"<< std::endl
         << std::endl
         << "Usage: " << name  << " [options] inputfile outputfile" << std::endl
         << "   option are: " << std::endl
         << "      -g a:b:c:d       Radial distortion coefficient for all channels, (a, b, c, d)" << std::endl
         << "      -b a:b:c:d       Radial distortion coefficents for blue channel, (a, b, c, d)" << std::endl
         << "                        this is applied on top of the -g distortion coefficients, use for TCA corr" << std::endl
         << "      -r a:b:c:d       Radial distortion coefficents for red channel, (a, b, c, d)" << std::endl
         << "                        this is applied on top of the -g distortion coefficients, use for TCA corr" << std::endl
         << "      -f filename      Vignetting correction by flatfield division" << std::endl
         << "                        I = I / c,    c = flatfield / mean(flatfield)" << std::endl
         << "      -c a:b:c:d       radial vignetting correction by division:" << std::endl
         << "                        I = I / c,    c = a + b*r^2 + c*r^4 + d*r^6" << std::endl
         << "      -a               Correct vignetting by addition, rather than by division" << std::endl
         << "                        I = I + c" << std::endl
         << "      -t n             Number of threads that should be used during processing" << std::endl
         << "      -v               Display version information" << std::endl
         << "      -h               Display help (this text)" << std::endl;
}


int main(int argc, char *argv[])
{
    // parse arguments
    const char * optstring = "g:r:b:f:ac:t:vh";
    int o;

    opterr = 0;

    SrcPanoImage c;
    vector<double> vec4(4);
    bool doFlatfield = false;
    bool doVigRadial = false;
    bool doVigAddition = false;
    unsigned nThreads=1;
    
    while ((o = getopt (argc, argv, optstring)) != -1)
        switch (o) {
        case 'r':
            if (sscanf(optarg, "%lf:%lf:%lf:%lf", &vec4[0], &vec4[1], &vec4[2], &vec4[3]) != 4)
            {
                std::cerr << std::endl << "Error: invalid -r argument" << std::endl <<std::endl;
                usage(argv[0]);
                return 1;
            }
            c.setRadialDistortionRed(vec4);
//            c.radDistRed[3] = 1 - c.radDistRed[0] - c.radDistRed[1] - c.radDistRed[2];
            break;
        case 'g':
            if (sscanf(optarg, "%lf:%lf:%lf:%lf", &vec4[0], &vec4[1], &vec4[2], &vec4[3]) != 4)
            {
                std::cerr << std::endl << "Error: invalid -g argument" << std::endl <<std::endl;
                usage(argv[0]);
                return 1;
            }
            c.setRadialDistortion(vec4);
            //            c.radDistBlue[3] = 1 - c.radDistBlue[0] - c.radDistBlue[1] - c.radDistBlue[2];
            break;
        case 'b':
            if (sscanf(optarg, "%lf:%lf:%lf:%lf", &vec4[0], &vec4[1], &vec4[2], &vec4[3]) != 4)
            {
                std::cerr << std::endl << "Error: invalid -b argument" << std::endl <<std::endl;
                usage(argv[0]);
                return 1;
            }
            c.setRadialDistortionBlue(vec4);
            //            c.radDistBlue[3] = 1 - c.radDistBlue[0] - c.radDistBlue[1] - c.radDistBlue[2];
            break;
        case 'f':
            c.setFlatfieldFilename(optarg);
            doFlatfield = true;
            break;
        case 'a':
            doVigAddition = true;
            break;
        case 'c':
            if (sscanf(optarg, "%lf:%lf:%lf:%lf", &vec4[0], &vec4[1], &vec4[2], &vec4[3]) !=4)
            {
                std::cerr << std::endl << "Error: invalid -c argument" << std::endl <<std::endl;
                usage(argv[0]);
                return 1;
            }
            c.setRadialVigCorrCoeff(vec4);
            break;
        case 'v':
            std::cerr << "fulla, version " << PACKAGE_VERSION << endl;
        case '?':
        case 'h':
            usage(argv[0]);
            return 1;
        case 't':
            nThreads = atoi(argv[0]);
            return 1;
        default:
            abort ();
        }

    if (doVigRadial && doFlatfield) {
        std::cerr << std::endl << "Error: cannot use -f and -c at the same time" << std::endl <<std::endl;
        usage(argv[0]);
        return 1;
    }

    SrcPanoImage::VignettingCorrMode vm=SrcPanoImage::VIGCORR_NONE;

    if (doVigRadial) 
        vm = SrcPanoImage::VIGCORR_RADIAL;
    if (doFlatfield)
        vm = SrcPanoImage::VIGCORR_FLATFIELD;
    if (! doVigAddition)
        vm = (SrcPanoImage::VignettingCorrMode) (vm | SrcPanoImage::VIGCORR_DIV);

    c.setVigCorrMode(vm);
    if (argc - optind != 2) {
        std::cerr << std::endl << "Error: No or too many input and output files specified" << std::endl <<std::endl;
        usage(argv[0]);
        return 1;
    }

    // load input image.

    const char * inputFile = argv[optind];
    const char * outputFile = argv[optind+1];

    c.setFilename(inputFile);

    // suppress tiff warnings
    TIFFSetWarningHandler(0);

    utils::StreamMultiProgressDisplay pdisp(cout);

    try {
        if (nThreads == 0) nThreads = 1;
        vigra_ext::ThreadManager::get().setNThreads(nThreads);

        // load the input image
        vigra::ImageImportInfo info(inputFile);
        const char * pixelType = info.getPixelType();
        int bands = info.numBands();
        int extraBands = info.numExtraBands();

        c.setSize(info.size());
        // stitch the pano with a suitable image type
        if (bands == 3 || bands == 4 && extraBands == 1) {
            // TODO: add more cases
            if (strcmp(pixelType, "UINT8") == 0) {
                correctRGB<RGBValue<UInt8> >(c, info, outputFile, pdisp);
            } else if (strcmp(pixelType, "UINT16") == 0) {
                correctRGB<RGBValue<UInt16> >(c, info, outputFile, pdisp);
            } else if (strcmp(pixelType, "INT16") == 0) {
                correctRGB<RGBValue<Int16> >(c, info, outputFile, pdisp);
            } else if (strcmp(pixelType, "UINT32") == 0) {
                correctRGB<RGBValue<UInt32> >(c, info, outputFile, pdisp);
            } else if (strcmp(pixelType, "FLOAT") == 0) {
                correctRGB<RGBValue<float> >(c, info, outputFile, pdisp);
            } else if (strcmp(pixelType, "DOUBLE") == 0) {
                correctRGB<RGBValue<double> >(c, info, outputFile, pdisp);
            }
        } else {
            DEBUG_ERROR("unsupported depth, only 3 channel images are supported");
            throw std::runtime_error("unsupported depth, only 3 channels images are supported");
            return 1;
        }

    } catch (std::exception & e) {
        cerr << "caught exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
