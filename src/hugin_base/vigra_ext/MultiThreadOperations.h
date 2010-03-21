// -*- c-basic-offset: 4 -*-
/** @file MultithreadOperations.h
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

#ifndef _MULTITHREADOPERATIONS_H
#define _MULTITHREADOPERATIONS_H

#include <hugin_shared.h>
#include <hugin_utils/utils.h>

#include <vigra/windows.h>
#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include <vigra/utilities.hxx>

namespace vigra_ext
{

/** Thread "manager". currently only used to query the number of threads that
 *  should be used for image processing operations
 */
class IMPEX ThreadManager
{

public:
    ThreadManager()
    {
        m_nThreads = 1;
    }

	virtual ~ThreadManager()
	{}

    static ThreadManager & get()
    {
        if (!m_instance) {
            m_instance = new ThreadManager();
        } 
        return *m_instance;
    }

    unsigned getNThreads() {
        return m_nThreads;
    }

    void setNThreads(unsigned n)
    {
        m_nThreads = n;
    }
private:
    unsigned m_nThreads;
    static ThreadManager * m_instance;
};

/** operation to do multithreaded image processing operations
 *
 *  Function @p op should be a function that transforms one source image
 *  to a destination image. Function @p op should tale @p src and @p dest
 *  as arguments.
 * 
 *  If it requires more arguments, boost::bind should be used to create
 *  a functor for it.
 *
 *  Note that the @p op has to take @p src and @p dest by value and not
 *  by reference, since they are not 
 *
 */

template <class SrcIter, class SrcAcc, class DestIter, class DestAcc, class Function>
void multithreadOperation (vigra::triple<SrcIter, SrcIter, SrcAcc> src,
                           vigra::pair<DestIter, DestAcc> dest,
                           Function op)
{
    // divide output image into multiple areas
    unsigned nThreads = ThreadManager::get().getNThreads();

    vigra::Diff2D srcSize = src.second - src.first;
    // limit amount of threads if only a few lines are given.
    if (srcSize.y < (int) nThreads) {
        nThreads = srcSize.y;
    }

    // just our thread
    if (nThreads == 1)
    {
        op(src,dest);
    }

    DEBUG_DEBUG("creating " << nThreads << " threads for transform");

    unsigned int chunkSize = srcSize.y / nThreads;
//    unsigned int lastChunkSize = srcSize.y - (nThreads-1) * chunkSize;

    // create threads to remap each area
    boost::thread_group threads;

    vigra::triple<SrcIter, SrcIter, SrcAcc> srcCurr(src);
    vigra::pair<DestIter, DestAcc> destCurr(dest);

    srcCurr.second.y -= srcSize.y - chunkSize;

    unsigned int i;
    for (i = 0; i < nThreads-1; ++i) {
    	DEBUG_DEBUG("Starting thread " << i);
        threads.create_thread(boost::bind(op, srcCurr, destCurr));

        srcCurr.first.y += chunkSize;
        srcCurr.second.y += chunkSize;
        destCurr.first.y += chunkSize;
    }
    // last chunk
    srcCurr.second = src.second;
    // remap last chunk in current thread.
    op(src,dest);

    DEBUG_DEBUG("Waiting for threads to join");
    threads.join_all();
    DEBUG_DEBUG("Threads joined");
}


} // namespace

#endif // _MULTITHREADOPERATIONS_H
