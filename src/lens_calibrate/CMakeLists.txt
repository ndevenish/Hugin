/***************************************************************************
 *   Copyright (C) 2009 by Tim Nugent                                      *
 *   timnugent@gmail.com                                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

ADD_EXECUTABLE(calibrate_lens 
Main.cpp
Globals.cpp
MapPoints.cpp
ProcessImage.cpp
Straighten.cpp
lensFunc.cpp
HermiteSpline.c
find_N8_lines.cpp
)

ADD_EXECUTABLE( test_lensFunc
test_lensFunc.cpp
lensFunc.cpp
HermiteSpline.c
)

TARGET_LINK_LIBRARIES( calibrate_lens huginANN ${image_libs} ${common_libs} )
INSTALL(TARGETS calibrate_lens DESTINATION ${BINDIR}) 
