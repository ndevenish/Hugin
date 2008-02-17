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

#ifndef _CTRLPNTSALGORITHMS_APSIFTFEATURE_EXTRACTOR_H
#define _CTRLPNTSALGORITHMS_APSIFTFEATURE_EXTRACTOR_H

#include <vector>
#include <algorithm/PanoramaAlgorithm.h>
#include <algorithms/control_points/FeatureExtractor.h>
#include <panodata/PanoImage.h>

namespace HuginBase {

void readAPSIFTXML(const char * filename, 
                   std::vector<Keypoint> & keypoints,
                   vigra::Size2D & imageSize,
                   std::string & imageFilename);

class APSIFTFeatureExtractor : public FeatureExtractorBase
{
    public:
        ///
        /// TODO: add some options
        APSIFTFeatureExtractor(PanoramaData& panorama, int imageNr)
         : FeatureExtractorBase(panorama, imageNr)
        {};

        ///
        virtual ~APSIFTFeatureExtractor() {};

    public:
        ///
        virtual bool modifiesPanoramaData() { return true; };

        static CPVector & extract(const PanoramaData& pano, int imageNr);

        ///
        virtual bool runAlgorithm();

    private:
};

void readAPSIFTXML(const char * filename);

} // namespace

#endif // _H
