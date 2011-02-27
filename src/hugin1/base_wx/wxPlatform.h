// -*- c-basic-offset: 4 -*-
/** @file wxPlatform.h
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

#ifndef HUGIN_WXPLATFORM_H
#define HUGIN_WXPLATFORM_H

#include <wx/wxchar.h>
#include <wx/string.h>
#include <hugin_utils/utils.h>

namespace hugin_utils {

template <class str>
str wxQuoteStringInternal(const str & arg, const str & quotechar,
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
str wxQuoteString(const str & arg)
{
#ifdef WIN32
    // escape all strange chars with ^
    // is this true for create process?
    return wxQuoteStringInternal(arg, str(wxT("^")), str(wxT("^ \"$|()")));
#else
    return wxQuoteStringInternal(arg, str(wxT("\\")), str(wxT("\\ ~$\"|'`{}[]()")));
#endif
}

/** Quote a filename, so that it is surrounded by ""
 *
 *  @BUG I don't know the escape char for windows
 */
template <class str>
str wxQuoteFilename(const str & arg)
{
#ifdef WIN32
    str ret;
    // just a guess
    ret = wxQuoteStringInternal(arg, str(wxT("^")), str(wxT("\"")));
    return str(wxT("\"")) + ret + str(wxT("\""));
#else
    str ret;
    ret = wxQuoteStringInternal(arg, str(wxT("\\")), str(wxT("\"")));
    return str(wxT("\"")) + ret + str(wxT("\""));
#endif
    }

inline wxString doubleTowxString(double d, int digits=-1)
{
    std::string t = hugin_utils::doubleToString(d, digits);
    return wxString(t.c_str(), wxConvLocal);
}

} // namespace

#endif // HUGIN_WXPLATFORM_H
