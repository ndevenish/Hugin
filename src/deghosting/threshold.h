
/**
 * Advanced threshold
 * Copyright (C) 2009  Lukáš Jirkovský <l.jirkovsky@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 *Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <vector>

#include <vigra/stdimage.hxx>
#include <vigra/transformimage.hxx>

#include "deghosting.h"

const uint16_t ONE_UNMASKED       = 0;
const uint16_t THRESHOLD_DONTCARE = 1;

/** Threshold function
 * used for creating alpha masks for images
 * @param const vector<FImagePtr> vector of images
 * @param const int threshold all pixels above this thresshold are set to 255, others to 0
 * @param const uint16_t flags flags for setting the behavior
 *          possible values are:
 *              THRESHOLD_DONTCARE – applies only simple threshold
 *              ONE_UNMASKED – if pixel should be black in all images after applying threshold
 *                             leave it in one image (where the pixel value is highest) white, default
 */
std::vector<deghosting::BImagePtr> threshold(const std::vector<deghosting::FImagePtr> &inputImages, const double threshold, const uint16_t flags) {
    std::vector<deghosting::BImagePtr> retVal;
    const uint8_t minValue = 0;
    const uint8_t maxValue = 255;
    
    // don't care about masking
    if (flags & THRESHOLD_DONTCARE) {
        for (unsigned int i=0; i < inputImages.size(); ++i) {
            deghosting::BImagePtr tmpImg(new vigra::BImage(inputImages[i]->size()));
            vigra::transformImage(vigra::srcImageRange(*tmpImg), vigra::destImage(*tmpImg),
                            vigra::Threshold<vigra::FImage::PixelType, vigra::BImage::PixelType>(threshold, 255, 0, 255));
            retVal.push_back(tmpImg);
        }
        return retVal;
    }
    
    // arrays with iterators
    std::vector<vigra::FImage::traverser> siterators(inputImages.size());
    std::vector<vigra::BImage::traverser> diterators(inputImages.size());
    // iterator to the end
    vigra::FImage::traverser send = inputImages[0]->lowerRight();
    // fill iterators and retVal
    for (unsigned int i=0; i < inputImages.size(); ++i) {
        // fill retVal
        deghosting::BImagePtr tmpImg(new vigra::BImage(inputImages[i]->size()));
        retVal.push_back(tmpImg);
        // fill iterators
        siterators[i] = inputImages[i]->upperLeft();
        diterators[i] = retVal[i]->upperLeft();
    }
    
    // leave pixels not masked in at least one image
    // this is DEFAULT
    // loop over row
    while (siterators[0].y != send.y) {
        // array with column iterators
        std::vector<vigra::FImage::traverser> siteratorsX(inputImages.size());
        std::vector<vigra::BImage::traverser> diteratorsX(inputImages.size());
        for (unsigned int i=0; i < inputImages.size(); ++i) {
            siteratorsX[i] = siterators[i];
            diteratorsX[i] = diterators[i];
        }
        // now we can loop over the same pixels in all images at once
        while (siteratorsX[0].x != send.x) {
            // image with highest weight for the pixel
            unsigned int highestI = 0;
            for (unsigned int i=0; i<inputImages.size(); ++i) {
                // apply threshold
                if (*(siteratorsX[i]) < threshold)
                    *(diteratorsX[i]) = minValue;
                else
                    *(diteratorsX[i]) = maxValue;
                // highest search
                highestI = (*(siteratorsX[highestI]) > *(siteratorsX[i])) ? highestI : i;
            }
            // set the highest one to maxValue;
            *(diteratorsX[highestI]) = maxValue;
            
            // move all iterators by one pixel to the right
            for (unsigned int i=0; i<inputImages.size(); ++i) {
                ++siteratorsX[i].x;
                ++diteratorsX[i].x;
            }
        }
        // move all iterators to the next row
        for (unsigned int i=0; i<inputImages.size(); ++i) {
            ++siterators[i].y;
            ++diterators[i].y;
        }
    }
    
    return retVal;
}
