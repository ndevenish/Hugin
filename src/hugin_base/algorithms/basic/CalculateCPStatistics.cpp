// -*- c-basic-offset: 4 -*-
/** @file CalculateCPStatistics.cpp
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


#include "CalculateCPStatistics.h"

#include <math.h>
#include <hugin_math/hugin_math.h>
#include <panodata/PanoramaData.h>


namespace HuginBase {

    
    
void CalculateCPStatisticsError::calcCtrlPntsErrorStats(const PanoramaData& pano,
                                                        double & min, double & max, double & mean,
                                                        double & var,
                                                        const int& imgNr)
{
    const CPVector& cps = pano.getCtrlPoints();
    
    max = 0;
    min = 1000000;
    mean = 0;
    var = 0;
    
    int n=0;
    CPVector::const_iterator it;
    for (it = cps.begin() ; it != cps.end(); it++) {
        if (imgNr >= 0 && ((int)(*it).image1Nr != imgNr || (int)(*it).image2Nr != imgNr))
        {
            continue;
        }
        n++;
        double x = (*it).error;
        double delta = x - mean;
        mean += delta/n;
        var += delta*(x - mean);
        if (x > max) {
            max= (*it).error;
        }
        if (x < min) {
            min= (*it).error;
        }
    }
    var = var/(n-1);
}    



void CalculateCPStatisticsRadial::calcCtrlPntsRadiStats(const PanoramaData& pano,
                                                  double & min, double & max, double & mean, double & var,
                                                  double & q10, double & q90, 
                                                  const int& imgNr)
{
    // calculate statistics about distance of control points from image center
    max = 0;
    min = 1000;
    mean = 0;
    var = 0;
    
    int n=0;
    CPVector::const_iterator it;
    const CPVector & cps = pano.getCtrlPoints();
    std::vector<double> radi;
    for (it = cps.begin() ; it != cps.end(); it++) {
        if (imgNr >= 0 && ((int)(*it).image1Nr != imgNr || (int)(*it).image2Nr != imgNr))
        {
            continue;
        }
        const SrcPanoImage & img1 = pano.getImage((*it).image1Nr);
        const SrcPanoImage & img2 = pano.getImage((*it).image2Nr);
        const vigra::Size2D img1_size = img1.getSize();
        int w1 = img1_size.width();
        int h1 = img1_size.height();
        const vigra::Size2D img2_size = img2.getSize();
        int w2 = img2_size.width();
        int h2 = img2_size.height();
        
        // normalized distance to image center
        double x1 = ((*it).x1-(w1/2.0)) / (h1/2.0);
        double y1 = ((*it).y1-(h1/2.0)) / (h1/2.0);
        double x2 = ((*it).x2-(w2/2.0)) / (h2/2.0);
        double y2 = ((*it).y2-(h2/2.0)) / (h2/2.0);
        
        double r1 = sqrt(x1*x1 + y1*y1);
        radi.push_back(r1);
        double r2 = sqrt(x2*x2 + y2*y2);
        radi.push_back(r2);
        
        double x = r1;
        n++;
        double delta = x - mean;
        mean += delta/n;
        var += delta*(x - mean);
        if (x > max) {
            max= x;
        }
        if (x < min) {
            min= x;
        }
        
        x = r2;
        n++;
        delta = x - mean;
        mean += delta/n;
        var += delta*(x - mean);
        if (x > max) {
            max= x;
        }
        if (x < min) {
            min= x;
        }
    }
    var = var/(n-1);
    
    std::sort(radi.begin(), radi.end());
    q10 = radi[hugin_utils::floori(0.1*radi.size())];
    q90 = radi[hugin_utils::floori(0.9*radi.size())];
}
    
    

} // namespace
