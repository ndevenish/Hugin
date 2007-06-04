// -*- c-basic-offset: 4 -*-
/** @file PanoramaDataLegacySupport.h
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

#ifndef _PANORAMA_H
#define _PANORAMA_H

#include "common/math.h"
#include "common/utils.h"

#include <vector>
#include <set>
#include <sstream>

#include "PT/PanoImage.h"
#include "PT/PanoramaMemento.h"



namespace PT {

    
class PanoramaDataLegacySupport
{
public:
    
    virtual ~PanoramaDataLegacySupport();
    
    
// -- Algorithms to be modified. --
    
    /** update control points distances.
    *
    *  updates control distances and position in final panorama
    *  usually used to set the changes from the optimization.
    *  The control points must be the same as in
    */
    virtual void updateCtrlPointErrors(const CPVector & controlPoints);
    
    /** update control points for a subset of images.
    *
    *  Usually, the control point subset is created using subset()
    *  The number and ordering and control points must not be changed
    *  between the call to subset() and this function.
    */
    void updateCtrlPointErrors(const UIntSet & imgs, const CPVector & cps);
    
    
// -- Algorithms to be moved out. --
        
    /** calculates the horizontal and vertial FOV of the complete panorama
    *
    *  @return HFOV,VFOV
    */
    FDiff2D calcFOV() const;
    
    /** calculate the HFOV and height so that the whole input
        *  fits in into the output panorama */
    void fitPano(double & HFOV, double & height);
    
    /** calculate the optimal width for this panorama 
        *
        *  Optimal means that the pixel density at the panorama and
        *  image center of the image with the highest resolution
        *  are the same.
        */
    unsigned calcOptimalWidth() const;
    
    /** calculate control point error distance statistics */
    void calcCtrlPntsErrorStats(double & min, double & max, double & mean, double & std, int imgNr=-1) const;
    
    /** calculate control point radial distance statistics. q10 and q90 are the 10% and 90% quantile */
    void calcCtrlPntsRadiStats(double & min, double & max, double & mean, double & var,
                               double & q10, double & q90, int imgNr=-1) const;
    
    /** center panorama horizontically */
    void centerHorizontically();
    
    /** rotate the complete panorama
        *
        *  Will modify the position of all images.
        */
    void rotate(double yaw, double pitch, double roll);
    
    /** rotate the complete panorama.
        *
        *  Will modify the position of all images.
        */
    void rotate(const Matrix3 & rot);
    
    /** try to automatically straighten the panorama */
    void straighten();
    
}


} // namespace




#endif // _PANORAMA_H
