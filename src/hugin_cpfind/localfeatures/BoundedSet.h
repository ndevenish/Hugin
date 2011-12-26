/*
* Copyright (C) 2007-2008 Anael Orlinski
*
* This file is part of Panomatic.
*
* Panomatic is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* Panomatic is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Panomatic; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef __detectpano_boundedset_h
#define __detectpano_boundedset_h

#include <set>
#include <limits>

// a container that keeps the N best elements during insertion.
// then the container has a maximum size of N

// TEMPLATE CLASS limited_multiset
// by default will keep the N largest values.
// to keep the N smallest values, use greater as comparator

#ifdef _WIN32
#undef max
#undef min
#endif

namespace lfeat
{

template <typename _Key, typename _Compare = std::less<_Key> >
class bounded_set
{
private:
    size_t						_maxSize;
    std::set< _Key, _Compare>	_set;

public:
    typedef	typename std::set<_Key, _Compare>::iterator iterator;

    // define the constructors
    bounded_set() : _maxSize(std::numeric_limits<size_t>::max()), _set(std::set<_Key, _Compare>()) {}

    bounded_set(size_t iMaxSize) : _set(std::set<_Key, _Compare>()), _maxSize (iMaxSize) {}

    /// sets the max size of bounded set
    void setMaxSize(int iMax)
    {
        _maxSize = iMax;
    }

    ///  Returns the maximum size of the bounded_set
    size_t max_size() const
    {
        return _maxSize;
    }

    ///  Returns the size of the limited_multiset.
    size_t size() const
    {
        return _set.size();
    }

    iterator begin()
    {
        return _set.begin();
    }

    iterator end()
    {
        return _set.end();
    }

    //	void swap(limited_multiset<_Key,_MaxLen ,_Compare, _Alloc>& __x)
    //	{
    //		multiset::swap(__x);
    //	}

    void truncate()
    {
        while (_set.size() > _maxSize)
        {
            _set.erase(_set.begin());
        }
    }

    // be careful, the returned iterator is always end !!!
    // in fact we don't know if the added value is truncated.
    void insert(const _Key& x)
    {
        _set.insert(x);
        truncate();
    }

    std::set< _Key, _Compare>& getSet()
    {
        return _set;
    }

};

}

#endif // __detectpano_boundedset_h
