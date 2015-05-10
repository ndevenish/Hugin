// -*- c-basic-offset: 4 -*-
/** @file OptimizerSwitches.h
 *
 *  @author T. Modes
 *
 *  @brief some definitions to work with optimizer master switches 
 */

/*  This is free software; you can redistribute it and/or
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

#ifndef _OPTSWITCHES_H
#define _OPTSWITCHES_H

namespace HuginBase {

/** defines all optimizer switches, they can be combined with OR */
enum OptimizerSwitches
{
    OPT_PAIR=0x0001,
    OPT_POSITION=0x0002,
    OPT_VIEW=0x0004,
    OPT_BARREL=0x0008,
    OPT_ALL=0x0010,
    OPT_TRANSLATION=0x0020,
    OPT_EXPOSURE=0x0001,
    OPT_WHITEBALANCE=0x0002,
    OPT_VIGNETTING=0x0004,
    OPT_VIGNETTING_CENTER=0x0008,
    OPT_RESPONSE=0x0010
};

} // namespace
#endif // _OPTSWITCHES_H
