// -*- c-basic-offset: 4 -*-

/** @file utils.cpp
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: utils.cpp 1954 2007-04-15 20:59:42Z dangelo $
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifdef WIN32
    #include <sys/utime.h>
#else
    #include <sys/time.h>
#endif
#include <time.h>
#include <stdio.h>
#include "common/utils.h"

#ifdef unix
std::string utils::CurrentTime()
{
  char tmp[100];
  struct tm t;
  struct timeval tv;
  gettimeofday(&tv,NULL);
  localtime_r(&tv.tv_sec, &t);
  strftime(tmp,99,"%H:%M:%S",&t);
  sprintf(tmp+8,".%06ld",tv.tv_usec);
  return tmp;
}
#else
std::string utils::CurrentTime()
{
    // FIXME implement for Win & Mac
    return "";
}
#endif


std::string utils::getExtension(const std::string & basename2)
{
	std::string::size_type idx = basename2.rfind('.');
    // check if the dot is not followed by a \ or a /
    // to avoid cutting pathes.
    if (idx == std::string::npos) {
        // no dot found
		return std::string("");
    }
    // check for slashes after dot
    std::string::size_type slashidx = basename2.find('/', idx);
    std::string::size_type backslashidx = basename2.find('\\', idx);
    if ( slashidx == std::string::npos &&  backslashidx == std::string::npos)
    {
        return basename2.substr(idx+1);
    } else {
		return std::string("");
    }

}

std::string utils::stripExtension(const std::string & basename2)
{
    std::string::size_type idx = basename2.rfind('.');
    // check if the dot is not followed by a \ or a /
    // to avoid cutting pathes.
    if (idx == std::string::npos) {
        // no dot found
        return basename2;
    }
    // check for slashes after dot
    std::string::size_type slashidx = basename2.find('/', idx);
    std::string::size_type backslashidx = basename2.find('\\', idx);
    if ( slashidx == std::string::npos &&  backslashidx == std::string::npos)
    {
        return basename2.substr(0, idx);
    } else {
        return basename2;
    }
}

std::string utils::stripPath(const std::string & filename)
{
    std::string::size_type idx1 = filename.rfind('\\');
    std::string::size_type idx2 = filename.rfind('/');
    std::string::size_type idx;
    if (idx1 == std::string::npos) {
        idx = idx2;
    } else if (idx2 == std::string::npos) {
        idx = idx1;
    } else {
        idx = std::max(idx1, idx2);
    }
    if (idx != std::string::npos) {
//        DEBUG_DEBUG("returning substring: " << filename.substr(idx + 1));
        return filename.substr(idx + 1);
    } else {
        return filename;
    }
}

std::string utils::doubleToString(double d, int digits)
{
    char fmt[10];
    if (digits < 0) {
        strcpy(fmt,"%f");
    } else {
        std::sprintf(fmt,"%%.%df",digits);
    }
    char c[80];
    std::sprintf (c, fmt, d);
    std::string number (c);

    int l = (int)number.length()-1;

    while ( l != 0 && number[l] == '0' ) {
      number.erase (l);
      l--;
    }
    if ( number[l] == ',' ) {
      number.erase (l);
      l--;
    }
    if ( number[l] == '.' ) {
      number.erase (l);
      l--;
    }

    return number;
}


utils::MultiProgressDisplay::MultiProgressDisplay(double minPrintStep)
  : m_minProgressStep(minPrintStep) 
{

}

utils::StreamProgressReporter::StreamProgressReporter(double maxProgress, std::ostream & out)
 : m_progress(0), m_maxProgress(maxProgress), m_stream(out)
{

}
utils::StreamProgressReporter::~StreamProgressReporter()
{
    m_stream << "\r" << std::flush;
}

bool utils::StreamProgressReporter::increaseProgress(double delta) 
{
    m_progress += delta;
    print();
            // check for Ctrl-C ?
    return true;
}

bool utils::StreamProgressReporter::increaseProgress(double delta, const std::string & msg) 
{
    m_message = msg;
    m_progress += delta;
    print();
            // check for Ctrl-C ?
    return true;
}

void utils::StreamProgressReporter::setMessage(const std::string & msg)
{
    m_message = msg;
    print();
}

void utils::StreamProgressReporter::print()
{
    double prog = floor(m_progress/m_maxProgress*100);
    if (prog > 100) prog = 100;
    m_stream << "\r" << m_message << ": " << prog << "%" << std::flush;
}

