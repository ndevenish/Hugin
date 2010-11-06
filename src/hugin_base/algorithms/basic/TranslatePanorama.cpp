// -*- c-basic-offset: 4 -*-
/** @file TranslatePanorama.cpp
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>  Yuval Levy <http://www.photopla.net/>
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

#include "TranslatePanorama.h"


namespace HuginBase {

///
TranslatePanorama::TranslatePanorama(PanoramaData& panorama)
 : PanoramaAlgorithm(panorama)
{
}

void TranslatePanorama::translatePano(PanoramaData& panorama, const double x, const double y, const double z)
{

    for (unsigned int i = 0; i < panorama.getNrOfImages(); i++)
    {
        const SrcPanoImage & image = panorama.getImage(i);
        double ix = image.getX();
        double iy = image.getY();
        double iz = image.getZ();
        
        // Don't update a variable linked to a variable we already updated.
        SrcPanoImage copy = image;
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
        conditional_set(X, ix+x);
        conditional_set(Y, iy+y);
        conditional_set(Z, iz+z);
        
        panorama.setImage(i, copy);
        panorama.imageChanged(i);
    }
}


} //namespace
