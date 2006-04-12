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
#include <jhead/jhead.h>

#ifdef WIN32
 #include <getopt.h>
#else
 #include <unistd.h>
#endif

#include "panoinc.h"
#include "PT/Panorama.h"
#include "PT/Stitcher.h"
#include "common/PTLensDB.h"

#include <tiff.h>

using namespace vigra;
using namespace PT;
using namespace std;
using namespace utils;


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
    // check if image should be tca corrected, and calculate scale factor accordingly.
    double scaleFactor=1;
    vector<double> radGreen(4);
    radGreen = src.getRadialDistortion();
    vector<double> radRed(4);
    combinePolynom3(radGreen , src.getRadialDistortionRed(), radRed);
    vector<double> radBlue(4);
    combinePolynom3(radGreen, src.getRadialDistortionBlue(), radBlue);
    if (src.getCorrectTCA())
    {
        // calculate scaling factor required to avoid black borders.
        double scaleR = estRadialScaleCrop(radRed, src.getSize().x, src.getSize().y);
        double scaleG = estRadialScaleCrop(radGreen, src.getSize().x, src.getSize().y);
        double scaleB = estRadialScaleCrop(radBlue, src.getSize().x, src.getSize().y);
        scaleFactor = std::max(std::max(scaleR,scaleG),scaleB);
    } else {
        scaleFactor = estRadialScaleCrop(radGreen, src.getSize().x, src.getSize().y);
    }
    // adjust radial distortion coeffs
    // expand or shrink image
    scaleFactor = 1/scaleFactor;
    DEBUG_DEBUG("Black border correction factor: " << scaleFactor);
    double sf=scaleFactor;
    for (int i=0; i < 4; i++) {
        radRed[3-i] *=sf;
        radGreen[3-i] *=sf;
        radBlue[3-i] *=sf;
        sf *=scaleFactor;
    }

    if (src.getCorrectTCA())
    {
        // remap individual channels
        SpaceTransform transf;
        transf.InitRadialCorrect(src.getSize(), radRed, FDiff2D(0,0));
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

        transf.InitRadialCorrect(src.getSize(), radGreen, FDiff2D(0,0));
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

        transf.InitRadialCorrect(src.getSize(), radBlue, FDiff2D(0,0));
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
        transf.InitRadialCorrect(src.getSize(), radGreen, FDiff2D(0,0));
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


bool getPTLensCoef(const char * fn, string cameraMaker, string cameraName,
                   string lensName, float focalLength, vector<double> & coeff)
{
    int verbose_flag = 1;
    const char * profilePath = getenv("PTLENS_PROFILE");
    if (profilePath == NULL)
    {
        cerr << "ERROR: " << endl
                << " You specify where \"profile.txt\" is!" << endl
                << " Use PTLENS_PROFILE environment variable, example:" << endl
                << " PTLENS_PROFILE=$HOME/.ptlens/profile.txt" << endl;
        return false;
    }
            // load database from file
    PTLDB_DB * db = PTLDB_readDB(profilePath);
    if (! db) {
        fprintf(stderr,"Failed to read PTLens profile: %s\n", profilePath);
        return false;
    }

    // TODO: try to extract camera and lens information from input file, for example using
    // exiftool, and a file with mapping for the lens name.
    // use simple jhead reader first.. works only with jpeg files

    std::string ext = utils::getExtension(fn);
    if ( ext == "jpg" || ext == "JPG" || ext == "JPEG" || ext == "jpeg") {
        //read the exif data
        ImageInfo_t exif;
        ResetJpgfile();
        // Start with an empty image information structure.

        memset(&exif, 0, sizeof(exif));
        exif.FlashUsed = -1;
        exif.MeteringMode = -1;

        if (!ReadJpegFile(exif,fn, READ_EXIF)){
            puts("Exif read failed");
        } else {
            // set if not overridden by camera
            if (cameraMaker.size() == 0) {
                cameraMaker = exif.CameraMake;
            }
            if (cameraName.size() == 0) {
                cameraName = exif.CameraModel;
            }
            if (focalLength == 0.0f) {
                focalLength = exif.FocalLength;
            }
        }
    }

    PTLDB_CamNode * thisCamera = PTLDB_findCamera(db, cameraMaker.c_str(), cameraName.c_str());
    if (!thisCamera) {
        fprintf(stderr, "could not find camera: %s, %s\n", cameraMaker.c_str(), cameraName.c_str());
        return false;
    }
    PTLDB_LnsNode * thisLens = PTLDB_findLens(db, lensName.c_str(), thisCamera);
    if (thisLens == NULL)
    {
        fprintf(stderr, "Lens \"%s\" not found in database.\n", lensName.c_str());
        fprintf(stderr,"Available lenses for camera: %s\n", thisCamera->menuModel);
        PTLDB_LnsNode * lenses = PTLDB_findLenses(db, thisCamera);
        while (lenses != NULL)
        {
            fprintf(stderr,"%s\n", lenses->menuLens);
            lenses = lenses->nextLns;
        }
        return false;
    }

    ImageImportInfo info(fn);
    // retrieve distortion coefficients
    PTLDB_ImageInfo img;
    img.camera = thisCamera;
    img.lens = thisLens;
    img.width = info.size().x;
    img.height = info.size().y;
    img.focalLength = focalLength;
    img.converterDetected = 0;
    img.resize=0;

    PTLDB_RadCoef coef;
    PTLDB_getRadCoefs(db, &img, &coef);
    if (verbose_flag)
    {
        fprintf(stderr,"%s %s,  Lens %s @ %f mm\n", thisCamera->menuMake, thisCamera->menuModel, lensName.c_str(), focalLength);
        fprintf(stderr, "PTLens coeff:  a=%8.6lf  b=%8.6lf  c=%8.6lf  d=%8.6lf\n", coef.a, coef.b, coef.c, coef.d);
    }
    coeff[0] = coef.a;
    coeff[1] = coef.b;
    coeff[2] = coef.c;
    coeff[3] = coef.d;
    return true;
}

static void usage(const char * name)
{
    cerr << name << ": correct lens distortion, vignetting and chromatic abberation" << std::endl
         << "version " << PACKAGE_VERSION << endl
         << std::endl
         << "Usage: " << name  << " [options] inputfile(s) " << std::endl
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
         << "      -p               Try to read radial distortion coefficients (-g) from PTLens database " << std::endl
         << "      -m Canon         Camera manufacturer, for PTLens database query" << std::endl
         << "      -n Camera        Camera name, for PTLens database query" << std::endl
         << "      -l Lens          Lens name, for PTLens database query" << std::endl
         << "                        if not specified, a list of possible lenses is displayed" << std::endl
         << "      -d 50            specify focal length in mm, for PTLens database query" << std::endl
         << "      -t n             Number of threads that should be used during processing" << std::endl
         << "      -h               Display help (this text)" << std::endl
         << "      -o name          set output filename. If more than one image is given," << std::endl
         << "                        the name will be uses as suffix (default suffix: _corr)" << std::endl
         << "      -v               Verbose" << std::endl;
}


int main(int argc, char *argv[])
{
    // parse arguments
    const char * optstring = "g:r:b:f:apm:n:l:d:c:t:vho:";
    int o;
    bool verbose_flag = true;

    opterr = 0;

    vector<double> vec4(4);
    bool doFlatfield = false;
    bool doVigRadial = false;
    bool doVigAddition = false;
    unsigned nThreads=1;
    unsigned verbose = 0;
    
    std::string batchPostfix("_corr");
    std::string outputFile;
    bool doPTLens = false;
    std::string cameraMaker;
    std::string cameraName;
    std::string lensName;
    float focalLength=0;
    SrcPanoImage c;
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
        case 'p':
            doPTLens = true;
            break;
        case 'm':
            cameraMaker = optarg;
            doPTLens = true;
            break;
        case 'n':
            cameraName = optarg;
            doPTLens = true;
            break;
        case 'l':
            lensName = optarg;
            doPTLens = true;
            break;
        case 'd':
            focalLength = atof(optarg);
            doPTLens = true;
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
        case '?':
        case 'h':
            usage(argv[0]);
            return 1;
        case 't':
            nThreads = atoi(optarg);
            break;
        case 'o':
            outputFile = optarg;
            break;
        case 'v':
            verbose++;
            break;
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

    unsigned nFiles = argc - optind;
    if (nFiles == 0) {
        std::cerr << std::endl << "Error: No input file(s) specified" << std::endl <<std::endl;
        usage(argv[0]);
        return 1;
    }

    // get input images.
    vector<string> inFiles;
    vector<string> outFiles;
    if (nFiles == 1) {
        if (outputFile.length() !=0) {
            inFiles.push_back(string(argv[optind]));
            outFiles.push_back(outputFile);
        } else {
            string name = string(argv[optind]);
            inFiles.push_back(name);
            string basen = utils::stripExtension(name);
            outFiles.push_back(basen.append(batchPostfix.append(".").append(getExtension(name))));
        }
    } else {
        // multiple files
        if (outputFile.length() != 0) {
            batchPostfix = outputFile;
        }
        for (int i = optind; i < argc; i++) {
            string name = string(argv[i]);
            inFiles.push_back(name);
            outFiles.push_back(stripExtension(name) + batchPostfix + "." + getExtension(name));
        }
    }


    // suppress tiff warnings
    TIFFSetWarningHandler(0);

    utils::StreamMultiProgressDisplay pdisp(cout);

    if (nThreads == 0) nThreads = 1;
    vigra_ext::ThreadManager::get().setNThreads(nThreads);

    try {
        vector<string>::iterator outIt = outFiles.begin();
        for (vector<string>::iterator inIt = inFiles.begin(); inIt != inFiles.end() ; ++inIt, ++outIt)
        {
            if (verbose > 0) {
                cerr << "Correcting " << *inIt << " -> " << *outIt << endl;
            }
            c.setFilename(*inIt);

            // load the input image
            vigra::ImageImportInfo info(inIt->c_str());
            const char * pixelType = info.getPixelType();
            int bands = info.numBands();
            int extraBands = info.numExtraBands();

            // if ptlens support required, load database
            if (doPTLens) {
                if (getPTLensCoef(inIt->c_str(), cameraMaker.c_str(), cameraName.c_str(),
                    lensName.c_str(), focalLength, vec4)) 
                {
                    c.setRadialDistortion(vec4);
                } else {
                    cerr << "Error: could not extract correction parameters from PTLens database" << endl;
                    return 1;
                }
            }
            c.setSize(info.size());
            // stitch the pano with a suitable image type
            if (bands == 3 || bands == 4 && extraBands == 1) {
                // TODO: add more cases
                if (strcmp(pixelType, "UINT8") == 0) {
                    correctRGB<RGBValue<UInt8> >(c, info, outIt->c_str(), pdisp);
                } else if (strcmp(pixelType, "UINT16") == 0) {
                    correctRGB<RGBValue<UInt16> >(c, info, outIt->c_str(), pdisp);
                } else if (strcmp(pixelType, "INT16") == 0) {
                    correctRGB<RGBValue<Int16> >(c, info, outIt->c_str(), pdisp);
                } else if (strcmp(pixelType, "UINT32") == 0) {
                    correctRGB<RGBValue<UInt32> >(c, info, outIt->c_str(), pdisp);
                } else if (strcmp(pixelType, "FLOAT") == 0) {
                    correctRGB<RGBValue<float> >(c, info, outIt->c_str(), pdisp);
                } else if (strcmp(pixelType, "DOUBLE") == 0) {
                    correctRGB<RGBValue<double> >(c, info, outIt->c_str(), pdisp);
                }
            } else {
                DEBUG_ERROR("unsupported depth, only 3 channel images are supported");
                throw std::runtime_error("unsupported depth, only 3 channels images are supported");
                return 1;
            }
        }
    } catch (std::exception & e) {
        cerr << "caught exception: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
