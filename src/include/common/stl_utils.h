// -*- c-basic-offset: 4 -*-

// taken from the GNU/sgi stl extensions

/*
 *
 * Copyright (c) 1994
 * Hewlett-Packard Company
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Hewlett-Packard Company makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 *
 * Copyright (c) 1996
 * Silicon Graphics Computer Systems, Inc.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Silicon Graphics makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 */


#ifndef _STL_UTILS_H
#define _STL_UTILS_H


#include <functional>
#include <algorithm>
#include <utility>
#include <string>
#include <string.h>
#include <ctype.h>

namespace utils {
    /// convert a string to lowercase
    inline std::string tolower(const std::string& s)
    {
        std::string result = s;
        std::transform<std::string::iterator,
            std::string::iterator,
            int (*)(int)>(result.begin(), result.end(),
                          result.begin(), ::tolower);
        return result;
    }
}


template<typename _Container>
//inline bool set_contains(const _Container & c, const _Container::key_type & key)
inline bool set_contains(const _Container & c, const typename _Container::key_type & key)
{
    return c.find(key) != c.end();
}

#endif // _STL_UTILS_H
