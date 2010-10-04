/***************************************************************************
 *   Copyright (C) 2008 by Tim Nugent
 *   timnugent@gmail.com
 *
 *   This file is part of hugin.
 *
 *   Hugin is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
 *   (at your option) any later version.
 * 
 *   Hugin is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 * 
 *   You should have received a copy of the GNU General Public License
 *   along with Hugin  If not, see <http://www.gnu.org/licenses/>.
 *
 ***************************************************************************/

#ifndef __GABOR__
#define __GABOR__

namespace celeste
{
float* ProcessChannel( float** image, int w, int h, int gNumLocs, int**& gLocations, int gRadius, float* response, int* len);
}; // namespace
#endif

