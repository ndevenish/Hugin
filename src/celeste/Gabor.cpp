/***************************************************************************
 *   Copyright (C) 2008 by Tim Nugent                                      *
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

#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include "GaborGlobal.h"
#include "GaborJet.h"
#include "ContrastFilter.h"
//#include "PGMImage.h"
#include "Utilities.h"
#include "CelesteGlobals.h"
#define kUseContrast 1

using namespace std; 

namespace celeste
{
float* ProcessChannel( float** image, int w, int h, int gNumLocs, int**& gLocations, int gRadius, float* response, int* len){

	ContrastFilter*	contrastFilter = NULL;
	GaborJet* gaborJet = NULL;
	int height = h;
	int width = w;
	float** pixels;
	int gflen, dummy;
	int i, j, offset = 0;
	char filename[256], suffix[5];

	// copy pointer
	pixels = image;

#if kUseContrast

	// apply contrast filter to image
	contrastFilter = new ContrastFilter( image, height, width );
    char file[] = "gabor_filters/celeste"; 
	// set filename if intermediate files should be saved
	if ( kSaveFilter == 1 )	{
		contrastFilter->SetFileName( file );	
		contrastFilter->Save();			// save contrast image
	}
	pixels = contrastFilter->GetContrast();		// get contrast map
	width = contrastFilter->GetWidth();		// obtain contrast dimensions
	height = contrastFilter->GetHeight();
#endif

// initialize gabor jet for the first fiducial point
	gaborJet = new GaborJet;
	if ( kSaveFilter == 1 )
	{
		strcpy( filename, file );
		sprintf( suffix, "%d-", 0 );
		strcat( filename, suffix );
		gaborJet->SetFileName( filename );
	}
	gaborJet->Initialize( height, width, gLocations[0][0], gLocations[0][1],
						  gRadius, gS, gF, gU, gL, gA, kSaveFilter );

// filter image
	// response vector is initialized here, but needs to be disposed by user
 
	gaborJet->Filter( pixels, &gflen );

	if ( *len == 0 ) 
	{
		*len = gflen * gNumLocs;
		response = new float[(*len)]; // numLocs locations

	}

	//cout << "off " << offset << " gflen " << gflen << " len " << *len << endl; 

	for ( i = 0; i < gflen; i++ ){
		
		
		response[i+offset] = gaborJet->GetResponse(i);
		//cout << i << " / " << gaborJet->GetResponse(i) << endl;
	}

	delete gaborJet;

// we already save the filters for the first fiducial, so turn it off for the others
	kSaveFilter = 0;
	
		
// process the rest of the fiducial points
	for ( i = 1; i < gNumLocs; i++ )
	{
		offset = offset + gflen;

		gaborJet = new GaborJet;
		if ( kSaveFilter == 1 )
		{
			strcpy( filename, file );
			sprintf( suffix, "%d-", i );
			strcat( filename, suffix );
			gaborJet->SetFileName( filename );
		}
		gaborJet->Initialize( height, width, gLocations[i][0], gLocations[i][1], 
							  gRadius, gS, gF, gU, gL, gA );
		
	// filter image
		// response vector is initialized here, but needs to be disposed by user
		gaborJet->Filter( pixels, &dummy );
		for ( j = 0; j < gflen; j++ ) response[j+offset] = gaborJet->GetResponse(j);
		delete gaborJet;
	}	
	
#if kUseContrast
	delete contrastFilter;
#endif

	return response;
}
}; // namespace