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

#include <hugin_config.h>
#include <hugin_version.h>
#include <fstream>
#include <sstream>

#include <vigra/error.hxx>
#include <vigra/impex.hxx>
#include <exiv2/image.hpp>
#include <exiv2/exif.hpp>
#ifdef WIN32
 #include <getopt.h>
#else
 #include <unistd.h>
#endif

#include <appbase/ProgressDisplayOld.h>
#include <nona/SpaceTransform.h>
#include <photometric/ResponseTransform.h>

#include <hugin_basic.h>

#include <foreign/lensdb/PTLensDB.h>

#include <tiffio.h>
#include <vigra_ext/MultiThreadOperations.h>
#include <vigra_ext/ImageTransforms.h>


using namespace std;
using namespace vigra;
//using namespace vigra_ext;
using namespace hugin_utils;
using namespace HuginBase;


template <class SrcImgType, class FlatImgType, class DestImgType>
void correctImage(SrcImgType & srcImg,
                  const FlatImgType & srcFlat,
                  SrcPanoImage src,
                  vigra_ext::Interpolator interpolator,
                  DestImgType & destImg,
                  bool doCrop,
                  AppBase::MultiProgressDisplay & progress);

template <class PIXELTYPE>
void correctRGB(SrcPanoImage & src, ImageImportInfo & info, const char * outfile,
                bool crop, const std::string & compression, AppBase::MultiProgressDisplay & progress);

bool getPTLensCoef(const char * fn, string cameraMaker, string cameraName,
                   string lensName, float focalLength, vector<double> & coeff);




static void usage(const char * name)
{
    cerr << name << ": correct lens distortion, vignetting and chromatic abberation" << std::endl
         << "fulla version " << DISPLAY_VERSION << endl
         << std::endl
         << "Usage: " << name  << " [options] inputfile(s) " << std::endl
         << "   option are: " << std::endl
         << "      -g a:b:c:d       Radial distortion coefficient for all channels, (a, b, c, d)" << std::endl
         << "      -b a:b:c:d       Radial distortion coefficents for blue channel, (a, b, c, d)" << std::endl
         << "                        this is applied on top of the -g distortion coefficients," << endl
         << "                        use for TCA corr" << std::endl
         << "      -r a:b:c:d       Radial distortion coefficents for red channel, (a, b, c, d)" << std::endl
         << "                        this is applied on top of the -g distortion coefficients," << endl
         << "                        use for TCA corr" << std::endl
         << "      -p               Try to read radial distortion coefficients for green" << endl
         << "                         channel from PTLens database" << std::endl
         << "      -m Canon         Camera manufacturer, for PTLens database query" << std::endl
         << "      -n Camera        Camera name, for PTLens database query" << std::endl
         << "      -l Lens          Lens name, for PTLens database query" << std::endl
         << "                        if not specified, a list of possible lenses is displayed" << std::endl
         << "      -d 50            specify focal length in mm, for PTLens database query" << std::endl
         << "      -s               do not rescale the image to avoid black borders." << std::endl
         << endl
         << "      -f filename      Vignetting correction by flatfield division" << std::endl
         << "                        I = I / c,    c = flatfield / mean(flatfield)" << std::endl
         << "      -c a:b:c:d       radial vignetting correction by division:" << std::endl
         << "                        I = I / c,    c = a + b*r^2 + c*r^4 + d*r^6" << std::endl
         << "      -i value         gamma of input data. used for gamma correction" << std::endl
         << "                        before and after flatfield correction" << std::endl
         << "      -t n             Number of threads that should be used during processing" << std::endl
         << "      -h               Display help (this text)" << std::endl
         << "      -o name          set output filename. If more than one image is given," << std::endl
         << "                        the name will be uses as suffix (default suffix: _corr)" << std::endl
         << "      -e value         Compression of the output files" << std::endl
         << "                        For jpeg output: 0-100" << std::endl
         << "                        For tiff output: PACKBITS, DEFLATE, LZW" << std::endl
         << "      -x X:Y           Horizontal and vertical shift" << std::endl
         << "      -v               Verbose" << std::endl;
}


