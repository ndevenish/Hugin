// -*- c-basic-offset: 4 -*-
/** @file panotools/PanoToolsUtils.h
 *
 *  @brief Utility calls into PanoTools using CPP interface 
 *
 *  @author Gerry Patterson <thedeepvoice@gmail.com>
 *
 *  $Id: PanoToolsUtils.h 2510 2007-10-28 22:24:11Z dangelo $
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

#ifndef _PANOTOOLS_PTUTILS_H
#define _PANOTOOLS_PTUTILS_H

#include <hugin_shared.h>
#include <panodata/PanoramaData.h>


namespace HuginBase
{
namespace PTools
{

    /** Update the Ctrl Point errors without optimizing
     */
    IMPEX void calcCtrlPointErrors(PanoramaData & pano);

} // PTools namespace
} // HuginBase namespace

#endif // _H
