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


#ifndef _HUGIN_UTILS_STL_UTILS_H
#define _HUGIN_UTILS_STL_UTILS_H


#include <functional>
#include <algorithm>
#include <utility>
#include <string>
#include <string.h>
#include <ctype.h>
#include <stdexcept>

#include <hugin_utils/utils.h>

namespace hugin_utils {
    
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


///
template<typename _Container>
//inline bool set_contains(const _Container & c, const _Container::key_type & key)
inline bool set_contains(const _Container & c, const typename _Container::key_type & key)
{
    return c.find(key) != c.end();
}

///
template<typename _Container>
inline void fill_set(_Container & c, typename _Container::key_type begin,
					 typename _Container::key_type end)
{
	for (typename _Container::key_type i=begin; i <= end; i++) {
		c.insert(i);
	}
}



/** get a map element.
*
*  does not create a new element in the map, like operator[] does
*
*  Throws an error if the element does not exist
*/
template<typename Map>
typename Map::mapped_type & map_get(Map &m, const typename Map::key_type & key)
{
    typename Map::iterator it = m.find(key);
    if (it != m.end()) {
        return (*it).second;
    } else {
        DEBUG_WARN("could not find " << key);
        throw std::out_of_range("No such element in vector");
    }
}

template<typename Map>
const typename Map::mapped_type & const_map_get(const Map &m, const typename Map::key_type & key)
{
    typename Map::const_iterator it = m.find(key);
    if (it != m.end()) {
        return (*it).second;
    } else {
        DEBUG_WARN("could not find " << key);
        throw std::out_of_range("No such element in vector");
    }
}


template<typename Map>
typename Map::mapped_type & map_get(Map &m, const char * key)
{
    typename Map::iterator it = m.find(key);
    if (it != m.end()) {
        return (*it).second;
    } else {
        DEBUG_WARN("could not find " << key);
        throw std::out_of_range("No such element in vector");
    }
}

template<typename Map>
const typename Map::mapped_type & const_map_get(const Map &m, const char * key)
{
    typename Map::const_iterator it = m.find(key);
    if (it != m.end()) {
        return (*it).second;
    } else {
        DEBUG_WARN("could not find " << key);
        throw std::out_of_range("No such element in vector");
    }
}

#endif // _HUGIN_UTILS_STL_UTILS_H
