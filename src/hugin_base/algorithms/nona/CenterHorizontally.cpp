// -*- c-basic-offset: 4 -*-
/** @file CenterHorizontally.cpp
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: Panorama.h 1947 2007-04-15 20:46:00Z dangelo $
 *
 * !! from Panorama.h 1947 
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

#include "CenterHorizontally.h"

#include <vigra/impex.hxx>
#include <panodata/PanoramaData.h>
#include <nona/RemappedPanoImage.h>



namespace HuginBase {


void CenterHorizontally::centerHorizontically(PanoramaData& panorama)
{
    vigra::Size2D panoSize(360,180);
    
    // remap into minature pano.
    PanoramaOptions opts;
    opts.setHFOV(360);
    opts.setProjection(PanoramaOptions::EQUIRECTANGULAR);
    opts.setWidth(360);
    opts.setHeight(180);
    
    // remap image
    vigra::BImage panoAlpha(panoSize);
    Nona::RemappedPanoImage<vigra::BImage, vigra::BImage> remapped;
    
    // use selected images.
    UIntSet allActiveImgs = panorama.getActiveImages();

    if (allActiveImgs.size() == 0) {
        // do nothing if there are no images
        return;
    }
    
    //only check unlinked images
    UIntSet activeImgs;
    for (UIntSet::const_iterator it = allActiveImgs.begin(); it!= allActiveImgs.end(); it++)
    {
        const SrcPanoImage & img=panorama.getImage(*it);
        bool consider=true;
        if(img.YawisLinked())
        {
            for(UIntSet::const_iterator it2=activeImgs.begin(); it2!=activeImgs.end(); it2++)
            {
                if(img.YawisLinkedWith(panorama.getSrcImage(*it2)))
                {
                    consider=false;
                    break;
                };
            };
        };
        if(img.getX()!=0 || img.getY()!=0 || img.getZ()!=0)
        {
            //if translation parameters are non-zero break, 
            //because centering by modifying yaw does not work in this case
            return;
        };
        if(consider)
            activeImgs.insert(*it);
    };

    for (UIntSet::iterator it = activeImgs.begin(); it != activeImgs.end(); ++it) {
        //    for (unsigned int imgNr=0; imgNr < getNrOfImages(); imgNr++) {
        //        const PanoImage & img = getImage(*it);
        //        Size2D sz(img.getWidth(), img.getHeight());
        //        remapped.setPanoImage(*this, *it, sz, opts);
        remapped.setPanoImage(panorama.getSrcImage(*it), opts, vigra::Rect2D(0,0,360,180));
        // calculate alpha channel
        remapped.calcAlpha();
        // copy into global alpha channel.
        vigra::copyImageIf(vigra_ext::applyRect(remapped.boundingBox(),
                                                vigra_ext::srcMaskRange(remapped)),
                           vigra_ext::applyRect(remapped.boundingBox(),
                                                vigra_ext::srcMask(remapped)),
                           vigra_ext::applyRect(remapped.boundingBox(),
                                                destImage(panoAlpha)));
        }
    //    vigra::ImageExportInfo imge("c:/hugin_calcfov_alpha.png");
    //    exportImage(vigra::srcImageRange(panoAlpha), imge);
    
    // get field of view
    std::vector<int> borders;
    bool colOccupied = false;
    for (int h=0; h < 360; h++) {
        bool curColOccupied = false;
        for (int v=0; v< 180; v++) {
            if (panoAlpha(h,v)) {
                // pixel is valid
                curColOccupied = true;
            }
        }
        if (colOccupied && (! curColOccupied) ||
            (!colOccupied) && curColOccupied )
        {
            // change in position, save point.
            borders.push_back(h-180);
            colOccupied = curColOccupied;
        }
    }
    
    
    int lastidx = borders.size() -1;
    if (lastidx == -1) {
        // empty pano
        return;
    }
    
    if (colOccupied) {
        // we have reached the right border, and the pano is still valid
        // shift right fragments by 360 deg
        // |11    2222|  -> |      222211     |
        std::vector<int> newBorders;
        newBorders.push_back(borders[lastidx]);
        for (int i = 0; i < lastidx; i++) {
            newBorders.push_back(borders[i]+360);
        }
        borders = newBorders;
    }
    
    double dYaw=(borders[0] + borders[lastidx])/2;
    
    // apply yaw shift
    unsigned int nImg = panorama.getNrOfImages();
    for (unsigned int i=0; i < nImg; i++) {
        const SrcPanoImage & image = panorama.getImage(i);
        // only adjust yaw if we haven't already done so because of linking.
        bool update=true;
        if(image.YawisLinked())
        {
            for(unsigned int j=0; j<i; j++)
            {
                if(image.YawisLinkedWith(panorama.getImage(j)))
                {
                    update=false;
                    break;
                };
            };
        };
        if (update)
        {
            double yaw = image.getYaw();
            yaw = yaw - dYaw;
            while (yaw < 180) {
                yaw += 360;
            }
            while (yaw > 180) {
                yaw -= 360;
            }
            SrcPanoImage newImage = image;
            newImage.setYaw(yaw);
            panorama.setImage(i, newImage);
            panorama.imageChanged(i);
        }
    }
}

} ///namespace
