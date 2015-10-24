/*
* Copyright (C) 2007-2008 Anael Orlinski
*
* This file is part of Panomatic.
*
* Panomatic is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* Panomatic is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Panomatic; if not, write to the Free Software
* <http://www.gnu.org/licenses/>.
*/

#include "RansacFiltering.h"
#include "Homography.h"

static int genint(int x)
{
    return (int)((double)rand()*x/(double)RAND_MAX);
}

namespace lfeat
{

// distance between estimate and real point in pixels.

double Ransac::calcError(Homography* aH, PointMatch& aM)
{
    double x1p, y1p;
    aH->transformPoint(aM._img1_x, aM._img1_y, x1p, y1p);

    double d1 = aM._img2_x - x1p;
    double d2 = aM._img2_y - y1p;

    return d1*d1+d2*d2;
}


void Ransac::filter(PointMatchVector_t& ioMatches, PointMatchVector_t& ioRemovedMatches)
{
    int aRemainingIterations = _nIter;
    const double aErrorDistSq = _distanceThres * _distanceThres;

    Homography aCurrentModel;

    unsigned int aMaxInliers = 0;
    PointMatchVector_t aBestInliers;
    PointMatchVector_t aBestOutliers;

    // normalization  !!!!!!
    aCurrentModel.initMatchesNormalization(ioMatches);


    //std::cout << "gravity " << _v1x << " " << _v1y << " " << _v2x << " " << _v2y << endl;

    for(; aRemainingIterations > 0; aRemainingIterations--)
    {
        //cout << "RANSAC -- iter " << aRemainingIterations << endl;

        // random select 4 matches to fit the model
        // from the input set as maybe_inliers
        PointMatchVector_t aMatches(ioMatches);
        PointMatchVector_t aInliers, aOutliers;

        //std::cout << aMatches.size() << " matches size" << endl;

        for(int i=0; i<5; i++)
        {
            int n = genint((int)aMatches.size()-1);
            aInliers.push_back(aMatches.at(n));
            aMatches.erase(aMatches.begin()+n);
        }

        //std::cout << aMatches.size() << " estimate" << endl;

        if (!aCurrentModel.estimate(aInliers))
        {
            aMatches.clear();
            continue;
        }

        // for every point remaining in aMatches, add them to aInliers if they fit the model well.
        for (size_t i = 0; i < aMatches.size(); ++i)
        {
            PointMatchPtr aMatchesIter2 = aMatches[i];
            if (calcError(&aCurrentModel, *aMatchesIter2) < aErrorDistSq)
            {
                aInliers.push_back(aMatchesIter2);
            }
            else
            {
                aOutliers.push_back(aMatchesIter2);
            }
        }

        if (aInliers.size() > aMaxInliers)
        {
            //cout << "good found -----------------" << aRemainingIterations << endl;
            //cout << aCurrentModel << endl;
            for (int i=0; i<3; ++i)
                for(int j=0; j<3; ++j)
                {
                    _bestModel._H[i][j] = aCurrentModel._H[i][j];
                }

            _bestModel._v1x = aCurrentModel._v1x;
            _bestModel._v2x = aCurrentModel._v2x;
            _bestModel._v1y = aCurrentModel._v1y;
            _bestModel._v2y = aCurrentModel._v2y;


            //*ioBestModel = *aCurrentModel;
            aMaxInliers = (unsigned int)aInliers.size();
            aBestInliers = aInliers;
            aBestOutliers = aOutliers;
            //cout << "Inliers : " << aInliers.size() << " Outliers : " << aOutliers.size() << endl;

        }

        // if there are 0 outliers then we are done.
        if (aOutliers.empty())
        {
            break;
        }

        //cout << "Inliers : " << aInliers.size() << " Outliers : " << aOutliers.size() << endl;
        //cout << "end iter" << endl;
    }

    ioMatches = aBestInliers;
    ioRemovedMatches = aBestOutliers;


}

void Ransac::transform(double iX, double iY, double& oX, double& oY)
{
    _bestModel.transformPoint(iX, iY, oX, oY);
}

}
