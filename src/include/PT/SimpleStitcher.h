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


#include <PT/PanoToolsInterface.h>
#include <vigra/impex.hxx>

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
                          std::string basename = "")
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
        importImage(info, destImage(srcImg));

        PTools::remapImage(pano, i,
                           srcImageRange(srcImg),
                           opts,
                           remapped[i]);

        if ( basename != "") {
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

    OutputImageType panoImg(opts.width, opts.getHeight());

    typename OutputImageType::Accessor oac(panoImg.accessor());

    // over whole image
    int xstart = 0;
    int xend = panoImg.width() -1;
    int ystart = 0;
    int yend = panoImg.height() -1;

    // create dest y iterator
    typename DestImageType::Iterator yd(dest.upperLeft());

    // loop over whole image
    for(int y=ystart; y < yend; ++y, ++yd.y)
    {
        // create x iterators
        typename DestImageType::Iterator xd(yd);
        for(int x=xstart; x < xend; ++x, ++xd.x)
        {
            // find the image where this pixel is closes to the image center
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
//                *xd = vigra::RGBValue<unsigned char>(minImgNr, x/10,y/10);
            } else {
                *xd = vigra::RGBValue<unsigned char>(255,255,255);
            }
        }
    }
    DEBUG_DEBUG("merge finished");


}

} // namespace PTools

#endif // _SIMPLESTITCHER_H
