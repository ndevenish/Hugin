// -*- c-basic-offset: 4 -*-

/** @file preprocessor.cpp
 *
 *  @brief A simple preprocessor that uses phase correlation
 *         to initialize a Kanade Lucas Tomasi tracker.
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
#include "vigra_ext/PhaseCorrelation.h"
#include <vigra/impex.hxx>

extern "C" {
#include "klt/klt.h"
}

#include <unistd.h>

#include "panoinc.h"
//#include "PT/Stitcher.h"

using namespace vigra;
using namespace vigra_ext;
using namespace PT;
using namespace std;
using namespace utils;

static void usage(const char * name)
{
    cerr << name << ": create a .pto file with control points, from a given image series" << endl
         << endl
         << "  It uses phase correlation to estimate an initial image shift" << endl
         << "  which is then used by a Kanade-Lucas-Tomasi Feature Tracker" << endl
         << "  to select corrosponding points." << endl
         << endl
         << "  currently assumes single row panos, where the images have been taken from" << endl
         << "  left to right" << endl
         << endl
         << "Usage: " << name  << " [options] pano.pto image1 image2 image3 ..." << endl
         << endl
         << "       all images must be of the same size!" << endl
         << endl
         << "  [options] can be: " << endl
         << "     -n number   # number of features to track, default: 100" << endl
         << "     -o number   # overlap between image in percent, default: 50." << endl
         << "                   doesn't need to be precise, 10 or 20% higher than" << endl
         << "                   the actual overlap might even work better" << endl
         << "     -v number   # HFOV of images, in degrees" << endl
         << "                   default: 40" << endl
         << "     -s number   # feature tracker window width and height, default: sqrt(w_overlap*h*/2500)" << endl
         << "                   increase if not enough points are tracked. " << endl
         << "     -c          # closed panorama. tries to match the first and the last image." << endl;
//         << "     -f          # do an additional correlation pass on the detected features, default: no" << endl;
}

void loadAndAddImage(vigra::BImage & img, const std::string & filename, Panorama & pano, double defaultHFOV, bool forcedHFOV)
{
    // load image
    ImageImportInfo info(filename.c_str());
    // FIXME.. check for grayscale / color
    img.resize(info.width(), info.height());
    if(info.isGrayscale())
    {
      // import the image just read
      importImage(info, destImage(img));
    } else {
      // convert to greyscale
      BRGBImage timg(info.width(), info.height());
      importImage(info, destImage(timg));
      vigra::copyImage(timg.upperLeft(),
                       timg.lowerRight(),
                       RGBToGrayAccessor<RGBValue<unsigned char> >(),
                       img.upperLeft(),
                       BImage::Accessor());
    }

    Lens lens;
    map_get(lens.variables,"v").setValue(defaultHFOV);
    lens.isLandscape = (img.width() > img.height());
    if (lens.isLandscape) {
                    lens.setRatio(((double)img.width())/img.height());
                } else {
                    lens.setRatio(((double)img.height())/img.width());
                }

    std::string::size_type idx = filename.rfind('.');
    if (idx == std::string::npos) {
      DEBUG_DEBUG("could not find extension in filename");
    }
    std::string ext = filename.substr( idx+1 );

    if (! forcedHFOV && utils::tolower(ext) == "jpg") {
        // try to read exif data from jpeg files.
        lens.readEXIF(filename);
    }

    int matchingLensNr=-1;
    // FIXME: check if the exif information
    // indicates other camera parameters
    for (unsigned int lnr=0; lnr < pano.getNrOfLenses(); lnr++) {
        const Lens & l = pano.getLens(lnr);

        // use a lens if hfov and ratio are the same
        // should add a check for exif camera information as
        // well.
        if ((l.getRatio() == lens.getRatio()) &&
            (l.isLandscape == lens.isLandscape) &&
            (const_map_get(l.variables,"v").getValue() == const_map_get(lens.variables,"v").getValue()) )
        {
            matchingLensNr= lnr;
        }
    }

    if (matchingLensNr == -1) {
        matchingLensNr = pano.addLens(lens);
    }

    VariableMap vars;
    fillVariableMap(vars);

    DEBUG_ASSERT(matchingLensNr >= 0);
    PanoImage pimg(filename, img.width(), img.height(), (unsigned int) matchingLensNr);
    pano.addImage(pimg, vars);
}


