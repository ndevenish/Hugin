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

#ifndef __Tracer_h
#define __Tracer_h

#include <iostream>
#include "zthread/FastMutex.h"
#include "zthread/Guard.h"

static ZThread::FastMutex aIOMutex;

#define TRACE_INFO(x) { ZThread::Guard<ZThread::FastMutex> g(aIOMutex); std::cout << x; }

#ifdef _DEBUG
#define TRACE_DEBUG(x) { ZThread::Guard<ZThread::FastMutex> g(aIOMutex); std::cout << x; }
#else
#define TRACE_DEBUG(x)
#endif

#define TRACE_ERROR(x) { ZThread::Guard<ZThread::FastMutex> g(aIOMutex); std::cerr << x; }







#endif //  __Tracer_h

