// -*- c-basic-offset: 4 -*-
/** @file utils.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
 *
 *  This is free software; you can redistribute it and/or
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

#ifndef _UTILS_H
#define _UTILS_H

#include <string>
#include <iostream>

// misc utility functions / macros

#ifdef __GNUC__
// the full function name is too long..
//#define DEBUG_HEADER utils::CurrentTime() << " (" << __FILE__ << ":" << __LINE__ << ") " << __PRETTY_FUNCTION__ << "()" << std::endl << "    "
#define DEBUG_HEADER utils::CurrentTime() <<" (" << __FILE__ << ":" << __LINE__ << ") "  << __func__ << "(): "
#else
#define DEBUG_HEADER utils::CurrentTime() <<" (" << __FILE__ << ":" << __LINE__ << ") "  << __func__ << "(): "
#endif

// debug trace
#define DEBUG_TRACE(msg) { std::cerr << "TRACE " << DEBUG_HEADER << msg << std::endl; }
// low level debug info
#define DEBUG_DEBUG(msg) { std::cerr << "DEBUG " << DEBUG_HEADER << msg << std::endl; }
// informational debug message,
#define DEBUG_INFO(msg) { std::cerr << "INFO " << DEBUG_HEADER << msg << std::endl; }
// major change/operation should use this
#define DEBUG_NOTICE(msg) { std::cerr << "NOTICE " << DEBUG_HEADER << msg << std::endl; }
// when an error occured, but can be handled by the same function
#define DEBUG_WARN(msg) { std::cerr << "WARN: " << DEBUG_HEADER << msg << std::endl; }
// an error occured, might be handled by a calling function
#define DEBUG_ERROR(msg) { std::cerr << "ERROR: " << DEBUG_HEADER << msg << std::endl; }
// a fatal error occured. further program execution is unlikely
#define DEBUG_FATAL(msg) { std::cerr << "FATAL: " << DEBUG_HEADER << "(): " << msg << std::endl; }

namespace utils
{
    std::string CurrentTime();
} // namespace


#endif // _UTILS_H
