// -*- c-basic-offset: 4 -*-

/** @file LoweSIFT.cpp
 *
 *  @brief implementation of LoweSIFT Class
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
 *
 *  This program is free software; you can redistribute it and/or
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
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "vigra_ext/LoweSIFT.h"

using namespace vigra_ext;

/** write sift feature to stream (lowe's format) */
inline std::ostream & operator<<(std::ostream & o, const SIFTFeature & f)
{
    o << f.pos.y << " " << f.pos.x << " " << f.scale << " " << f.angle
      << std::endl;
    for (unsigned int i=0; i < f.descriptor.size(); ++i) {
        o << (int) f.descriptor[i] << " ";
    }
    o << std::endl;
    return o;
}

