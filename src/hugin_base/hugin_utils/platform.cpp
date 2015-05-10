// -*- c-basic-offset: 4 -*-

/** @file hugin_utils/platform.cpp
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: utils.cpp 2570 2007-12-18 16:53:54Z dangelo $
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
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "platform.h"


#ifdef _WIN32
#include "vigra/windows.h"
#elif defined(sun) || defined(__sun) || defined(__sun__)
#include <unistd.h>
#else
#include <unistd.h>
#include <sys/sysctl.h>
#endif

namespace hugin_utils {
    
#ifdef _WIN32

int getCPUCount()
{
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return si.dwNumberOfProcessors;
}

#else

int getCPUCount()
{
    #ifdef HW_NCPU
        // BSD and OSX like system
        int mib[2];
        int numCPUs = 1;
        size_t len = sizeof(numCPUs);

        mib[0] = CTL_HW;
        mib[1] = HW_NCPU;
        sysctl(mib, 2, &numCPUs, &len, 0, 0);
        return numCPUs;
        
    #elif defined(_SC_NPROCESSORS_ONLN)
        // Linux and Solaris
        long nProcessorsOnline = sysconf(_SC_NPROCESSORS_ONLN);
        return nProcessorsOnline;
    #else
        #warning sysconf or sysctl does not support quering the number of processors/cores.
        return -1;
    #endif
}

#endif

} //namespace
