// -*- c-basic-offset: 4 -*-
/** @file ComputeImageROI.h
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

#ifndef _COMPUTE_IMAGE_ROI_H
#define _COMPUTE_IMAGE_ROI_H

#include <hugin_shared.h>
#include <vigra/diff2d.hxx>
#include <algorithms/PanoramaAlgorithm.h>
#include <panodata/Panorama.h>
#include <panodata/PanoramaOptions.h>



namespace HuginBase {

IMPEX vigra::Rect2D estimateOutputROI(const PanoramaData & pano, const PanoramaOptions & opts, unsigned i);

class IMPEX ComputeImageROI : public PanoramaAlgorithm
{

    public:
        ///
        ComputeImageROI(PanoramaData& panorama, const UIntSet & images)
         : PanoramaAlgorithm(panorama), m_images(images)
        {};

        ///
        virtual ~ComputeImageROI() {};

    public:
        ///
        virtual bool modifiesPanoramaData() const
            { return false; }
        ///
        virtual bool runAlgorithm()
        {
            m_rois = computeROIS(o_panorama, o_panorama.getOptions(), m_images);
            return true; // let's hope so.
        }

        virtual std::vector<vigra::Rect2D> getROIS()
        {
            return m_rois;
        }

    public:
        ///
        static std::vector<vigra::Rect2D> computeROIS(const PanoramaData& panorama,
                                                      const PanoramaOptions & opts,
                                                      const UIntSet & images);

    protected:
        UIntSet m_images;
        std::vector<vigra::Rect2D> m_rois;
};


} // namespace
#endif // _COMPUTE_IMAGE_ROI_H
