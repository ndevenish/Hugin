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

#ifndef __detectpano_testcode_h
#define __detectpano_testcode_h

#include <localfeatures/PointMatch.h>

namespace lfeat
{
class Ransac;
}

class TestCode
{
public:
    static void drawRansacMatches(std::string& i1, std::string& i2,
                                  lfeat::PointMatchVector_t& iOK, lfeat::PointMatchVector_t& iNOK, lfeat::Ransac& iRansac, bool iHalf);


};

#endif
