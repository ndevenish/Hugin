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


#ifndef __STL_UTILS_H
#define __STL_UTILS_H


#include <functional>
#include <algorithm>
#include <utility>
#include <string>
#include <string.h>
#include <ctype.h>
#include <stdexcept>

#include <common/utils.h>

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


template<typename Map>
//const Map::data_type & map_get(const Map &m, const Map::key_type & key)
typename Map::data_type & map_get(Map &m, const char * key)
{
    typename Map::iterator it = m.find(key);
    if (it != m.end()) {
        return (*it).second;
    } else {
        DEBUG_WARN("could not find " << key);
        throw std::out_of_range("No such element in vector");
    }
}

/** get a map element.
 *
 *  does not create a new element in the map, like operator[] does
 *
 *  Throws an error if the element does not exist
 */
template<typename Map>
//const Map::data_type & map_get(const Map &m, const Map::key_type & key)
typename Map::data_type & map_get(Map &m, const typename Map::key_type & key)
{
    typename Map::iterator it = m.find(key);
    if (it != m.end()) {
        return (*it).second;
    } else {
        DEBUG_WARN("could not find " << key);
        throw std::out_of_range("No such element in vector");
    }
}

template<class Map>
//const Map::data_type & map_get(const Map &m, const Map::key_type & key)
const typename Map::data_type & map_get(const Map &m, const typename Map::key_type & key)
{
    typename Map::const_iterator it = m.find(key);
    if (it != m.end()) {
        return (*it).second;
    } else {
        DEBUG_WARN("could not find " << key);
        throw std::out_of_range("No such element in vector");
    }
}

#endif // _STL_UTILS_H
