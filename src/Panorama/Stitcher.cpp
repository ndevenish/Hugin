// -*- c-basic-offset: 4 -*-
/** @file Stitcher.cpp
 *
 *  Contains various routines used for stitching panoramas.
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
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

#include <PT/Stitcher.h>

using namespace std;
using namespace vigra;
using namespace vigra_ext;
using namespace PT;


template<typename ImageType, typename AlphaType>
static void stitchPanoIntern(const PT::Panorama & pano,
                          const PT::PanoramaOptions & opts,
                          utils::MultiProgressDisplay & progress,
                          const std::string & basename)
{
    //    typedef
    //        vigra::NumericTraits<typename OutputImageType::Accessor::value_type> DestTraits;

    UIntSet imgs;
    for (unsigned int i=0; i< pano.getNrOfImages(); i++) {
	imgs.insert(i);
    }

    // determine stitching output
    switch (opts.outputFormat) {
    case PT::PanoramaOptions::JPEG:
    case PT::PanoramaOptions::PNG:
    case PT::PanoramaOptions::TIFF:
	{
	    WeightedStitcher<ImageType, AlphaType> stitcher(pano, progress);
	    stitcher.stitch(opts, imgs, basename);
	    break;
	}
    case PT::PanoramaOptions::TIFF_m:
        {
	    MultiImageRemapper<ImageType, AlphaType> stitcher(pano, progress);
	    stitcher.stitch(opts, imgs, basename);
	    break;
        }
    case PT::PanoramaOptions::TIFF_multilayer:
	{
	    TiffMultiLayerRemapper<ImageType, AlphaType> stitcher(pano, progress);
	    stitcher.stitch(opts, imgs, basename);
	    break;
	}
    case PT::PanoramaOptions::TIFF_mask:
    case PT::PanoramaOptions::TIFF_multilayer_mask:
	DEBUG_DEBUG("multi mask stitching not implemented!");
	break;
    default:
	DEBUG_ERROR("output format " << opts.getFormatName(opts.outputFormat) << "not supported");
	break;
    }
}


/** determine blending order (starting with image 0), and continue to
 *  stitch the image with the biggest overlap area with the real image..
 *  do everything on a low res version of the masks
 */
void PT::estimateBlendingOrder(const Panorama & pano, UIntSet images, vector<unsigned int> & blendOrder)
{
    unsigned int nImg = images.size();
    DEBUG_ASSERT(nImg > 0);

    PanoramaOptions opts = pano.getOptions();
    // small area, for alpha mask overlap analysis.
    opts.width = 400;
    Diff2D size(opts.width, opts.getHeight());
    // find intersecting regions, on a small version of the panorama.
    std::map<unsigned int, PT::RemappedPanoImage<vigra::BRGBImage, vigra::BImage> > rimg;

    BImage alpha(size);
    ROI<Diff2D> alphaROI;

    for (UIntSet::iterator it = images.begin(); it != images.end(); ++it)
    {
        // calculate alpha channel
        rimg[*it].calcAlpha(pano, opts, *it);
	vigra::exportImage(rimg[*it].alpha(), vigra::ImageExportInfo("debug_alpha.tif"));
    }

    int firstImg = *(images.begin());
    // copy first alpha channel
    copyImage(rimg[firstImg].alpha(), rimg[firstImg].roi().apply(destImage(alpha)));
    alphaROI = rimg[firstImg].roi();

    ROI<Diff2D> overlap;
    // intersect ROI's & masks of all images
    while (images.size() > 0) {
	unsigned int maxSize = 0;
	unsigned int choosenImg = *(images.begin());
	// search for maximum overlap
	for (UIntSet::iterator it = images.begin(); it != images.end(); ++it) {
	    // check for possible overlap
	    DEBUG_DEBUG("examing overlap with image " << *it);
	    if (alphaROI.intersect(rimg[*it].roi(), overlap)) {
	      DEBUG_DEBUG("ROI intersects: " << overlap.getUL() << " to " << overlap.getLR());
		// if the overlap ROI is smaller than the current maximum,
		// ignore.
	        if (overlap.size().x * overlap.size().y > (int) maxSize) {
		    OverlapSizeCounter counter;
		    inspectTwoImages(overlap.apply(rimg[*it].alpha(),
						   rimg[*it].roi()),
				     overlap.apply(srcImage(alpha)),
				     counter);
		    DEBUG_DEBUG("overlap size in pixel: " << counter.getSize());
		    if (counter.getSize() > maxSize) {
			choosenImg = *it;
			maxSize = counter.getSize();
		    }
		}
	    }
        }
	// add to the blend list
	blendOrder.push_back(choosenImg);
	images.erase(choosenImg);
	// update alphaROI, to new roi.
	rimg[choosenImg].roi().unite(alphaROI, alphaROI);
    }
}

