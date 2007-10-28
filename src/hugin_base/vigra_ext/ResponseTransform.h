// -*- c-basic-offset: 4 -*-
/** @file lut.h
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
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _VIGRA_EXT_RESPONSETRANSFORM_H
#define _VIGRA_EXT_RESPONSETRANSFORM_H

#include <photometric/ResponseTransform.h>

// does not work on MSVC
//#warning "Deprecated!"

namespace vigra_ext {

    using HuginBase::Photometric::ResponseTransform;
    using HuginBase::Photometric::InvResponseTransform;

}

#endif //_H