int main(int argc, char *argv[])
{

    // parse arguments
    const char * optstring = "bckhn:o:v:f:s:";
    int c;

    opterr = 0;

    int nFeatures = 100;
    double overlapFactor = 0.5;
    double defaultHFOV = 40;
	bool forcedHFOV = false;
    int defaultKLTWindowSize = -1;
    bool closedPanorama = false;
    bool doFinetune = false;
	double featherWidth = 10;
	bool correctColor = false;
	bool correctBrightness = false;

    while ((c = getopt (argc, argv, optstring)) != -1)
        switch (c) {
        case 'n':
            nFeatures = atoi(optarg);
            break;
//        case 'f':
//            doFinetune = true;
//            break;
        case 'o':
            overlapFactor = atof(optarg) / 100;
            break;
        case 's':
            defaultKLTWindowSize = atoi(optarg);
            break;
        case 'v':
            defaultHFOV = atof(optarg);
			forcedHFOV = true;
            break;
        case 'f':
            featherWidth = atof(optarg);
            break;
        case 'b':
            correctBrightness = true;
            break;
        case 'k':
            correctColor = true;
            break;
        case 'c':
            closedPanorama = true;
            break;
      case '?':
      case 'h':
          usage(argv[0]);
          return 1;
      default:
          abort ();
      }

    if (argc - optind <3) {
        usage(argv[0]);
        return 1;
    }
    // output file
    char * outputFile = argv[optind];

    // input images
    unsigned int nImages = argc-optind-1;
    char **imgNames = argv+optind+1;

    cerr << "Creating " << outputFile << " from " << nImages << " images." << endl
         << " Options: " << endl
         << "  - feature points per overlap: " << nFeatures << endl
         << "  - overlap: " << overlapFactor * 100 << "%" << endl
         << "  - default HFOV: " << defaultHFOV << " (only if images doesn't contain an EXIF header)" << endl;

    // create panorama object
    Panorama pano;
    BImage *firstImg = new BImage();
    BImage *secondImg = new BImage();

    loadAndAddImage(*firstImg, imgNames[0], pano, defaultHFOV, forcedHFOV);
    // update the defaultHFOV with the hfov from the first image, might
    // contain the EXIF HFOV
	if (! forcedHFOV) {
    	defaultHFOV = const_map_get(pano.getImageVariables(0),"v").getValue();
	}

    // assume that the images are taken from left to right

    Diff2D overlap((int)(firstImg->width()*overlapFactor +0.5), firstImg->height());

    // create temporary images for these patches.
    // needed, because KLT needs the images in a big chunk
    BImage firstOverlap(overlap.x, overlap.y);
    BImage secondOverlap(overlap.x, overlap.y);

    // create fftw plans, they are the same for all images
    fftwnd_plan fftw_p, fftw_pinv;
    fftw_p    = fftw2d_create_plan(overlap.x, overlap.y, FFTW_FORWARD, FFTW_ESTIMATE);
    fftw_pinv = fftw2d_create_plan(overlap.x, overlap.y, FFTW_BACKWARD, FFTW_ESTIMATE);

    // setup KLT tracker
    KLT_TrackingContext tc;
    KLT_FeatureList fl;
    KLT_FeatureList flFirst;
    tc = KLTCreateTrackingContext();

    // calculate feature density for perfectly distributed features.
    float featureArea = overlap.x * overlap.y / nFeatures;
    if (defaultKLTWindowSize == -1) {
        defaultKLTWindowSize = (int) (sqrt((overlap.x * overlap.y)/2500.0));
    }

    // make sure that the features are distributed over the whole image
    // by basing the minimum feature distance on the feature area.
    tc->mindist = (int)(sqrt(featureArea) / 2 + 0.5);
    tc->window_width = defaultKLTWindowSize;
    tc->window_height = defaultKLTWindowSize;
    KLTUpdateTCBorder(tc);
    fl = KLTCreateFeatureList(nFeatures);
    // feature list 2, to keep features in first image
    flFirst = KLTCreateFeatureList(nFeatures);

    KLTPrintTrackingContext(tc);


    // select a starting yaw
    double currentImageYaw = - defaultHFOV *  (nImages/2.0) * (1.0-overlapFactor);
    // set yaw
    PT::Variable var("y",currentImageYaw);
    pano.updateVariable(0,var);

    unsigned int nPairs = nImages -1;
    if (closedPanorama) {
        nPairs = nImages;
    }
    for (unsigned int pair=0; pair< nPairs; pair++) {
        loadAndAddImage(*secondImg, imgNames[(pair+1)%nImages], pano, defaultHFOV, forcedHFOV);
        // estimate translation between images, using phase correlation
        // could be optimized by keeping fft of one image

        Diff2D offset = Diff2D(firstImg->width() - overlap.x,
                               firstImg->height() - overlap.y);
        // copy overlapping regions.
        // assume images where taken left -> right
        copyImage(firstImg->lowerRight() - overlap,
                  firstImg->lowerRight(),
                  firstImg->accessor(),
                  firstOverlap.upperLeft(),
                  firstOverlap.accessor());

        copyImage(secondImg->upperLeft(),
                  secondImg->upperLeft() + overlap,
                  secondImg->accessor(),
                  secondOverlap.upperLeft(),
                  secondOverlap.accessor());

        CorrelationResult cres = vigra_ext::phaseCorrelation(srcImageRange(firstOverlap),
                                              srcImageRange(secondOverlap),
                                              fftw_p, fftw_pinv);
        DEBUG_DEBUG("Translation between " << pair << " and " << pair+1 << ": "
                    << cres.maxpos);
        // run KLT on the images
        {
            // HACK, we assume that BImage stores its image in col major
            // format, in a continous chunk of memory
            unsigned char * img1 = firstOverlap.begin();
            unsigned char * img2 = secondOverlap.begin();

            KLTSelectGoodFeatures(tc, img1, firstOverlap.width(),
                                  firstOverlap.height(), fl);
            for (int i = 0 ; i < fl->nFeatures ; i++)  {
                // remember current feature positions
                flFirst->feature[i]->x = fl->feature[i]->x;
                flFirst->feature[i]->y = fl->feature[i]->y;
                flFirst->feature[i]->val = fl->feature[i]->val;
                // add estimation of feature position in next image
                fl->feature[i]->next_x = fl->feature[i]->x - cres.maxpos.x;
                fl->feature[i]->next_y = fl->feature[i]->y - cres.maxpos.y;
            }
//            ostringstream finame;
//            finame << "pair_" << pair << "_0.ppm";
//            KLTWriteFeatureListToPPM(flFirst, img1, firstOverlap.width(), firstOverlap.height(), finame.str().c_str());

            DEBUG_DEBUG("KLT tracking features for image pair " << pair);
            KLTTrackFeatures(tc, img1, img2, firstOverlap.width(), firstOverlap.height(), fl);

//            ostringstream foname;
//            foname << "pair_" << pair << "_1.ppm";
//            KLTWriteFeatureListToPPM(fl, img2, firstOverlap.width(), firstOverlap.height(), foname.str().c_str());

            // check how many features where tracked
            int nTracked = 0;
            for (int i = 0 ; i < fl->nFeatures ; i++)  {
                if (fl->feature[i]->val >=0) {
                    nTracked++;
                }
            }
            int nAdded=0;
            for (int i = 0 ; i < fl->nFeatures ; i++)  {
                if (fl->feature[i]->val >=0) {
                    // correlation based fine tune.
                    CorrelationResult res;
                    res.maxpos.x = fl->feature[i]->x;
                    res.maxpos.y = fl->feature[i]->y;
                    res.maxi = 1;
                    // currently disabled, ther must be an error somewhere
                    if (doFinetune) {
                        res = vigra_ext::PointFineTune(*firstImg,
                                                       Diff2D (roundi(flFirst->feature[i]->x),
                                                               roundi(flFirst->feature[i]->y)),
                                                       11,
                                                       *secondImg,
                                                       Diff2D (roundi(fl->feature[i]->x),
                                                               roundi(fl->feature[i]->y)),
                                                       defaultKLTWindowSize);

                    }
                    // add only if the correlation result was not too bad
                    if (res.maxi > 0.5) {
                        // add point pair
                        ControlPoint cp(pair%nImages,
                                        flFirst->feature[i]->x + offset.x,
                                        flFirst->feature[i]->y + offset.y,
                                        (pair+1)%nImages,
                                        res.maxpos.x,
                                        res.maxpos.y);
                        pano.addCtrlPoint(cp);
                        nAdded++;
                    }
                }
            }
            DEBUG_NOTICE("image pair " << pair << ": point tracked: " << nTracked
                         << " good points: " << nAdded);
            if (nAdded < nFeatures/5) {
                DEBUG_ERROR("image pair " << pair << ": only " << nAdded
                            << " features added. Seems like the phase correlation estimate was not true, or the projective distortion is too high");
            }
            // add control point pairs.

            // set yaw estimate.
            var.setValue(var.getValue() + (cres.maxpos.x+firstImg->width() - overlap.x)/firstImg->width() * defaultHFOV);
            pano.updateVariable((pair+1)%nImages,var);
        }

        delete firstImg;
        firstImg = secondImg;
        secondImg = new BImage();
    }
    fftwnd_destroy_plan(fftw_p);
    fftwnd_destroy_plan(fftw_pinv);

    ofstream of(outputFile);
    // try setting sensible options
    PanoramaOptions opts = pano.getOptions();
    FDiff2D fov = pano.calcFOV();
    opts.HFOV = fov.x;
    opts.VFOV = fov.y;
    if (opts.HFOV < 80 && opts.VFOV < 80) {
        opts.projectionFormat = PanoramaOptions::RECTILINEAR;
    } else {
        opts.projectionFormat = PanoramaOptions::EQUIRECTANGULAR;
    }

    opts.optimizeReferenceImage = nImages/2;
    OptimizeVector optvec(nImages);
    // fill optimize vector, just anchor one image.
    for (unsigned int i=0; i<nImages; i++) {
        if (i != opts.optimizeReferenceImage) {
            optvec[i].insert("y");
            optvec[i].insert("p");
            optvec[i].insert("r");
        }
    }
	
	// calc optimal width for output image
    int nImgs = pano.getNrOfImages();
    double pixelDensity=0;
    for (int i=0; i<nImgs; i++) {
        const PanoImage & img = pano.getImage(i);
        const VariableMap & var = pano.getImageVariables(i);
        double density = img.getWidth() / const_map_get(var,"v").getValue();
        if (density > pixelDensity) {
            pixelDensity = density;
        }
    }
    opts.width = (int) (pixelDensity * opts.HFOV);
	
	opts.featherWidth = (int) featherWidth;
	
	if (correctColor && correctBrightness) {
		opts.colorCorrection = PanoramaOptions::BRIGHTNESS_COLOR;
	} else if (correctColor) {
		opts.colorCorrection = PanoramaOptions::COLOR;
	} else if (correctBrightness) {
		opts.colorCorrection = PanoramaOptions::BRIGHTNESS;
	}
	opts.colorReferenceImage = opts.optimizeReferenceImage;
	
    pano.printOptimizerScript(of, optvec, opts);
}