/** The main stitching function.
 *  This function delegates all the work to the other functions
 */
void PT::stitchPanorama(const PT::Panorama & pano,
                        const PT::PanoramaOptions & opts,
			utils::MultiProgressDisplay & progress,
			const std::string & basename)
{
    // probe the first image to determine a suitable image type for stitching
    DEBUG_ASSERT(pano.getNrOfImages() > 0);

    unsigned int imgNr = 0;
    pano.getImage(imgNr);
    vigra::ImageImportInfo info(pano.getImage(imgNr).getFilename().c_str());
    const char * pixelType = info.getPixelType();
    int bands = info.numBands();

    // check if all other images have the same type
    for (imgNr = 1 ; imgNr < pano.getNrOfImages(); imgNr++) {
        vigra::ImageImportInfo info2(pano.getImage(imgNr).getFilename().c_str());
        if (strcmp(info2.getPixelType(), pixelType) != 0) {
            DEBUG_FATAL("image " << pano.getImage(imgNr).getFilename()
                        << " uses " << info2.getPixelType() << " valued pixel, while " << pano.getImage(0).getFilename() << " uses: " << pixelType);
            return;
        }

        if (info2.numBands() != bands) {
            DEBUG_FATAL("image " << pano.getImage(imgNr).getFilename()
                        << " has " << info2.numBands() << " channels, while " << pano.getImage(0).getFilename() << " uses: " << bands);
            return;
        }
    }

    // FIXME add alpha channel treatment!

    // stitch the pano with a suitable image type
    if (bands == 1) {
        if (strcmp(pixelType, "UINT8") == 0 ) {
            stitchPanoIntern<BImage,BImage>(pano, opts, progress, basename);
        } else if (strcmp(pixelType, "INT16") == 0 ) {
            stitchPanoIntern<SImage,BImage>(pano, opts, progress, basename);
        } else if (strcmp(pixelType, "UINT16") == 0 ) {
            stitchPanoIntern<USImage,BImage>(pano, opts, progress, basename);
        } else if (strcmp(pixelType, "UINT32") == 0 ) {
            stitchPanoIntern<UIImage,BImage>(pano, opts, progress, basename);
        } else if (strcmp(pixelType, "INT32") == 0 ) {
            stitchPanoIntern<IImage,BImage>(pano, opts, progress, basename);
        } else if (strcmp(pixelType, "FLOAT") == 0 ) {
            stitchPanoIntern<FImage,BImage>(pano, opts, progress, basename);
        } else if (strcmp(pixelType, "DOUBLE") == 0 ) {
            stitchPanoIntern<DImage,BImage>(pano, opts, progress, basename);
        } else {
            DEBUG_FATAL("Unsupported pixel type: " << pixelType);
            return;
        }
    } else if (bands == 3) {
        if (strcmp(pixelType, "UINT8") == 0 ) {
            stitchPanoIntern<BRGBImage,BImage>(pano, opts, progress, basename);
        } else if (strcmp(pixelType, "INT16") == 0 ) {
            stitchPanoIntern<SRGBImage,BImage>(pano, opts, progress, basename);
        } else if (strcmp(pixelType, "UINT16") == 0 ) {
            stitchPanoIntern<USRGBImage, BImage>(pano, opts, progress, basename);
        } else if (strcmp(pixelType, "INT32") == 0 ) {
            stitchPanoIntern<IRGBImage,BImage>(pano, opts, progress, basename);
        } else if (strcmp(pixelType, "UINT32") == 0 ) {
            stitchPanoIntern<UIRGBImage,BImage>(pano, opts, progress, basename);
        } else if (strcmp(pixelType, "FLOAT") == 0 ) {
            stitchPanoIntern<FRGBImage,BImage>(pano, opts, progress, basename);
        } else if (strcmp(pixelType, "DOUBLE") == 0 ) {
            stitchPanoIntern<DRGBImage,BImage>(pano, opts, progress, basename);
        } else {
            DEBUG_FATAL("Unsupported pixel type: " << pixelType);
            return;
        }
    } else {
        DEBUG_FATAL("unsupported depth (images with alpha channel not yet supported)");
        return;
    }
}


