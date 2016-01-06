// -*- c-basic-offset: 4 -*-
/** @file PanoramaMemento.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
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


#include "ControlPoint.h"


namespace HuginBase {

    
bool ControlPoint::operator==(const ControlPoint & o) const
{
    return (image1Nr == o.image1Nr &&
            image2Nr == o.image2Nr &&
            x1 == o.x1 && y1 == o.y1 &&
            x2 == o.x2 && y2 == o.y2 &&
            mode == o.mode &&
            error == o.error);
}


void ControlPoint::mirror()
{
    unsigned int ti;
    double td;
    ti =image1Nr; image1Nr = image2Nr, image2Nr = ti;
    td = x1; x1 = x2 ; x2 = td;
    td = y1; y1 = y2 ; y2 = td;
}

const std::string ControlPoint::getCPString() const
{
    std::ostringstream s;
    s << mode;
    if(image1Nr<=image2Nr)
    {
        s << " " << image1Nr << ": " << x1 << "," << y1 << "|" << image2Nr << ": " << x2 << "," <<y2;
    }
    else
    {
        s << " " << image2Nr << ": " << x2 << "," << y2 << "|" << image1Nr << ": " << x1 << "," <<y1;
    }
    return s.str();
};

} // namespace
