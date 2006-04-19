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

#include <config.h>
#include <vigra/windows.h>
#include <vigra/stdimage.hxx>

#include <PT/Stitcher.h>

#include <PT/RemappedPanoImage.h>

using namespace std;
using namespace vigra;
using namespace vigra_ext;
using namespace PT;


/** determine blending order (starting with image 0), and continue to
 *  stitch the image with the biggest overlap area with the real image..
 *  do everything on a low res version of the masks
 */
void PT::estimateBlendingOrder(const Panorama & pano, UIntSet images, vector<unsigned int> & blendOrder)
{
    unsigned int nImg = images.size();
    DEBUG_ASSERT(nImg > 0);

    typedef PT::RemappedPanoImage<vigra::BRGBImage, vigra::BImage> RPImg;

    PanoramaOptions opts = pano.getOptions();
    // small area, for alpha mask overlap analysis.
    opts.setWidth(400);
    Size2D size(opts.getWidth(), opts.getHeight());
    Rect2D completeAlphaROI(size);
    // find intersecting regions, on a small version of the panorama.
    std::map<unsigned int, PT::RemappedPanoImage<vigra::BRGBImage, vigra::BImage> * > rimg;
	std::map<unsigned int, PT::RemappedPanoImage<vigra::BRGBImage, vigra::BImage> * >::iterator rimgIter;

    BImage alpha(size);
    Rect2D alphaROI;

    for (UIntSet::iterator it = images.begin(); it != images.end(); ++it)
    {
        // calculate alpha channel
        rimg[*it] = new RPImg;
        rimg[*it]->setPanoImage(pano.getSrcImage(*it), opts);
        rimg[*it]->calcAlpha();
#ifdef DEBUG
//	vigra::exportImage(rimg[*it].alpha(), vigra::ImageExportInfo("debug_alpha.tif"));
#endif
    }

    int firstImg = *(images.begin());
    // copy first alpha channel
    alphaROI = rimg[firstImg]->boundingBox();
    // restrict to output pano size
    alphaROI = alphaROI & completeAlphaROI;
    DEBUG_DEBUG("alphaROI: " << alphaROI);
    DEBUG_DEBUG("alpha size: " << alpha.size());
    copyImage(applyRect(alphaROI, vigra_ext::srcMaskRange(*(rimg[firstImg]))),
//    copyImage(vigra_ext::srcMaskRange(rimg[firstImg]),
              applyRect(alphaROI, destImage(alpha)));

    Rect2D overlap;
    // intersect ROI's & masks of all images
    while (images.size() > 0) {
	unsigned int maxSize = 0;
	unsigned int choosenImg = *(images.begin());
	// search for maximum overlap
	for (UIntSet::iterator it = images.begin(); it != images.end(); ++it) {
	    // check for possible overlap
	    DEBUG_DEBUG("examing overlap with image " << *it);
	    // overlapping images..
	    overlap = alphaROI & rimg[*it]->boundingBox();
	    if (!overlap.isEmpty()) {
	      DEBUG_DEBUG("ROI intersects: " << overlap.upperLeft()
                          << " to " << overlap.lowerRight());
		// if the overlap ROI is smaller than the current maximum,
		// ignore.
	        if (overlap.area() > (int) maxSize) {
		    OverlapSizeCounter counter;
                    inspectTwoImages(applyRect(overlap, srcMaskRange(*(rimg[*it]))),
				     applyRect(overlap, srcImage(alpha)),
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
	alphaROI = alphaROI | rimg[choosenImg]->boundingBox();
        alphaROI = alphaROI & completeAlphaROI;
    }
	// All images have been erased so just iterate through the map
    for (rimgIter = rimg.begin(); rimgIter != rimg.end(); rimgIter++) {
        delete rimgIter->second;
    }
}

/** The main stitching function.
 *  This function delegates all the work to the other functions
 *
 *  Due to the compile memory requirements of the instantiated templates
 *  ( > 1GB, for all pixel types), the instatiations are divided into 4 separate
 *  cpp files
 */
void PT::stitchPanorama(const PT::Panorama & pano,
                        const PT::PanoramaOptions & opts,
                        utils::MultiProgressDisplay & progress,
                        const std::string & basename,
                        const PT::UIntSet & usedImgs)
{
    // probe the first image to determine a suitable image type for stitching
    DEBUG_ASSERT(pano.getNrOfImages() > 0);

    unsigned int imgNr = 0;
    pano.getImage(imgNr);
    string fname =  pano.getImage(imgNr).getFilename().c_str();
    DEBUG_DEBUG("Probing image: " << fname);
    vigra::ImageImportInfo info(fname.c_str());
    const char * pixelType = info.getPixelType();
    int bands = info.numBands();
    int extraBands = info.numExtraBands();

    // check if all other images have the same type
    for (imgNr = 1 ; imgNr < pano.getNrOfImages(); imgNr++) {
        vigra::ImageImportInfo info2(pano.getImage(imgNr).getFilename().c_str());
        if (strcmp(info2.getPixelType(), pixelType) != 0) {
            UTILS_THROW(std::runtime_error, "image " << pano.getImage(imgNr).getFilename()
                        << " uses " << info2.getPixelType() << " valued pixel, while " << pano.getImage(0).getFilename() << " uses: " << pixelType);
            return;
        }

        if (info2.numBands() != bands) {
            UTILS_THROW(std::runtime_error, "image " << pano.getImage(imgNr).getFilename()
                        << " has " << info2.numBands() << " channels, while " << pano.getImage(0).getFilename() << " uses: " << bands);
            return;
        }
    }

    // stitch the pano with a suitable image type
    if (bands == 1 || bands == 2 && extraBands == 1) {
        if (strcmp(pixelType, "UINT8") == 0 ||
            strcmp(pixelType, "INT16") == 0 ||
            strcmp(pixelType, "UINT16") == 0 ) 
        {
            stitchPanoGray_8_16(pano, opts, progress, basename, usedImgs, pixelType);
        } else {
            stitchPanoGray_32_float(pano, opts, progress, basename, usedImgs, pixelType);
        }
    } else if (bands == 3 || bands == 4 && extraBands == 1) {
        if (strcmp(pixelType, "UINT8") == 0 ||
            strcmp(pixelType, "INT16") == 0 ||
            strcmp(pixelType, "UINT16") == 0 ) 
        {
            stitchPanoRGB_8_16(pano, opts, progress, basename, usedImgs, pixelType);
        } else {
            stitchPanoRGB_32_float(pano, opts, progress, basename, usedImgs, pixelType);
        }
    } else {
        DEBUG_ERROR("unsupported depth, only images with 1 and 3 channel images are supported");
        throw std::runtime_error("unsupported depth, only images with 1 and 3 channel images are supported");
        return;
    }
}


