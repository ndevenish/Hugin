// -*- c-basic-offset: 4 -*-

/** @file CalculateOverlap.h
 *
 *  @brief definitions of classes to calculate overlap between different images
 *
 *  @author Thomas Modes
 *
 *
 *  $Id$
 *
 */

/*  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _ALGO_CALCULATE_OVERLAP_H
#define _ALGO_CALCULATE_OVERLAP_H

#include <hugin_shared.h>
#include <panodata/PanoramaData.h>
#include <panodata/Panorama.h>
#include <panotools/PanoToolsInterface.h>

namespace HuginBase 
{

/** class for calculating overlap of images */
class IMPEX CalculateImageOverlap
{
public:
    /** constructor */
    explicit CalculateImageOverlap(const PanoramaData * pano);
    /** destructor */
    virtual ~CalculateImageOverlap();
    /** does the calculation, 
        for each image steps*steps points are extracted and tested with all other images overlap */
    void calculate(unsigned int steps);
    /** returns the overlap for 2 images with number i and j */
    double getOverlap(unsigned int i, unsigned int j) const;
    /** limits the calculation of the overlap to given image numbers */
    void limitToImages(UIntSet img);
    /** returns a set of images which overlap with given image number */
    UIntSet getOverlapForImage(unsigned int i) const;
    /** return number of images in underlying pano */
    unsigned int getNrOfImages() const { return m_nrImg; };

private:
    std::vector<std::vector<double> > m_overlap;
    std::vector<PTools::Transform*> m_transform;
    std::vector<PTools::Transform*> m_invTransform;
    unsigned int m_nrImg;
    const PanoramaData* m_pano;
    std::vector<unsigned int> m_testImages;
};

} //namespace
#endif // _ALGO_CALCULATE_OVERLAP_H
