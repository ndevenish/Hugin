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

#ifndef __lfeat_sieve_h
#define __lfeat_sieve_h

#include "BoundedSet.h"

namespace lfeat
{

template <typename _Key>
class SieveExtractor
{
public:
    virtual void operator()(const _Key& k) = 0;
};

template <typename _Key, typename _Compare = std::less<_Key> >
class Sieve
{

public:
    Sieve(int iWidth, int iHeight, int iLength);
    void insert(_Key& iElem, int iWidth, int iHeight);

    // extract
    void extract(SieveExtractor<_Key>& iEx);

private:
    std::vector<lfeat::bounded_set< _Key, _Compare > > _buckets;
    int _width, _height;


};

template <typename _Key, typename _Compare>
Sieve<_Key, _Compare>::Sieve(int iWidth, int iHeight, int iLength) :
    _buckets(std::vector<bounded_set< _Key, _Compare > >(iWidth* iHeight)),
    _width(iWidth), _height(iHeight)
{
    typename std::vector<bounded_set< _Key, _Compare > >::iterator aVB, aVE;
    aVB = _buckets.begin();
    aVE = _buckets.end();
    for (; aVB != aVE; ++aVB)
    {
        aVB->setMaxSize(iLength);
    }
}

template <typename _Key, typename _Compare>
void Sieve<_Key, _Compare>::insert(_Key& iElem, int iWidth, int iHeight)
{
    _buckets[iWidth * _height + iHeight].insert(iElem);
}

template <typename _Key, typename _Compare>
void Sieve<_Key, _Compare>::extract(SieveExtractor<_Key>& iEx)
{
    typename std::vector<bounded_set< _Key, _Compare > >::iterator aVB, aVE;
    aVB = _buckets.begin();
    aVE = _buckets.end();
    for (; aVB != aVE; ++aVB)
    {
        typename std::set<_Key, _Compare>::iterator aSB, aSE;
        std::set<_Key, _Compare>& aS = aVB->getSet();
        aSB = aS.begin();
        aSE = aS.end();
        for (; aSB != aSE; ++aSB)
        {
            iEx(*aSB);
        }
    }
}

}

#endif // __lfeat_sieve_h
