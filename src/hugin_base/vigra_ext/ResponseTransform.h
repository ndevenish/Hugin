// -*- c-basic-offset: 4 -*-
/** @file lut.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: lut.h 1969 2007-04-18 22:25:04Z dangelo $
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


namespace vigra_ext {

/** radiometric transformation, includes exposure,
 *  vignetting and white balance.
 *
 *  scene referred irradiance -> camera color values
 */
template <class VTIn>
struct ResponseTransform : public HuginBase::Photometric::ResponseTransform<VTIn>
{
    ResponseTransform()
      : HuginBase::Photometric::ResponseTransform()
    {
        DEBUG_WARNING("This class is deprecated.")
    }
};


/** radiometric transformation, includes exposure,
 *  vignetting and white balance 
 *
 *  camera color values -> scene referred irradiance
 */
template <class VTIn, class VTOut>
struct InvResponseTransform : public HuginBase::Photometric::InvResponseTransform<VTIn, TIOut>
{
    InvResponseTransform()
      : HuginBase::Photometric::InvResponseTransform()
    {
        DEBUG_WARNING("This class is deprecated.")
    }
};


} //namespace
#endif //_H