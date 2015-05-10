// -*- c-basic-offset: 4 -*-

/** @file openmp_lock.h
*
*  @brief helper for OpenMP
*
*/
/*  This program is free software; you can redistribute it and/or
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

#ifndef OPENMP_LOCK_H
#define OPENMP_LOCK_H

#include <hugin_config.h>

namespace hugin_omp
{

#ifdef HAVE_OPENMP
#include <omp.h>

/** simple lock class */
class Lock
{
public:
    Lock()       {omp_init_lock(&m_lock);}
    ~Lock()      {omp_destroy_lock(&m_lock);}

    void Set()   {omp_set_lock(&m_lock);}
    void Unset() {omp_unset_lock(&m_lock);}
    bool Test()  {return (omp_test_lock(&m_lock) != 0);}
private:
    Lock(const Lock&);
    Lock& operator=(const Lock&);
    omp_lock_t m_lock;
};
#else
/* A dummy mutex that doesn't actually exclude anything,
* but as there is no parallelism either, no worries. */
class Lock
{
public:
    void Set() {}
    void Unset() {}
    bool Test() { return false; }
};
#endif
 
 /* An exception-safe scoped lock-keeper. */
class ScopedLock
{
public:
    ScopedLock(Lock& lock):m_lock(lock) {m_lock.Set();}
    ~ScopedLock() {m_lock.Unset();}
private:
    ScopedLock(const ScopedLock&);
    ScopedLock&operator=(const ScopedLock&);
    Lock& m_lock;
};

}; // namespace omp
#endif
