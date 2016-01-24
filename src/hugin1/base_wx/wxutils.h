// -*- c-basic-offset: 4 -*-
/** @file base_wx/wxutils.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
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
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _BASE_WX_WXUTILS_H
#define _BASE_WX_WXUTILS_H

#include <hugin_utils/utils.h>

// use trace function under windows, because usually there is
// no stdout under windows
#ifdef __WXMSW__
    #include <wx/string.h>
    #include <wx/log.h>

    #ifdef DEBUG
        #undef DEBUG_TRACE
        #undef DEBUG_DEBUG
        #undef DEBUG_INFO
        #undef DEBUG_NOTICE

        // debug trace
//      #define DEBUG_TRACE(msg) { std::stringstream o; o << "TRACE " << DEBUG_HEADER << msg; wxLogDebug(o.str().c_str());}
        #define DEBUG_TRACE(msg) { std::cerr << "TRACE " << DEBUG_HEADER << msg << std::endl; }
        // low level debug info
//      #define DEBUG_DEBUG(msg) { std::stringstream o; o << "DEBUG " << DEBUG_HEADER << msg; wxLogDebug(o.str().c_str()); }
        #define DEBUG_DEBUG(msg) { std::cerr << "DEBUG " << DEBUG_HEADER << msg << std::endl; }
        // informational debug message,
//      #define DEBUG_INFO(msg) { std::stringstream o; o << "INFO " << DEBUG_HEADER << msg; wxLogDebug(o.str().c_str()); }
        #define DEBUG_INFO(msg) { std::cerr << "INFO " << DEBUG_HEADER << msg << std::endl; }
        // major change/operation should use this
//      #define DEBUG_NOTICE(msg) { std::stringstream o; o << "NOTICE " << DEBUG_HEADER << msg; wxLogMessage(o.str().c_str()); }
        #define DEBUG_NOTICE(msg) { std::cerr << "NOTICE " << DEBUG_HEADER << msg << std::endl; }
    #endif
    
    #undef DEBUG_WARN
    #undef DEBUG_ERROR
    #undef DEBUG_FATAL
    #undef DEBUG_ASSERT

    // when an error occurred, but can be handled by the same function
    #define DEBUG_WARN(msg) { std::stringstream o; o << "WARN: " << DEBUG_HEADER << msg; wxLogWarning(wxString(o.str().c_str(), wxConvISO8859_1));}
    // an error occurred, might be handled by a calling function
    #define DEBUG_ERROR(msg) { std::stringstream o; o << "ERROR: " << DEBUG_HEADER << msg; wxLogError(wxString(o.str().c_str(),wxConvISO8859_1));}
    // a fatal error occurred. further program execution is unlikely
    #define DEBUG_FATAL(msg) { std::stringstream o; o << "FATAL: " << DEBUG_HEADER << "(): " << msg; wxLogError(wxString(o.str().c_str(),wxConvISO8859_1)); }
    // assertion
    #define DEBUG_ASSERT(cond) \
        do { \
            if (!(cond)) { \
                std::stringstream o; o << "ASSERTATION: " << DEBUG_HEADER << "(): " << #cond; \
                    wxLogFatalError(wxString(o.str().c_str(),wxConvISO8859_1)); \
            } \
        } while(0)
#endif


#endif // _BASE_WX_WXUTILS_H
