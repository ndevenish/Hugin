// -*- c-basic-offset: 4 -*-
/** @file UniversalCursor.h
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

#ifndef _UNIVERSALCURSOR_H
#define _UNIVERSALCURSOR_H

#include "wx/cursor.h"


class wxImage;

/** a proper cursor abstraction.
 *
 *  does the same on all supported platforms!
 *  I still wonder why this couldn't be done by
 *  the wxWindows people themself.
 *
 *  currently, this is implemented for MSW and GTK.
 *  on other platforms, the wxCursor(wxImage) constructor
 *  is called.
 *
 *  It is also a bad hack, because it contains some class definitions
 *  contained in the wxWindows source. but well I don't care to hack
 *  wxWindows stuff anymore. Its too broken anyway.
 *
 */
class UniversalCursor : public wxCursor
{
public:

    /** ctor.
     *
     *  Creates a cursor out of \p img.
     *
     *  The different RGB channels have this meaning:
     *   - red contains the cursor "grey level",
     *     0-127 for black, 127-255 for white.
     *   - blue is the alpha (mask) channel
     *   - green >0 indicates the hot spot
     * 
     */
    UniversalCursor(const wxImage & img);

private:
};



#endif // _UNIVERSALCURSOR_H
