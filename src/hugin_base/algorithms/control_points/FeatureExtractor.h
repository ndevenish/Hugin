// -*- c-basic-offset: 4 -*-
/** @file PanoramaDataLegacySupport.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: Panorama.h 1947 2007-04-15 20:46:00Z dangelo $
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

#ifndef _CTRLPNTSALGORITHMS_FEATURE_EXTRACTOR_H
#define _CTRLPNTSALGORITHMS_FEATURE_EXTRACTOR_H

#include <algorithm/PanoramaAlgorithm.h>
#include <panodata/PanoImage.h>

namespace HuginBase {

/** Abstract class that encapsulates a feature matcher */
class FeatureExtractorBase : public PanoramaAlgorithm
{

    public:
        ///
        /// TODO: add some options
        FeatureExtractorBase(PanoramaData& panorama, int imageNr)
         : PanoramaAlgorithm(panorama)
        {};

        ///
        virtual ~FeatureExtractorBase() {};


    public:
        ///
        virtual bool modifiesPanoramaData() const = 0;

        ///
        virtual bool runAlgorithm() = 0;

    public:

    protected:
        /// some utility functions required by subclasses

        /// get scaled down image

        /** get image remapped to stereographic coordinates.
         *  Useful for wide angle and fisheye images
         */
        // void getStereographicImage(vigra::FImage &);

        /**
         */

        unsigned m_imageNr;

        // transformation from stereographic coordinates to original coordinates
//        PT::Transform m_transform;
};

} // namespace

#endif // _H