int main(int argc, char *argv[])
{
    // parse arguments
    const char * optstring = "e:g:b:r:pm:n:l:d:sf:c:i:t:ho:x:v";
    int o;
    //bool verbose_flag = true;

    opterr = 0;

    vector<double> vec4(4);
    bool doFlatfield = false;
    bool doVigRadial = false;
    bool doCropBorders = true;
    unsigned nThreads=1;
    unsigned verbose = 0;

    std::string batchPostfix("_corr");
    std::string outputFile;
    std::string compression;
    bool doPTLens = false;
    std::string cameraMaker;
    std::string cameraName;
    std::string lensName;
    float focalLength=0;
    double gamma = 1.0;
    double shiftX = 0;
    double shiftY = 0;

    SrcPanoImage c;
    while ((o = getopt (argc, argv, optstring)) != -1)
        switch (o) {
        case 'e':
             compression = optarg;
             break;
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
        case 's':
            doCropBorders = false;
            break;
        case 'f':
            c.setFlatfieldFilename(optarg);
            doFlatfield = true;
            break;
        case 'i':
            gamma = atof(optarg);
            c.setGamma(gamma);
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
            doVigRadial=true;
            break;
        case '?':
        case 'h':
            usage(argv[0]);
            return 0;
        case 't':
            nThreads = atoi(optarg);
            break;
        case 'o':
            outputFile = optarg;
            break;
        case 'x':
            if (sscanf(optarg, "%lf:%lf", &shiftX, &shiftY) != 2)
            {
                std::cerr << std::endl << "Error: invalid -x argument" << std::endl <<std::endl;
                usage(argv[0]);
                return 1;
            }
	    c.setRadialDistortionCenterShift(FDiff2D(shiftX, shiftY));
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
            string basen = stripExtension(name);
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

    AppBase::StreamMultiProgressDisplay pdisp(cout);

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
                    correctRGB<RGBValue<UInt8> >(c, info, outIt->c_str(), doCropBorders, compression, pdisp);
                }
                else if (strcmp(pixelType, "UINT16") == 0) {
                    correctRGB<RGBValue<UInt16> >(c, info, outIt->c_str(), doCropBorders, compression, pdisp);
                } else if (strcmp(pixelType, "INT16") == 0) {
                    correctRGB<RGBValue<Int16> >(c, info, outIt->c_str(), doCropBorders, compression, pdisp);
                } else if (strcmp(pixelType, "UINT32") == 0) {
                    correctRGB<RGBValue<UInt32> >(c, info, outIt->c_str(), doCropBorders, compression, pdisp);
                } else if (strcmp(pixelType, "FLOAT") == 0) {
                    correctRGB<RGBValue<float> >(c, info, outIt->c_str(), doCropBorders, compression, pdisp);
                } else if (strcmp(pixelType, "DOUBLE") == 0) {
                    correctRGB<RGBValue<double> >(c, info, outIt->c_str(), doCropBorders, compression, pdisp);
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




/** remap a single image
 *
 *  Be careful, might modify srcImg (vignetting and brightness correction)
 *
 */
template <class SrcImgType, class FlatImgType, class DestImgType>
void correctImage(SrcImgType & srcImg,
                  const FlatImgType & srcFlat,
                  SrcPanoImage src,
                  vigra_ext::Interpolator interpolator,
                  DestImgType & destImg,
                  bool doCrop,
                  AppBase::MultiProgressDisplay & progress)
{
    typedef typename SrcImgType::value_type SrcPixelType;
    typedef typename DestImgType::value_type DestPixelType;

    typedef typename vigra::NumericTraits<SrcPixelType>::RealPromote RSrcPixelType;

    // prepare some information required by multiple types of vignetting correction
    progress.pushTask(AppBase::ProgressTask("correcting image", ""));

    vigra::Diff2D shiftXY(- roundi(src.getRadialDistortionCenterShift().x),
			  - roundi(src.getRadialDistortionCenterShift().y));

    if( (src.getVigCorrMode() & SrcPanoImage::VIGCORR_FLATFIELD)
        || (src.getVigCorrMode() & SrcPanoImage::VIGCORR_RADIAL) )
    {
        src.setResponseType(HuginBase::SrcPanoImage::RESPONSE_LINEAR);
        Photometric::InvResponseTransform<SrcPixelType,SrcPixelType> invResp(src);
	    invResp.enforceMonotonicity();
        if (src.getVigCorrMode() & SrcPanoImage::VIGCORR_FLATFIELD) {
            invResp.setFlatfield(&srcFlat);
        }
        vigra_ext::transformImageSpatial(srcImageRange(srcImg), destImage(srcImg), invResp, vigra::Diff2D(0,0));
    }

    double scaleFactor=1.0;
    // radial distortion correction
    if (doCrop) {
        scaleFactor=Nona::estScaleFactorForFullFrame(src);
        DEBUG_DEBUG("Black border correction scale factor: " << scaleFactor);
        double sf=scaleFactor;
        vector<double> radGreen = src.getRadialDistortion();
        for (int i=0; i < 4; i++) {
            radGreen[3-i] *=sf;
            sf *=scaleFactor;
        }
        src.setRadialDistortion(radGreen);
    }

    // hmm, dummy alpha image...
    BImage alpha(destImg.size());

    vigra_ext::PassThroughFunctor<typename SrcPixelType::value_type> ptf;

    if (src.getCorrectTCA())
    {
        /*
        DEBUG_DEBUG("Final distortion correction parameters:" << endl
                << "r: " << radRed[0] << " " << radRed[1] << " " << radRed[2] << " " << radRed[3]  << endl
                << "g: " << radGreen[0] << " " << radGreen[1] << " " << radGreen[2] << " " << radGreen[3] << endl
                << "b: " << radBlue[0] << " " << radBlue[1] << " " << radBlue[2] << " " << radBlue[3] << endl);
        */
        // remap individual channels
        Nona::SpaceTransform transfr;
        transfr.InitRadialCorrect(src, 0);
        if (transfr.isIdentity()) {
            vigra::copyImage(srcIterRange(srcImg.upperLeft(), srcImg.lowerRight(), RedAccessor<SrcPixelType>()),
                             destIter(destImg.upperLeft(), RedAccessor<DestPixelType>()));
        } else {
            vigra_ext::transformImage(srcIterRange(srcImg.upperLeft(), srcImg.lowerRight(), RedAccessor<SrcPixelType>()),
                           destIterRange(destImg.upperLeft(), destImg.lowerRight(), RedAccessor<DestPixelType>()),
                           destImage(alpha),
                           shiftXY,
                           transfr,
                           ptf,
                           false,
                           vigra_ext::INTERP_SPLINE_16,
                           progress);
        }

        Nona::SpaceTransform transfg;
        transfg.InitRadialCorrect(src, 1);
        if (transfg.isIdentity()) {
            vigra::copyImage(srcIterRange(srcImg.upperLeft(), srcImg.lowerRight(), GreenAccessor<SrcPixelType>()),
                             destIter(destImg.upperLeft(), GreenAccessor<DestPixelType>()));
        } else {
            transformImage(srcIterRange(srcImg.upperLeft(), srcImg.lowerRight(), GreenAccessor<SrcPixelType>()),
                           destIterRange(destImg.upperLeft(), destImg.lowerRight(), GreenAccessor<DestPixelType>()),
                           destImage(alpha),
                           shiftXY,
                           transfg,
                           ptf,
                           false,
                           vigra_ext::INTERP_SPLINE_16,
                           progress);
        }

        Nona::SpaceTransform transfb;
        transfb.InitRadialCorrect(src, 2);
        if (transfb.isIdentity()) {
            vigra::copyImage(srcIterRange(srcImg.upperLeft(), srcImg.lowerRight(), BlueAccessor<SrcPixelType>()),
                             destIter(destImg.upperLeft(), BlueAccessor<DestPixelType>()));
        } else {
            transformImage(srcIterRange(srcImg.upperLeft(), srcImg.lowerRight(), BlueAccessor<SrcPixelType>()),
                           destIterRange(destImg.upperLeft(), destImg.lowerRight(), BlueAccessor<DestPixelType>()),
                           destImage(alpha),
                           shiftXY,
                           transfb,
                           ptf,
                           false,
                           vigra_ext::INTERP_SPLINE_16,
                           progress);
        }
    } else {
        // remap with the same coefficient.
        Nona::SpaceTransform transf;
        transf.InitRadialCorrect(src, 1);
        vector <double> radCoeff = src.getRadialDistortion();
        if (transf.isIdentity() || 
            (radCoeff[0] == 0.0 && radCoeff[1] == 0.0 && radCoeff[2] == 0.0 && radCoeff[3] == 1.0))
        {
            vigra::copyImage(srcImageRange(srcImg),
                             destImage(destImg));
        } else {
            vigra_ext::PassThroughFunctor<SrcPixelType> ptfRGB;
            transformImage(srcImageRange(srcImg),
                           destImageRange(destImg),
                           destImage(alpha),
                           shiftXY,
                           transf,
                           ptfRGB,
                           false,
                           vigra_ext::INTERP_SPLINE_16,
                           progress);
        }
    }
}


//void correctRGB(SrcImageInfo & src, ImageImportInfo & info, const char * outfile)
template <class PIXELTYPE>
void correctRGB(SrcPanoImage & src, ImageImportInfo & info, const char * outfile,
                bool crop, const std::string & compression, AppBase::MultiProgressDisplay & progress)
{
    vigra::BasicImage<RGBValue<float> > srcImg(info.size());
    vigra::BasicImage<PIXELTYPE> output(info.size());
    importImage(info, destImage(srcImg));
    FImage flatfield;
    if (src.getVigCorrMode() & SrcPanoImage::VIGCORR_FLATFIELD) {
        ImageImportInfo finfo(src.getFlatfieldFilename().c_str());
        flatfield.resize(finfo.size());
        importImage(finfo, destImage(flatfield));
    }
    correctImage(srcImg, flatfield, src, vigra_ext::INTERP_SPLINE_16, output, crop, progress);
    ImageExportInfo outInfo(outfile);
    outInfo.setICCProfile(info.getICCProfile());
    outInfo.setPixelType(info.getPixelType());
    if (compression.size() > 0) {
        outInfo.setCompression(compression.c_str());
    }
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
                << " You need to specify the location of \"profile.txt\"." << endl
                << " Please set the PTLENS_PROFILE environment variable, for example:" << endl
                << " PTLENS_PROFILE=$HOME/.ptlens/profile.txt" << endl;
        return false;
    }
            // load database from file
    PTLDB_DB * db = PTLDB_readDB(profilePath);
    if (! db) {
        fprintf(stderr,"Failed to read PTLens profile: %s\n", profilePath);
        return false;
    }

    // TODO: Extract lens name from camera makernotes

    Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(fn);
    assert (image.get() != 0);
    image->readMetadata();

    Exiv2::ExifData &exifData = image->exifData();
    if (exifData.empty()) {
        std::cout << fn << ": no EXIF data found in file" << std::endl;
    } else {
        if (cameraMaker.size() == 0) {
            cameraMaker = exifData["Exif.Image.Make"].toString();
        }
        if (cameraName.size() == 0) {
            cameraName = exifData["Exif.Image.Model"].toString();
        }
        if (focalLength == 0.0f) {
            focalLength = exifData["Exif.Photo.FocalLength"].toFloat();
        }
    }


    // TODO: Replace this with lensfun
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

