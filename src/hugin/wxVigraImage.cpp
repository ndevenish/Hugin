// -*- c-basic-offset: 4 -*-

/** @file wxVigraImage.cpp
 *
 *  @brief implementation of wxVigraImage Class
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

#include "hugin/wxVigraImage.h"


using namespace vigra;

triple<wxVigraImage::ConstIterator,
              wxVigraImage::ConstIterator,
              wxVigraImage::ConstAccessor>
srcImageRange(wxVigraImage const & img)
{
    return vigra::triple<wxVigraImage::ConstIterator,
                  wxVigraImage::ConstIterator,
                  wxVigraImage::ConstAccessor>(img.upperLeft(),
                                               img.lowerRight(),
                                                img.accessor());
}

pair< wxVigraImage::ConstIterator,
             wxVigraImage::ConstAccessor>
srcImage(wxVigraImage const & img)
{
    return pair<wxVigraImage::ConstIterator,
                wxVigraImage::ConstAccessor>(img.upperLeft(),
                                         img.accessor());
}


inline triple< wxVigraImage::Iterator,
               wxVigraImage::Iterator,
           wxVigraImage::Accessor>
destImageRange(wxVigraImage & img)
{
    return triple<wxVigraImage::Iterator,
                  wxVigraImage::Iterator,
          wxVigraImage::Accessor>(img.upperLeft(),
                                        img.lowerRight(),
                        img.accessor());
}

inline pair< wxVigraImage::ConstIterator,
             wxVigraImage::ConstAccessor>
maskImage(wxVigraImage const & img)
{
    return pair<wxVigraImage::ConstIterator,
                wxVigraImage::ConstAccessor>(img.upperLeft(),
                                         img.accessor());
}
