// -*- c-basic-offset: 4 -*-
/** @file simplestitcher.h
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

#ifndef _SIMPLESTITCHER_H
#define _SIMPLESTITCHER_H

#include <vigra/impex.hxx>

#include <PT/PanoToolsInterface.h>
#include <PT/tiffUtils.h>

namespace PTools {

/** stitch a panorama
 *
 * @todo more advanced seam calculation?
 * @todo move seam calculation into a separate class/function?
 * @todo usage of different iterpolators
 * @todo vignetting correction
 * @todo do not keep all images in memory, if short on mem.
 * @todo proper handling for 16 bit images etc.
 *
 */
template <class DestImageType>
void stitchPanoramaSimple(const PT::Panorama & pano,
                          const PT::PanoramaOptions & opts,
                          DestImageType & dest,
                          utils::ProgressDisplay & progress,
                          const std::string & basename,
                          const std::string & format = "tif",
                          bool savePartial = false)
{
    // until we have something better...
    typedef DestImageType InputImageType;
    typedef DestImageType OutputImageType;

    // resize dest to output panorama size
    dest.resize(opts.width, opts.getHeight());

    // catch the errors that can be thrown by vigra.. easier to debug then...

    // remapped panorama images
    std::vector<RemappedPanoImage<OutputImageType, vigra::FImage> > remapped(pano.getNrOfImages());

        // remap all images (save to disk and keep in memory...)
    unsigned int nImg = pano.getNrOfImages();
    for (unsigned int i=0; i< nImg; i++) {
        // load image
        const PT::PanoImage & img = pano.getImage(i);
        vigra::ImageImportInfo info(img.getFilename().c_str());
        // create a RGB image of appropriate size
        // FIXME.. use some other mechanism to define what format to use..
        InputImageType srcImg(info.width(), info.height());
        // import the image just read
        progress.progressMessage("loading image " + img.getFilename());
        importImage(info, destImage(srcImg));

        progress.progressMessage("remapping " + img.getFilename());

        // this should be made a bit smarter, but I don't
        // want to have virtual function call for the interpolator
        switch (opts.interpolator) {
        case PT::PanoramaOptions::CUBIC:
            DEBUG_DEBUG("using cubic interpolator")
            PTools::remapImage(pano, i,
                               srcImageRange(srcImg),
                               opts,
                               remapped[i],
                               interp_cubic());
            break;
        case PT::PanoramaOptions::SPLINE_16:
            DEBUG_DEBUG("interpolator: spline16")
            PTools::remapImage(pano, i,
                               srcImageRange(srcImg),
                               opts,
                               remapped[i],
                               interp_spline16());
            break;
        case PT::PanoramaOptions::SPLINE_36:
            DEBUG_DEBUG("interpolator: spline36")
            PTools::remapImage(pano, i,
                               srcImageRange(srcImg),
                               opts,
                               remapped[i],
                               interp_spline36());
            break;
        case PT::PanoramaOptions::SPLINE_64:
            DEBUG_DEBUG("interpolator: spline64")
            PTools::remapImage(pano, i,
                               srcImageRange(srcImg),
                               opts,
                               remapped[i],
                               interp_spline64());
            break;
        case PT::PanoramaOptions::SINC_256:
            DEBUG_DEBUG("interpolator: sinc 256")
            PTools::remapImage(pano, i,
                               srcImageRange(srcImg),
                               opts,
                               remapped[i],
                               interp_sinc<8>());
            break;
        case PT::PanoramaOptions::BILINEAR:
            PTools::remapImage(pano, i,
                               srcImageRange(srcImg),
                               opts,
                               remapped[i],
                               interp_bilin());
            break;
        case PT::PanoramaOptions::NEAREST_NEIGHBOUR:
            PTools::remapImage(pano, i,
                               srcImageRange(srcImg),
                               opts,
                               remapped[i],
                               interp_nearest());
            break;
        case PT::PanoramaOptions::SINC_1024:
            PTools::remapImage(pano, i,
                               srcImageRange(srcImg),
                               opts,
                               remapped[i],
                               interp_sinc<32>());
            break;
        }

        // test
        if ( savePartial) {
            // write out the destination images
            std::ostringstream ofname;
            ofname << basename << "_" << i << ".tif";
            exportImage(srcImageRange(remapped[i].image), vigra::ImageExportInfo(ofname.str().c_str()));
            std::ostringstream ofdistname;
            ofdistname << basename << "_dist_" << i << ".tif";
            exportImage(srcImageRange(remapped[i].dist), vigra::ImageExportInfo(ofdistname.str().c_str()));
        }
    }

    DEBUG_DEBUG("merging images");
    // stitch images
    progress.progressMessage("merging images");

    // save individual images into a single big multi-image tif
    if (opts.outputFormat == PT::PanoramaOptions::TIFF_m) {
        std::string filename = basename + ".tif";
        DEBUG_DEBUG("flattening image into a multi image tif file " << filename);
        vigra::TiffImage * tiff = TIFFOpen(filename.c_str(), "w");
        DEBUG_ASSERT(tiff);

        // loop over all images and create alpha channel for it,
        // and write it into the output file.
        for (unsigned int imgNr=0; imgNr< nImg; imgNr++) {
            vigra::Diff2D sz = remapped[imgNr].image.size();
            vigra::BImage alpha(sz);

            // over whole image
            int xstart = remapped[imgNr].ul.x;
            int xend = remapped[imgNr].ul.x + sz.x;
            int ystart = remapped[imgNr].ul.y;
            int yend = remapped[imgNr].ul.y + sz.y;

            vigra::BImage::Iterator ya(alpha.upperLeft());
            for(int y=ystart; y < yend; ++y, ya.y++) {
                // create x iterators
                typename vigra::BImage::Iterator xa(ya);
                for(int x=xstart; x < xend; ++x, ++xa.x) {
                    // find the image where this pixel is closest to the image center
                    float minDist = FLT_MAX;
                    unsigned int minImgNr = 0;
                    vigra::Diff2D cp(x,y);
                    for (unsigned int i=0; i< nImg; i++) {
                        float dist = remapped[imgNr].getDistanceFromCenter(cp);
                        if ( dist < minDist ) {
                            minDist = dist;
                            minImgNr = i;
                        }
                    }
                    // if a minimum was found in current image, use this pixel
                    if (minDist < FLT_MAX && minImgNr == imgNr) {
                        // set value in alpha channel
                        *xa = 255;
                    }
                }
            }
            
            // write the alpha intoa a debug file
            {
                std::ostringstream ofname;
                ofname << basename << "_alpha_" << imgNr << ".tif";
                vigra::exportImage(srcImageRange(alpha), vigra::ImageExportInfo(ofname.str().c_str()));
            }

            // create a new directory for our image
            // hopefully I didn't forget too much stuff..
            TIFFCreateDirectory (tiff);

            // set page
            TIFFSetField (tiff, TIFFTAG_SUBFILETYPE, FILETYPE_PAGE);
            TIFFSetField (tiff, TIFFTAG_PAGENUMBER, (unsigned short)imgNr, (unsigned short)nImg);
            // set offset
            TIFFSetField (tiff, TIFFTAG_XRESOLUTION, (float) 72.0f);
            TIFFSetField (tiff, TIFFTAG_YRESOLUTION, (float) 72.0f);
            DEBUG_DEBUG("offset: " << xstart << "," << ystart);
            TIFFSetField (tiff, TIFFTAG_XPOSITION, (float) xstart/72.0f);
            TIFFSetField (tiff, TIFFTAG_YPOSITION, (float) ystart/72.0f);

            // save input name.
            TIFFSetField (tiff, TIFFTAG_DOCUMENTNAME, basename.c_str());
            TIFFSetField (tiff, TIFFTAG_PAGENAME, pano.getImage(imgNr).getFilename().c_str() );
            //
            TIFFSetField (tiff, TIFFTAG_IMAGEDESCRIPTION, "created with nona, see http://hugin.sf.net");

            // call vigra function to write the image data
            vigra::createBRGBATiffImage(remapped[imgNr].image.upperLeft(),
                                        remapped[imgNr].image.lowerRight(),
                                        remapped[imgNr].image.accessor(),
                                        alpha.upperLeft(), alpha.accessor(),
                                        tiff);
            // write this image
            TIFFWriteDirectory (tiff);
            TIFFFlushData (tiff);

        }
        TIFFClose(tiff);

    } else {
        // flatten image into a panorama image
        DEBUG_DEBUG("flattening image");
        typename OutputImageType::Accessor oac(dest.accessor());

            // over whole image
        int xstart = 0;
        int xend = dest.width();
        int ystart = 0;
        int yend = dest.height();


        // loop over whole image
        typename DestImageType::Iterator yd(dest.upperLeft());
        for(int y=ystart; y < yend; ++y, ++yd.y)
        {
            // create x iterators
            typename DestImageType::Iterator xd(yd);
            for(int x=xstart; x < xend; ++x, ++xd.x)
            {
                // find the image where this pixel is closest to the image center
                float minDist = FLT_MAX;
                unsigned int minImgNr = 0;
                vigra::Diff2D cp(x,y);
                for (unsigned int i=0; i< nImg; i++) {
                    float dist = remapped[i].getDistanceFromCenter(cp);
                    if ( dist < minDist ) {
                        minDist = dist;
                        minImgNr = i;
                    }
                }
                // if a minimum was found, set output pixel
                if (minDist < FLT_MAX) {
                    *xd = remapped[minImgNr].get(cp);
                }
            }
        }

        // save final panorama
        std::string filename = basename + "." + format;
	DEBUG_DEBUG("saving output file: " << filename);
        vigra::ImageExportInfo exinfo(filename.c_str());
        if (format == "jpg") {
            std::ostringstream jpgqual;
            jpgqual << opts.quality;
            exinfo.setCompression(jpgqual.str().c_str());
        }
        exportImage(srcImageRange(dest), exinfo);
    }

}

} // namespace PTools

#endif // _SIMPLESTITCHER_H
