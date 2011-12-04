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

#include "Utils.h"
#if defined(HW_NCPU) || defined(__APPLE__)
#include <sys/sysctl.h>
#endif
#ifdef _WINDOWS
#include <Windows.h>
#elif defined __APPLE__
#include <CoreServices/CoreServices.h>  //for gestalt
#else
#include <unistd.h>
#endif

int utils::getCPUCount()
{
#ifdef WIN32
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return si.dwNumberOfProcessors;
#elif defined(HW_NCPU) || defined(__APPLE__)
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
    return 1;
#endif
}

#ifdef _WINDOWS
unsigned long long utils::getTotalMemory()
{
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatusEx(&status);
    return status.ullTotalPhys;
};
#elif defined __APPLE__
unsigned long long utils::getTotalMemory()
{
    long ramSize;
    if(Gestalt(gestaltPhysicalRAMSizeInMegabytes, &ramSize)==noErr)
    {
        return ramSize * 1024 * 1024;
    }
    else
    {
        // if query was not successful return 1 GB, 
        // return 0 would result in crash in calling function
        return 1024*1024*1024;
    }
};
#else
unsigned long long utils::getTotalMemory()
{
    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    return pages * page_size;
}
#endif
