// -*- c-basic-offset: 4 -*-
/** @file hugin1/PT/PanoToolsInterface.h
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

#ifndef _Hgn1_PT_PANOTOOLSINTERFACE_H
#define _Hgn1_PT_PANOTOOLSINTERFACE_H

#include <panotools/PanoToolsInterface.h>

namespace PTools {

    using HuginBase::PTools::Transform;

    using HuginBase::PTools::setDestImage;
    using HuginBase::PTools::setFullImage;
    using HuginBase::PTools::freeImage;

    using HuginBase::PTools::GetAlignInfoVariables;
    using HuginBase::PTools::GetAlignInfoCtrlPoints;

}

#endif // PT_PANOTOOLSINTERFACE_H
