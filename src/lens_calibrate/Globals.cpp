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

#include <map>
#include "Globals.h"
#include <string>
#include <vector>

unsigned int verbose = 0;
unsigned int resize_dimension = 1600;
unsigned int detector = 0;
unsigned int lens_type = 1;
unsigned int current_line = 0;
unsigned int optimise_centre = 0;
unsigned int optimise_radius = 1;
unsigned int poly_type = 2;
unsigned int ss = 0;
unsigned int straighten_verts = 0;
double sensor_width = 0;
double sensor_height = 0;
double original_width = 0;
double original_height = 0;;
double focal_length = 0;
double pixel_density = 4000;
double focal_length_pixels = 0;
double cropFactor = 0;
/*
#ifdef _WIN32
  std::string path = ("output\\");
#else
  std::string path = ("output/");
#endif
*/
std::string path = ("");
std::string format = (".jpg");
double sizefactor = 1;
double threshold = 4;
double bt_threshold = 75;
double length_threshold = 0.3;
double min_line_length_squared;
double scale = 2;
double tscale = 1.45;
double a = 140.0;	
double corner_threshold = 150;
unsigned int gap_limit = 4;	
unsigned int vertical_slices = 12;	
unsigned int horizontal_slices = 8;	
unsigned int generate_images = 1;
std::vector<std::vector<vigra::Point2D> > lines;
double sigma = 1.4;
double r = 140;
std::map<int,double> line_errors;
std::map<int,double> line_weights;
double RMS_error = 0;
