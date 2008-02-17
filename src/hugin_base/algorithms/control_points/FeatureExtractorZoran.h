// -*- c-basic-offset: 4 -*-
/** @file FeatureExtractorZoran.h
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

#include "APImage.h"
#include "HessianDetector.h"
#include "Descriptor.h"


template <class Image>
void extractFeaturesZoran(Image image)
{
    APImage im1(image);

    im1.integrate();

    HessianDetector hd1(&im1,nrPoints, HD_BOX_FILTERS,1);

    if(!hd1.detect()) {
        cout << "Detection of points failed!";
        return 1;
    }

    Descriptor d1(&im1,&hd1);
    d1.setPoints(hd1.getPoints());
    //d.orientate();
    d1.createDescriptors();

    // copy descriptors to our data structure
    std::vector<Keypoint> result;
    Keypoint p;
    p.descriptor.resize(d1.descriptors.size());
    p.laplacianSign = 1;

    vector<vector<int> >::iterator iter1 = d1.interestPoints->begin();
    vector<vector<double> >::iterator iterDesc = d1.descriptors.begin();

    //int c=0;
    while( iter1 != interestPoints->end()) {
         vector<int > tmp2=*iter1;
         p.pos = FDiff2D(tmp2[1], tmp2[0]);
         double r = _getMaxima(tmp2[0], tmp2[1])*HD_INIT_KERNEL_SIZE;
         p.scale = 1/(r*r);
         std::vector<double>::iterator destIt = p.descriptor = *iterDesc;
         result.push_back(p);
    }
    return p;
}

