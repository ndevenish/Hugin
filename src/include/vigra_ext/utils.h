// -*- c-basic-offset: 4 -*-
/** @file ROI.h
 *
 *  functions to manage ROI's
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
 *
 *  This is free software; you can redistribute it and/or
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

#ifndef VIGRA_EXT_UTILS_H
#define VIGRA_EXT_UTILS_H

namespace vigra_ext {
/** count pixels that are > 0 in both images */
struct OverlapSizeCounter
{
    OverlapSizeCounter()
	: size(0)
    { }

    template<typename PIXEL>
    void operator()(PIXEL const & img1, PIXEL const & img2)
    {
	if (img1 > 0 && img2 > 0) {
	    size++;
	}
    }

    unsigned int getSize()
    {
	return size;
    }

    unsigned int size;
};

/** count pixels that are > 0 in a single image */
struct MaskPixelCounter
{
    MaskPixelCounter()
	: count(0)
    { }

    template<typename PIXEL>
    void operator()(PIXEL const & img1)
    {
	if (img1 > 0) {
	    count++;
	}
    }

    int getCount()
    {
	return count;
    }

    int count;
};

} // namespace

#endif // VIGRA_EXT_UTILS_H

