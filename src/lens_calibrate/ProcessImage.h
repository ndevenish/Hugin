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

#ifndef PROCESSIMAGE_H
#define PROCESSIMAGE_H

#include "vigra/impex.hxx"
#include "vigra/diff2d.hxx"

double line_length_squared(int&, int&, int&, int&);
bool compare_xy (const vigra::Point2D,const vigra::Point2D);
bool compare_yx (const vigra::Point2D,const vigra::Point2D);
bool fileexists(std::string);
void tokenize(const std::string&, std::vector<std::string>&, const std::string&);
void resize_image(vigra::UInt16RGBImage&, int&, int&);
void find_ann(std::vector<vigra::Point2D>&, vigra::FVector2Image&, unsigned int&);
void sort_inliers(std::vector<vigra::Point2D>&);
void plot_inliers(std::string&, vigra::BImage& image, std::vector<vigra::Point2D>&, int);
void extract_coords(vigra::BImage&, std::vector<vigra::Point2D>&);
void nuke_corners(vigra::BImage&, vigra::FImage&, std::string&);
void detect_edge(vigra::BImage&, std::string&, vigra::BImage&);
void process_image(std::string&, int&);void sort_lines_by_length();
bool compare_line_length(std::vector<vigra::Point2D>, std::vector<vigra::Point2D>);
void generate_boundary_tensor(vigra::BImage&, vigra::FVector2Image&, vigra::FImage&, std::string&);
int min_x_index(std::vector<vigra::Point2D>&);
int max_x_index(std::vector<vigra::Point2D>&);
int min_y_index(std::vector<vigra::Point2D>&);
int max_y_index(std::vector<vigra::Point2D>&);

#endif
