// -*- c-basic-offset: 4 -*-
/**  @file FindLines.h
 *
 *  @brief declaration of functions for finding lines
 *
 */

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

#ifndef FINDLINES_H
#define FINDLINES_H

#include <hugin_shared.h>
#include "LinesTypes.h"
#include "vigra/stdimage.hxx"
#include "panodata/Panorama.h"

namespace HuginLines
{
    /** detect and mark edges in an edge image using Canny's algorithm 
     *  @param input input image, on which the algorithm should run
     *  @param scale scale factor in pixel (precondition: scale > 0)
     *  @param threshold threshold for edge detection algorithm (precondition: threshold > 0)
     *  @param resize_dimension maximum dimension on which the algorithm should work
     *  @param size_factor contains the scale factor for transform from edge image to input image
     *  @return image with the marked edges
     */
    LINESIMPEX vigra::BImage* detectEdges(vigra::UInt8RGBImage input,double scale,double threshold,unsigned int resize_dimension, double &size_factor);
    LINESIMPEX vigra::BImage* detectEdges(vigra::BImage input,double scale,double threshold,unsigned int resize_dimension, double &size_factor);
    /** @brief find straightish non-crossing lines 
     *  find straightish non-crossing lines in an edge map
     *  using 8-neighborhood operations. (Points on the edges
     *  of the image cannot be line points).
     *  @param edge edge image (e.g. created with HuginLines::detectEdges
     *  @param length_threshold minimum length of a line, given in ratio to longest images dimension (0<length_threshold<1)
     *  @param focal_length focal length of the lens (used for estimate which curvature the lines can have)
     *  @param crop_factor crop factor of the camera/lens
     *  @return the found lines as HuginLines::Lines, contains also the invalid lines (e.g. too short, too curved), use Lines[].status to get result
     */
    LINESIMPEX HuginLines::Lines findLines(vigra::BImage& edge, double length_threshold, double focal_length,double crop_factor);
    /** scales the given lines with given factor 
     *  use in conjugation with HuginLines::detectEdges to scale the lines to image space because edge image to scaled to smaller size
     *  for faster computation
     */
    LINESIMPEX void ScaleLines(HuginLines::Lines& lines,const double scale);
    /** returns a HuginBase::CPVector with cps_per_lines 
     *  @param line line from which the control points should be created
     *  @param imgNr number of the image in the HuginBase::Panorama class
     *  @param lineNr number of the line to be created (must be >=3, line 1 and 2 are horizontal and vertical lines)
     *  @param numberOfCtrlPoints number of control points to create 
     *  @return HuginBase::CPVector with all control points 
     */
    LINESIMPEX HuginBase::CPVector GetControlPoints(const SingleLine line,const unsigned int imgNr, const unsigned int lineNr,const unsigned int numberOfCtrlPoints);
    /** searches for vertical control points in given image
     *  @param pano panorama object in which is searched
     *  @param imgNr number of image in which should be searched
     *  @param image vigra image in which should be searched
     *  @param nrLine maximal number of lines to return
     *  @return HuginBase::CPVector with all vertical control points
     */
    LINESIMPEX HuginBase::CPVector GetVerticalLines(const HuginBase::Panorama& pano,const unsigned int imgNr,vigra::UInt8RGBImage& image,const unsigned int nrLines);
    LINESIMPEX HuginBase::CPVector GetVerticalLines(const HuginBase::Panorama& pano,const unsigned int imgNr,vigra::BImage& image,const unsigned int nrLines);
};
#endif
