// -*- c-basic-offset: 4 -*-

/** @file ImageProcessing.cpp
 *
 *  @brief some image processing functions
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "panoinc.h"
#include "panoinc_WX.h"
#include "hugin/ImageProcessing.h"

using namespace vigra;

wxImageIterator
wxImageUpperLeft(wxImage & img)
{
    return wxImageIterator((vigra::RGBValue<unsigned char> *)img.GetData(), img.GetWidth());
}

wxImageIterator
wxImageLowerRight(wxImage & img)
{
    return wxImageUpperLeft(img) + vigra::Dist2D(img.GetWidth(), img.GetHeight());
}

