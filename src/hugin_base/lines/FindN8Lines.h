// -*- c-basic-offset: 4 -*-
/**  @file FindN8Lines.h
 *
 *  @brief declaration of find lines algorithm
 *
 *  @author Thomas K. Sharpless
 *
 *  finds straightish, non-crossing lines in an edge map,
 *  using 8-neighborhood operations.
 *  This functions are intended to be used inside HuginLines lib, not to be called from outside
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

#ifndef FINDN8LINES_H
#define FINDN8LINES_H

#include <vector>
#include "LinesTypes.h"
#include <vigra/stdimage.hxx>
#include <vigra/basicimage.hxx>

namespace HuginLines
{
    /** marks line point
     *  @param input input image (should be black edges on white background)
     *  @return image with marked lines (background = 0, end points = 1, interior points = 2)
     */
    vigra::BImage edgeMap2linePts(vigra::BImage & input);
    /** converts a linePts image to a list of lines
     *  @param img lineimage
     *  @param minsize minimum length of line given in
     *  @param flPix focal length in pixel (determines the maximal allowed curvature of the line)
     *  @param lines detected lines
     *  @return number of detected lines
     */
    int linePts2lineList( vigra::BImage & img, int minsize,double flPix,Lines& lines);
}

#endif

