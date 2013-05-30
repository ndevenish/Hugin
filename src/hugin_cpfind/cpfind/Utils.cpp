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

#ifdef _WINDOWS
unsigned long long utils::getTotalMemory()
{
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatusEx(&status);
#ifndef _WIN64
    // when compiled as 32 bit version, we can only use about 2 GB
    // even if we have more memory available on a 64 bit system
    return std::min<unsigned long long>(status.ullTotalPhys, 1500*1024*1024);
#else
    return status.ullTotalPhys;
#endif
};
#elif defined __APPLE__
unsigned long long utils::getTotalMemory()
{
    SInt32 ramSize;
    if(Gestalt(gestaltPhysicalRAMSizeInMegabytes, &ramSize)==noErr)
    {
        unsigned long long _ramSize = ramSize;
        return _ramSize * 1024 * 1024;
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
