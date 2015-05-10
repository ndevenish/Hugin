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
* <http://www.gnu.org/licenses/>.
*/

#ifndef __Tracer_h
#define __Tracer_h

#include <iostream>

#define TRACE_INFO(x) { std::ostringstream buf; buf << x; std::cout << buf.str(); std::cout.flush();}

#ifdef _DEBUG
#define TRACE_DEBUG(x) { std::ostringstream buf; buf << x; std::cout << buf.str(); std::cout.flush();}
#else
#define TRACE_DEBUG(x)
#endif

#define TRACE_ERROR(x) { std::ostringstream buf; buf << x; std::cerr << buf.str(); std::cerr.flush(); }







#endif //  __Tracer_h

