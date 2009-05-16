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

#include "CelesteGlobals.h"
#include <stdlib.h>

// GLOBAL GLOBALS

bool kSaveFilter = true;  // in case of multiple files, we save GFs only once
bool kVerbosity  = false; // whether to output any messages or not

// LOCAL GLOBALS

// -r : Radius of filter
int gRadius = 20;

// -s : Sigma modulator			
float gS = 18.0;

// -a : Number of angles				
int gA = 8;

// -f : Number of frequencies				
int gF = 6;

// -l : Lower bound of frequency				
float gL = 0.1f;

// -u : Upper bound of frequency
float gU = 1.8f;	

// Number of fiducials			
int gNumLocs = 0;

// Co-ordinates of fiducials			
int **gLocations = NULL;

// Spacing between fiducial points
int spacing = (gRadius * 2) + 1;

int resize_dimension = 820;
