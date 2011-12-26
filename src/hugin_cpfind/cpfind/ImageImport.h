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

#ifndef __imageimport_h
#define __imageimport_h

#ifdef WIN32
#define NOMINMAX
#include "vigra/windows.h"
#endif

#ifdef MACOS_X // only XCode project
#include <AnaImageFramework/vigra/stdimage.hxx>
#include <AnaImageFramework/vigra/stdimagefunctions.hxx>
#include <AnaImageFramework/vigra/impex.hxx>
#include <AnaImageFramework/vigra/rgbvalue.hxx>
#else
#include "vigra/stdimage.hxx"
#include "vigra/stdimagefunctions.hxx"
#include "vigra/impex.hxx"
#include "vigra/rgbvalue.hxx"
//#include "vigra/ext/impexalpha.hxx"
#endif

#endif
