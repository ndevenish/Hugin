// -*- c-basic-offset: 4 ; tab-width: 4 -*-
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

#ifndef __detectpano_panodetectordefs_h
#define __detectpano_panodetectordefs_h
#include <vector>
#include <iostream>


#include <boost/shared_ptr.hpp>
#include <localfeatures/KeyPoint.h>
#include "KDTree.h"
#include "Utils.h"


// define KDTree element from a KeyPointPtr
// define a class to wrap an Ipoint and make it KDTree compliant.
class KDElemKeyPoint : public KDTreeSpace::KDTreeElemInterface<double>
{
public:
    KDElemKeyPoint (lfeat::KeyPointPtr& iK, int iNumber) : _ivec(iK->_vec), _n(iNumber) {}
    inline double& getVectorElem(int iPos) const
    {
        return _ivec[iPos];   // access to the vector elements.
    }
    double* _ivec;
    size_t _n;
};

typedef std::vector<KDElemKeyPoint>	KDElemKeyPointVect_t;








#endif // __detectpano_panodetectordefs_h

