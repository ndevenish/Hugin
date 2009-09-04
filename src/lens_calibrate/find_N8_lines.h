/* find_N8_lines.h

  find straightish non-crossing lines in an edge map,
  using 8-neighborhood operations. (Points on the edges 
  of the image cannot be line points).

  edgeMap2linePts() marks line point in an otherwise zero 
  BImage; end points = 1, interior points = 2.  A threshold 
  on the edge map defines valid candidates.  Returns the 
  number of segments, 0 if edge and line image sizes differ.

  linePts2lineList() converts a linePts image to a list of 
  all lines that meet some selection criteria. The line list
  is a vector of pointers to N8_line objects created here;
  the vector itself is passed in by caller.  
  Returns the number of lines added
*/

/***************************************************************************
 *   Copyright (C) 2009 Thomas K Sharpless                                 *
 *   tksharpless@gmail.com                                                 *
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

#include <vector>
#include <vigra/stdimage.hxx>
#include <vigra/basicimage.hxx>
#include <vigra/pixelneighborhood.hxx>

// colors used in lines image
#define N8_bg  255
#define N8_end 1
#define N8_mid 96

int 
edgeMap2linePts( vigra::BImage & edge, bool save_images );

int 
linePts2lineList( vigra::BImage & img, 
				  int minsize, 
				  double minCtoA,  // chord/arc threshold
				  std::vector<std::vector<vigra::Point2D> > & lines
			     );


