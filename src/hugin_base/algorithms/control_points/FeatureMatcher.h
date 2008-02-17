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

#ifndef _CTRLPNTSALGORITHMS_FEATURE_MATCHER_H
#define _CTRLPNTSALGORITHMS_FEATURE_MATCHER_H

#include <algorithm/PanoramaAlgorithm.h>
#include <panodata/Panorama.h>

namespace HuginBase {

struct ImageKeypoint
{
    ImageKeypoint(unsigned image, unsigned keyNr, const Keypoint & key)
    {
        imageNr = image;
        keypointNr = keyNr;
        keypoint = key;
    }

    unsigned imageNr;
    unsigned keypointNr;
    Keypoint keypoint;
};

std::vector<std::vector<ImageKeypoint> >
matchKeypoints(const PanoramaData& pano, UIntSet srcImgs, UIntSet destImgs);

class FeatureMatcher : public PanoramaAlgorithm
{

    public:
        ///
        /// TODO: add some options
        FeatureMatcher(PanoramaData& panorama, UIntSet imgs)
         : PanoramaAlgorithm(panorama), m_imgs(imgs)
        {};
        
        ///
        virtual ~FeatureMatcher() {};
        
        
    public:
        ///
        virtual bool modifiesPanoramaData() const
            { return false; }
            
        ///
        virtual bool runAlgorithm()
        {
            o_controlPoints = match(o_panorama, m_imgs);
            return true; // let's hope so.
        }
          
        
    public:
        
        /** Try to match all images specified. */
        static HuginBase::CPVector & match(const PanoramaData& pano, UIntSet images );
        
        ///
        virtual const CPVector & getControlPoints() const
        { 
            // [TODO] if(!hasRunSuccessfully()) DEBUG;
            return o_controlPoints;
        }
    
        
    protected:
        CPVector o_controlPoints;
        UIntSet m_imgs;
};


}  // namespace
#endif // _H
