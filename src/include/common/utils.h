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
#include <sstream>
#include <cassert>

#if __WXMSW__
#include <wx/log.h>
#endif

// misc utility functions / macros

#ifdef __GNUC__
// the full function name is too long..
//#define DEBUG_HEADER utils::CurrentTime() << " (" << __FILE__ << ":" << __LINE__ << ") " << __PRETTY_FUNCTION__ << "()" << std::endl << "    "
#define DEBUG_HEADER utils::CurrentTime() <<" (" << __FILE__ << ":" << __LINE__ << ") "  << __func__ << "(): "
#else
#define DEBUG_HEADER utils::CurrentTime() <<" (" << __FILE__ << ":" << __LINE__ << ") "  << __func__ << "(): "
#endif

// use trace function under windows, because usually there is
// no stdout under windows
//#ifdef __WXMSW__
#if __WXMSW__
 #ifdef DEBUG
  // debug trace
    #define DEBUG_TRACE(msg) { std::stringstream o; o << "TRACE " << DEBUG_HEADER << msg; wxLogDebug(o.str().c_str());}
  // low level debug info
  #define DEBUG_DEBUG(msg) { std::stringstream o; o << "DEBUG " << DEBUG_HEADER << msg; wxLogDebug(o.str().c_str()); }
  // informational debug message,
  #define DEBUG_INFO(msg) { std::stringstream o; o << "INFO " << DEBUG_HEADER << msg; wxLogDebug(o.str().c_str()); }
  // major change/operation should use this
  #define DEBUG_NOTICE(msg) { std::stringstream o; o << "NOTICE " << DEBUG_HEADER << msg; wxLogMessage(o.str().c_str()); }
 #else
  #define DEBUG_TRACE(msg)
  #define DEBUG_DEBUG(msg)
  #define DEBUG_INFO(msg)
  #define DEBUG_NOTICE(msg)
 #endif

 // when an error occured, but can be handled by the same function
 #define DEBUG_WARN(msg) { std::stringstream o; o << "WARN: " << DEBUG_HEADER << msg; wxLogWarning(o.str().c_str());}
 // an error occured, might be handled by a calling function
 #define DEBUG_ERROR(msg) { std::stringstream o; o << "ERROR: " << DEBUG_HEADER << msg; wxLogError(o.str().c_str());}
// a fatal error occured. further program execution is unlikely
 #define DEBUG_FATAL(msg) { std::stringstream o; o << "FATAL: " << DEBUG_HEADER << "(): " << msg; wxLogError(o.str().c_str()); }
 #define DEBUG_ASSERT(cond) \
 do { \
     if (!(cond)) { \
         std::stringstream o; o << "ASSERTATION: " << DEBUG_HEADER << "(): " << #cond; \
         wxLogFatalError(o.str().c_str()); \
    } \
 } while(0)

#else

 #ifdef DEBUG
  // debug trace
  #define DEBUG_TRACE(msg) { std::cerr << "TRACE " << DEBUG_HEADER << msg << std::endl; }
  // low level debug info
  #define DEBUG_DEBUG(msg) { std::cerr << "DEBUG " << DEBUG_HEADER << msg << std::endl; }
  // informational debug message,
  #define DEBUG_INFO(msg) { std::cerr << "INFO " << DEBUG_HEADER << msg << std::endl; }
  // major change/operation should use this
  #define DEBUG_NOTICE(msg) { std::cerr << "NOTICE " << DEBUG_HEADER << msg << std::endl; }
 #else
  #define DEBUG_TRACE(msg)
  #define DEBUG_DEBUG(msg)
  #define DEBUG_INFO(msg)
  #define DEBUG_NOTICE(msg)
 #endif

 // when an error occured, but can be handled by the same function
 #define DEBUG_WARN(msg) { std::cerr << "WARN: " << DEBUG_HEADER << msg << std::endl; }
 // an error occured, might be handled by a calling function
 #define DEBUG_ERROR(msg) { std::cerr << "ERROR: " << DEBUG_HEADER << msg << std::endl; }
 // a fatal error occured. further program execution is unlikely
 #define DEBUG_FATAL(msg) { std::cerr << "FATAL: " << DEBUG_HEADER << "(): " << msg << std::endl; }

 #define DEBUG_ASSERT(cond) assert(cond)

#endif


namespace utils
{
    std::string CurrentTime();

    std::string doubleToString(double d);

    template <typename Target, typename Source>
    Target lexical_cast(Source arg) {

        std::stringstream interpreter;

        Target result;

        if (!(interpreter << arg) ||
            !(interpreter >> result) ||
            !(interpreter >> std::ws).eof()) {

            DEBUG_ERROR("lexical cast error");
            // cast error.  handle it somehow
            // boost guys throw an exception here
        };

        return result;

    }; // lexical cast

    /** The progress display is used to report progress to another
     *  part of the program.
     *
     *  This enables the utility classes to report progress both to
     *  the statusbar if there is one, or a textmode, for applications
     *  without GUI
     */
    class ProgressDisplay
    {
    public:
        virtual ~ProgressDisplay() {};
        /** receive notification about progress
         *
         *  @param msg message text
         *  @param progress optional progress indicator (0-100%)
         */
        virtual void progressMessage(const std::string & msg,
                                     int progress=-1) = 0;
    };

    // print progress to cout.
    class CoutProgressDisplay : public ProgressDisplay
    {
    public:
        virtual ~CoutProgressDisplay() {};

        /** receive notification about progress
         *
         *  @param msg message text
         *  @param progress optional progress indicator (0-100%)
         */
        virtual void progressMessage(const std::string & msg, int progress=-1)
            {
                if (msg == last_msg && progress != -1) {
                    // just print the progress
                    if (progress != -1) {
                        std::cout << "\r" << msg << ": "
                                  << progress << "%" << std::flush;
                    }
                } else {
                    if (progress != -1) {
                        std::cout << std::endl << msg << ": " << progress << "%" << std::flush;
                    } else {
                        std::cout << std::endl << msg << std::flush;
                    }
                    last_msg = msg;
                }
            }

    private:
        std::string last_msg;
    };

} // namespace


#endif // _UTILS_H
