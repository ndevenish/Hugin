// -*- c-basic-offset: 4 -*-
/** @file platform.h
 *
 *
 *  platform/compiler specific stuff.
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

#ifndef _HUGIN_UTILS_PLATFORM_H
#define _HUGIN_UTILS_PLATFORM_H

#include <math.h>
#include <string>

#if 0
// ??????????????????????????????????????????
#ifdef MSVC
#define snprintf _snprintf
#endif
// ??????????????????????????????????????????
#endif

// Platform identifiers
#ifndef MAC_OS_X
 #if ((defined __APPLE__) && (defined __MACH__))
  // Mac OS X platform
  #define MAC_OS_X 1
 #endif
#endif

#ifndef UNIX_LIKE
 #if __unix__ || MAC_OS_X
  // __unix__ is not defined on Mac, but Mac is a BSD
  #define UNIX_LIKE 1
 #endif
#endif


namespace hugin_utils {
    
    /// utility function; escapes characters in replacements with quotechar.
    template <class str>
    str quoteStringInternal(const str & arg, const str & quotechar,
                            const str & replacements)
    {
        // loop over all chars..
        str ret(arg);
        size_t len = replacements.size();
        for (size_t i = 0; i < len; i++) {
            str source(replacements.substr(i,1));
            str dest(quotechar + source);
            size_t idx = 0;
            do {
                idx = ret.find(source,idx);
                if (idx != str::npos) {
                    ret.replace(idx, 1, dest);
                    // skip to next unknown char.
                    idx += 2;
                }
            } while (idx != str::npos);
        }
        return ret;
    }

    
    /** Try to escape special chars on windows and linux.
     *
     * @BUG: I'm quite sure that this routine doesn't replace
     *       some important shell chars I don't know of.
     *       This could lead to nasty behaviour and maybe
     *       even security holes.
     */
    template <class str>
    str quoteString(const str & arg)
    {
    #ifdef WIN32
        // escape all strange chars with ^
        // is this true for create process?
        return quoteStringInternal(arg, str("^"), str("^ \"$|()"));
    #else
        return quoteStringInternal(arg, str("\\"), str("\\ ~$\"|'`{}[]()"));
    #endif
    }
    
    /** Quote a filename, so that it is surrounded by ""
     *
     *  @BUG I don't know the escape char for windows
     */
    template <class str>
    str quoteFilename(const str & arg)
    {
    #ifdef WIN32
        str ret;
        // just a guess
        ret = quoteStringInternal(arg, str("^"), str("\""));
        return str("\"") + ret + str("\"");
    #else
        str ret;
        ret = quoteStringInternal(arg, str("\\"), str("\""));
        return str("\"") + ret + str("\"");
    #endif
    }

} // namespace


#endif // _HUGIN_UTILS_PLATFORM_H
