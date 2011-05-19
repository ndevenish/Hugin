// -*- c-basic-offset: 4 -*-
/**  @file LinesTypes.h
 *
 *  @brief types definitions for line finding algorithm
 *
 */

 /*  This is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef LINESTYPES_H
#define LINESTYPES_H
#include <vector>
#include <vigra/diff2d.hxx>

namespace HuginLines
{

/** enumeration for different line status */
enum LineStatus
{
    valid_line=0,
    valid_line_disabled,
    bad_length,
    bad_orientation,
    bad_curvature
};

/** a single line extracted from image */
struct SingleLine
{
    std::vector<vigra::Point2D> line;
    LineStatus status;
};

/** vector of extracted lines from image */
typedef std::vector<SingleLine> Lines;

} //namespace
#endif