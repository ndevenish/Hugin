// -*- c-basic-offset: 4 -*-
/** @file hugin_utils/platform.h
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

#include <hugin_shared.h>
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

    /// return the CPU count. On error returns a cpu count of 0 or -1
    IMPEX int getCPUCount();
    
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
            size_t destlen = dest.size();
            size_t idx = 0;
            do {
                idx = ret.find(source,idx);
                if (idx != str::npos) {
                    ret.replace(idx, 1, dest);
                    // skip to next unknown char.
                    idx += destlen;
                }
            } while (idx != str::npos);
        }
        return ret;
    }

#ifdef _WINDOWS
    /// utility function; replaces backslash with slash
    template <class str>
    str replaceBackslash(const str & arg)
    {
        str ret(arg);
        size_t idx = 0;
        do 
        {
            idx = ret.find(str("\\"),idx);
            if (idx != str::npos) 
            {
                ret.replace(idx, 1, str("/"));
                idx++;
            }
        } 
        while (idx != str::npos);
        return ret;
    };
#endif    

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

    /** Try to escape special chars in a string used by a unix type shell
     *
     * @BUG: I'm quite sure that this routine doesn't replace
     *       some important shell chars I don't know of.
     *       This could lead to nasty behaviour and maybe
     *       even security holes.
     *
     *  Note that : and = are not special shell charaters but they also
     *  should be escaped because they causes problems with gnumake.
     */
    template <class str>
    str quoteStringShell(const str & arg)
    {
#ifdef WIN32
        // Do not quote backslash,: and ~ on win32.
        // we only need to escape hash (#) and $, all other chars are handled by quoting with " "
        return str("\"")+quoteStringInternal(quoteStringInternal(replaceBackslash(arg),str("\\"),str("#")), str("$"), str("$"))+str("\"");
#else
        return quoteStringInternal(quoteStringInternal(arg, str("\\"), str("\\ ~$\"|'`{}[]()*#:=")), str("$"), str("$"));
#endif
    }

    /** Escape dangerous chars in makefile strings/filenames
     *  (space),#,=
     */
    template <class str>
    str escapeStringMake(const str & arg)
    {
#ifdef WIN32
        // Do not escape colon in windows because it causes problems with absolute paths
        return quoteStringInternal(quoteStringInternal(replaceBackslash(arg), str("\\"), str(" #=")), str("$"), str("$"));
#else
        return quoteStringInternal(quoteStringInternal(arg, str("\\"), str(" #:=")), str("$"), str("$"));
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
        ret = quoteStringInternal(arg, str("\\"), str("\"$'\\"));
        return str("\"") + ret + str("\"");
    #endif
    }

} // namespace


#endif // _HUGIN_UTILS_PLATFORM_H
