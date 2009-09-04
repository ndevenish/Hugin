/* lensFunc_glue.h

  C API that lets libpano's coordinate remapping and parameter 
  optimization systems use the generic camera model provided
  by C++ class lensFunc.

  A specific lens function is selected by a 'handle' passed to
  a generic interface function.  That handle will have been
  posted in the parameter tables by the using program.  In an 
  fDesc, func is the address of a remapping function declared 
  here, and params = handle.

  These remappers combine all the steps of transforming centered 
  image coordinates to/from spherical pano coordinates, so no 
  other fns should be put on the "image" side of the stack.

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


// remapping stack functions (can call direct, too)
int LF_sphere_photo ( double x_dest, double  y_dest, double* x_src, double* y_src, void* handle );
int LF_photo_sphere ( double x_dest, double  y_dest, double* x_src, double* y_src, void* handle );

// Optimization parameter funcs called via handle
int LF_setOptPurpose ( void * handle, int purpose );

int LF_getOptParams ( void * handle, double * ppa );

int LF_setOptParams ( void * handle, double * ppa );