// -*- c-basic-offset: 4 -*-
/** @file hugin_utils/utils.h
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

#ifndef _HUGIN_UTILS_UTILS_H
#define _HUGIN_UTILS_UTILS_H

#include <hugin_shared.h>
#include <string.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <cassert>

#include <hugin_utils/platform.h>

#if 0
// ??????????????????????????????????????????
#ifdef __WXMSW__
    // has to be included before!
    #include <wx/log.h>
#endif

#ifdef WIN32
    #define snprintf _snprintf
#endif

#ifdef __WXMSW__
    // has to be included before!
    #include <wx/log.h>
    #define snprintf _snprintf
#endif
// ??????????????????????????????????????????
#endif


// misc utility functions / macros

// remark:
// on unix_like systems don't use CurrentTime, this is defined as a macro in X.h and breaks the debug messages
// on windows we can't use GetCurrentTime because this is replaced with GetTickCount

#ifdef __GNUC__
    // the full function name is too long..
//  #define DEBUG_HEADER hugin_utils::CurrentTime() <<" (" << __FILE__ << ":" << __LINE__ << ") "  << __PRETTY_FUNCTION__ << "()" << std::endl << "    "
    #define DEBUG_HEADER hugin_utils::GetCurrentTimeString() <<" (" << __FILE__ << ":" << __LINE__ << ") "  << __func__ << "(): "
#elif _MSC_VER > 1300
    #define DEBUG_HEADER hugin_utils::GetCurrentTimeString() <<" (" << __FILE__ << ":" << __LINE__ << ") "  << __FUNCTION__ << "(): "
#else
    #define DEBUG_HEADER hugin_utils::GetCurrentTimeString() <<" (" << __FILE__ << ":" << __LINE__ << ") "  << __func__ << "(): "
#endif


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
// C-style assertion
#define DEBUG_ASSERT(cond) assert(cond)


// use trace function under windows, because usually there is
// no stdout under windows
#ifdef __WXMSW__
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

    // when an error occured, but can be handled by the same function
    #define DEBUG_WARN(msg) { std::stringstream o; o << "WARN: " << DEBUG_HEADER << msg; wxLogWarning(wxString(o.str().c_str(), wxConvISO8859_1));}
    // an error occured, might be handled by a calling function
    #define DEBUG_ERROR(msg) { std::stringstream o; o << "ERROR: " << DEBUG_HEADER << msg; wxLogError(wxString(o.str().c_str(),wxConvISO8859_1));}
    // a fatal error occured. further program execution is unlikely
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

//
#define UTILS_THROW(class, msg)  { std::stringstream o; o <<  msg; throw(class(o.str().c_str())); };



namespace hugin_utils
{
    
    /** current time as a string */
    IMPEX std::string GetCurrentTimeString();

    /** convert a double to a string, suitable for display
     *  within a GUI.
     *
     *  @p d value to convert t a string
     *  @p fractionaldigits number of fractional digits.
     *     -1: not specified, use default.
     */
    IMPEX std::string doubleToString(double d, int fractionaldigits=-1);

    /** convert a string to a double, ignore localisation.
     *  always accept both.
     *
     *  sets \p dest to the new value, and returns true
     *  if it could be read.
     *
     *  it the conversion fails, returns false and does not
     *  modify \p dest.
     *
     *  @return success
     */
    template <typename STR>
    bool stringToDouble(const STR & str_, double & dest)
    {
        double res=0;
        // set numeric locale to C, for correct number output
        char * old_locale = setlocale(LC_NUMERIC,NULL);
        old_locale = strdup(old_locale);
        setlocale(LC_NUMERIC,"C");

        STR str(str_);
        // replace all kommas with points, independant of the locale..
        for (typename STR::iterator it = str.begin(); it != str.end(); ++it) {
            if (*it == ',') {
                *it = '.';
            }
        }

        const char * p = str.c_str();
        char * pe=0;
        res = strtod(p,&pe);

        // reset locale
        setlocale(LC_NUMERIC,old_locale);
        free(old_locale);

        if (pe == p) {
            // conversion failed.
            DEBUG_DEBUG("conversion failed: " << str << " to:" << dest);
            return false;
        } else {
            // conversion ok.
            dest = res;
    //        DEBUG_DEBUG("converted: " << str << " to:" << dest);
            return true;
        }
    }

    /** Get the path to a filename */
    IMPEX std::string getPathPrefix(const std::string & filename);

    /** Get extension of a filename */
    IMPEX std::string getExtension(const std::string & basename);

    /** remove the path of a filename (mainly useful for gui
     *  display of filenames)
     */
    IMPEX std::string stripPath(const std::string & filename);

    /** remove extension of a filename */
    IMPEX std::string stripExtension(const std::string & basename);

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


    template <class str>
    str QuoteStringInternal(const str & arg, const str & quotechar,
                            const str & replacements)
    {
        // loop over all chars..
        str ret(arg);
        size_t len = replacements.size();
        for (size_t i = 0; i < len; i++) {
            str source(replacements.substr(i,1));
            str dest(quotechar + source);
            size_t idx = 0;
            do {
                idx = ret.find(source,idx);
                if (idx != str::npos) {
                    ret.replace(idx, 1, dest);
                    // skip to next unknown char.
                    idx += 2;
                }
            } while (idx != str::npos);
        }
        return ret;
    }
    
    ///
    template <class str>
    str replaceAll(const str& arg, const str& target, const str& replacement)
    {
        str ret(arg);
        typename str::size_type pos = ret.find(target, 0);
        
        for ( typename str::size_type n = 0 ;  pos != str::npos ;  pos = ret.find(target, n) )
        {
            ret.replace(pos, target.size(), replacement);
            n = pos + replacement.size();
        }
        
        return ret;
    }
    
    IMPEX void ControlPointErrorColour(const double cperr, 
        double &r,double &g, double &b);

    /** checks if file exists */
    IMPEX bool FileExists(const std::string filename);

    /** returns the full absolute filename */
    IMPEX std::string GetAbsoluteFilename(const std::string filename);
} // namespace


#endif // _HUGIN_UTILS_UTILS_H
