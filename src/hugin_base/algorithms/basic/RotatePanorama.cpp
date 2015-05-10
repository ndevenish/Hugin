// -*- c-basic-offset: 4 -*-
/** @file RotatePanorama.cpp
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
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "RotatePanorama.h"


namespace HuginBase {
    

///
RotatePanorama::RotatePanorama(PanoramaData& panorama, double yaw, double pitch, double roll)
 : PanoramaAlgorithm(panorama)
{
    o_transformMat.SetRotationPT(DEG_TO_RAD(yaw), DEG_TO_RAD(pitch), DEG_TO_RAD(roll));
}
    

#define conditional_set(variable, value) \
if (image.variable##isLinked())\
{\
    unsigned int j = 0;\
    while (j < i && !image.variable##isLinkedWith(panorama.getImage(j)))\
    {\
        j++;\
    }\
    if (j == i) copy.set##variable(value);\
} else {\
    copy.set##variable(value);\
}

void RotatePanorama::rotatePano(PanoramaData& panorama, const Matrix3& transformMat)
{
    for (unsigned int i = 0; i < panorama.getNrOfImages(); i++)
    {
        const SrcPanoImage & image = panorama.getImage(i);
        SrcPanoImage copy = image;
        double y = image.getYaw();
        double p = image.getPitch();
        double r = image.getRoll();
        Matrix3 mat;
        mat.SetRotationPT(DEG_TO_RAD(y), DEG_TO_RAD(p), DEG_TO_RAD(r));
        DEBUG_DEBUG("rotation matrix (PT) for img " << i << " << ypr:" << y << " " << p << " " << r << std::endl << mat);
        Matrix3 rotated;
        rotated = transformMat * mat;
        DEBUG_DEBUG("rotation matrix after transform: " << rotated);
        rotated.GetRotationPT(y,p,r);
        y = RAD_TO_DEG(y);
        p = RAD_TO_DEG(p);
        r = RAD_TO_DEG(r);
        DEBUG_DEBUG("rotated angles of img " << i << ": " << y << " " << p << " " << r);
        
        // Don't update a variable linked to a variable we already updated.
        conditional_set(Yaw, y);
        conditional_set(Pitch, p);
        conditional_set(Roll, r);
        if(image.getX()!=0.0 || image.getY()!=0.0 || image.getZ()!=0.0)
        {
            // rotate translation vector
            Vector3 vecRot=transformMat.Inverse().TransformVector(Vector3(image.getZ(), image.getX(), image.getY()));
            conditional_set(X, vecRot.y);
            conditional_set(Y, vecRot.z);
            conditional_set(Z, vecRot.x);
            // rotate translation plane
            mat.SetRotationPT(DEG_TO_RAD(image.getTranslationPlaneYaw()), DEG_TO_RAD(image.getTranslationPlanePitch()), 0.0);
            rotated = transformMat * mat;
            rotated.GetRotationPT(y,p,r);
            conditional_set(TranslationPlaneYaw, RAD_TO_DEG(y));
            conditional_set(TranslationPlanePitch, RAD_TO_DEG(p));
        };
        panorama.setImage(i, copy);
        panorama.imageChanged(i);
    }
}


} //namespace
