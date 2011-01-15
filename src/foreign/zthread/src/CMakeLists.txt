
include_directories(../include)

add_definitions(-DNDEBUG)

add_library(ZThread STATIC
AtomicCount.cxx 
Condition.cxx 
ConcurrentExecutor.cxx 
CountingSemaphore.cxx 
FastMutex.cxx 
FastRecursiveMutex.cxx 
Mutex.cxx 
RecursiveMutexImpl.cxx 
RecursiveMutex.cxx 
Monitor.cxx 
PoolExecutor.cxx 
PriorityCondition.cxx 
PriorityInheritanceMutex.cxx 
PriorityMutex.cxx 
PrioritySemaphore.cxx 
Semaphore.cxx 
SynchronousExecutor.cxx 
Thread.cxx 
ThreadedExecutor.cxx 
ThreadImpl.cxx 
ThreadLocalImpl.cxx 
ThreadQueue.cxx 
Time.cxx 
ThreadOps.cxx )

