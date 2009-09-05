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

#ifndef GLOBALS_H
#define GLOBALS_H

#include <map>
#include <string>
#include <vector>
#include "vigra/diff2d.hxx"

extern unsigned int verbose;
extern unsigned int resize_dimension;
extern unsigned int detector;
extern unsigned int lens_type;
extern unsigned int current_line;
extern unsigned int optimise_centre;
extern unsigned int optimise_radius;
extern unsigned int poly_type;
extern unsigned int straighten_verts;
extern unsigned int ss;
extern double sensor_width;
extern double sensor_height;
extern double original_width;
extern double original_height;
extern double focal_length;
extern double pixel_density;
extern double focal_length_pixels;
extern double cropFactor;
extern std::string path;
extern std::string format;
extern double sizefactor;
extern double threshold;
extern double bt_threshold;
extern double length_threshold;
extern double min_line_length_squared;
extern double scale;
extern double tscale;
extern double a;	
extern double corner_threshold;
extern unsigned int gap_limit;	
extern unsigned int horizontal_slices;	
extern unsigned int vertical_slices;	
extern unsigned int generate_images;
extern unsigned int generate_images;
extern std::vector<std::vector<vigra::Point2D> > lines;
extern double sigma;
extern double r;
extern std::map<int,double> line_errors;
extern std::map<int,double> line_weights;
extern double RMS_error;
#endif

