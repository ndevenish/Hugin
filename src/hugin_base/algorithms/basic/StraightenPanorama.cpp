// -*- c-basic-offset: 4 -*-
/** @file StraightenPanorama.cpp
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

#include "StraightenPanorama.h"

#include <hugin_math/eig_jacobi.h>

namespace HuginBase {

Matrix3 StraightenPanorama::calcStraighteningRotation(const PanoramaData& panorama)
{
    // landscape/non rotated portrait detection is not working correctly
    // should use the exif rotation tag but thats not stored anywhere currently...
    // 1: use y axis (image x axis), for normal image
    // 0: use z axis (image y axis), for non rotated portrait images
    //    (usually rotation is just stored in EXIF tag)
    std::vector<int> coord_idx;

    for (unsigned int i = 0; i < panorama.getNrOfImages(); i++) {
        SrcPanoImage img = panorama.getSrcImage(i);
        // BUG: need to read exif data here, since exif orientation is not
        // stored in Panorama data model
        double fl=0;
        double crop=0;
        img.readEXIF(fl, crop, false, false);
        double roll = img.getExifOrientation();
        if (roll == 90 || roll == 270 ) {
            coord_idx.push_back(2);
        } else {
            coord_idx.push_back(1);
        }
    }

    // build covariance matrix of X
    Matrix3 cov;
    unsigned int nrOfVariableImages=0;

    for (unsigned int i = 0; i < panorama.getNrOfImages(); i++) 
    {
        const SrcPanoImage & img=panorama.getImage(i);
        if(img.YawisLinked())
        {
            //only consider images which are not linked with the previous ones
            bool consider=true;
            for(unsigned int j=0; j<i; j++)
            {
                if(img.YawisLinkedWith(panorama.getImage(j)))
                {
                    consider=false;
                    break;
                };
            };
            if(!consider)
                continue;
        };
        double y = const_map_get(panorama.getImageVariables(i), "y").getValue();
        double p = const_map_get(panorama.getImageVariables(i), "p").getValue();
        double r = const_map_get(panorama.getImageVariables(i), "r").getValue();
        Matrix3 mat;
        mat.SetRotationPT(DEG_TO_RAD(y), DEG_TO_RAD(p), DEG_TO_RAD(r));
        nrOfVariableImages++;
        DEBUG_DEBUG("mat = " << mat);
        for (int j=0; j<3; j++) {
            for (int k=0; k<3; k++) {
                cov.m[j][k] += mat.m[j][coord_idx[i]] * mat.m[k][coord_idx[i]];
            }
        }
    }
    cov /= nrOfVariableImages;
    DEBUG_DEBUG("cov = " << cov);

    // calculate eigenvalues and vectors
    Matrix3 eigvectors;
    double eigval[3];
    int eigvalIdx[3];
    int maxsweep = 100;
    int maxannil = 0;
    double eps = 1e-16;
    
    hugin_utils::eig_jacobi(3, cov.m, eigvectors.m, eigval, eigvalIdx, &maxsweep, &maxannil, &eps);
    
    DEBUG_DEBUG("Eigenvectors & eigenvalues:" << std::endl
                << "V = " << eigvectors << std::endl
                << "D = [" << eigval[0] << ", " << eigval[1] << ", " << eigval[2] << " ]"
                << "idx = [" << eigvalIdx[0] << ", " << eigvalIdx[1] << ", " << eigvalIdx[2] << " ]");
    
    // get up vector, eigenvector with smallest eigenvalue
    Vector3 up;
    up.x = eigvectors.m[eigvalIdx[2]][0];
    up.y = eigvectors.m[eigvalIdx[2]][1];
    up.z = eigvectors.m[eigvalIdx[2]][2];
    
    // normalize vector
    up.Normalize();
    DEBUG_DEBUG("Up vector: up = " << up );
    
    double rotAngle = acos(up.Dot(Vector3(0,0,1)));
    if (rotAngle > M_PI/2) {
        // turn in shorter direction
        up *= -1;
        rotAngle = acos(up.Dot(Vector3(0,0,1)));
    }
    DEBUG_DEBUG("rotation Angle: " << rotAngle);
    
    // get rotation axis
    Vector3 rotAxis = up.Cross(Vector3(0,0,1));
    DEBUG_DEBUG("rotAxis = " << rotAngle);
    
    // calculate rotation matrix
    Matrix3 rotMat = GetRotationAroundU(rotAxis, -rotAngle);
    DEBUG_DEBUG("rotMat = " << rotMat);

    return rotMat;
}

} // namespace
