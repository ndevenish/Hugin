// -*- c-basic-offset: 4 -*-
/** @file hugin_utils/shared_ptr.h
 *
 *  @brief wrapper around shared_ptr<>, select from std C++11 or boost
 */

/*  This is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _HUGIN_UTILS_SHAREDPTR_H
#define _HUGIN_UTILS_SHAREDPTR_H

#include "hugin_config.h"
#ifdef HAVE_CXX11
#include <memory>
namespace sharedPtrNamespace = std;
#else
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
namespace sharedPtrNamespace = boost;
#endif

#endif // _HUGIN_UTILS_SHAREDPTR_H
