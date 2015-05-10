// -*- c-basic-offset: 4 -*-
/**  @file GuiLevel.h
 *
 *  @brief declaration of helper for work with different GuiLevels 
 *
 *  @author T. Modes
 *
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

#ifndef _GUISETTING_H
#define _GUISETTING_H

#include "panodata/Panorama.h"

enum GuiLevel
{
     GUI_SIMPLE=0,
     GUI_ADVANCED,
     GUI_EXPERT
};

/** returns the requiered GuiLevel for the given panorama to work correctly */
GuiLevel GetMinimumGuiLevel(HuginBase::PanoramaData& pano);

#endif
