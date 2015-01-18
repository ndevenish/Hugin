# Check if compiler supports C++11 features 
# and which compiler switches are necessary
# CXX11_FLAG : contains the necessary compiler flag


INCLUDE(CheckCXXSourceCompiles)
INCLUDE(FindPackageHandleStandardArgs)

SET(CXX11_FLAG_CANDIDATES
  " "
  "--std=c++11"
  "--std=gnu++11"
  "--std=gnu++0x"
)

# sample openmp source code to test
SET(CXX11_TEST_SOURCE 
"
template <typename T>
struct check
{
    static_assert(sizeof(int) <= sizeof(T), \"not big enough\");
};

typedef check<check<bool>> right_angle_brackets;

class TestDeleted
{
public:
    TestDeleted() = delete;
};


int a;
decltype(a) b;

typedef check<int> check_type;
check_type c;
check_type&& cr = static_cast<check_type&&>(c);

auto d = a;

int main() {
  return 0;
};
")

# check c compiler
FOREACH(FLAG ${CXX11_FLAG_CANDIDATES})
  SET(SAFE_CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
  SET(CMAKE_REQUIRED_FLAGS "${FLAG}")
  UNSET(CXX11_FLAG_DETECTED CACHE)
  CHECK_CXX_SOURCE_COMPILES("${CXX11_TEST_SOURCE}" CXX11_FLAG_DETECTED)
  SET(CMAKE_REQUIRED_FLAGS "${SAFE_CMAKE_REQUIRED_FLAGS}")
  IF(CXX11_FLAG_DETECTED)
    SET(CXX11_FLAG "${FLAG}")
    BREAK()
  ENDIF()
ENDFOREACH()

# handle the standard arguments for find_package
FIND_PACKAGE_HANDLE_STANDARD_ARGS(CXX11Compiler DEFAULT_MSG CXX11_FLAG)

MARK_AS_ADVANCED(CXX11_FLAG)

IF(CXX11_FLAG_DETECTED)
# check if std::threads is correctly supported
SET(CXX11_THREAD_TEST_SOURCE
"
#include <thread>
#include <iostream>

void func(int x)
{
    std::cout << \"Hello from thread.\" << std::endl << \"Got value \" << x << std::endl;
};

int main()
{
    std::thread t(func, 42);
    t.join();
    return 0;
};
")

INCLUDE(CheckCXXSourceRuns)

SET(SAFE_CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
SET(CMAKE_REQUIRED_FLAGS "${CXX11_FLAG}")

CHECK_CXX_SOURCE_RUNS("${CXX11_THREAD_TEST_SOURCE}" CXX11_THREAD_TEST1)

IF(NOT CXX11_THREAD_TEST1)
  SET(SAFE_CMAKE_REQUIRED_LIBS "${CMAKE_REQUIRED_LIBRARIES}")
  FIND_PACKAGE(Threads)
  SET(CMAKE_REQUIRED_LIBRARIES "${CMAKE_THREAD_LIBS_INIT}")
  CHECK_CXX_SOURCE_RUNS("${CXX11_THREAD_TEST_SOURCE}" CXX11_THREAD_TEST2)
  IF(CXX11_THREAD_TEST2)
    SET(CXX11_THREAD_ADDITIONAL_LIB "${CMAKE_THREAD_LIBS_INIT}")
  ELSE()
    # thread library does not work
    # now try with fixed -pthread
    SET(CMAKE_REQUIRED_LIBRARIES "-pthread")
    CHECK_CXX_SOURCE_RUNS("${CXX11_THREAD_TEST_SOURCE}" CXX11_THREAD_TEST3)
    IF(CXX11_THREAD_TEST3)
      SET(CXX11_THREAD_ADDITIONAL_LIB "-pthread")
    ENDIF()
  ENDIF()
  SET(CMAKE_REQUIRED_LIBRARIES "${SAFE_CMAKE_REQUIRED_LIBS}")
ENDIF()

IF(CXX11_THREAD_TEST1 OR CXX11_THREAD_TEST2 OR CXX11_THREAD_TEST3)
  SET(CXX11_THREAD ON)
ELSE()
  UNSET(CXX11_THREAD)
ENDIF()

SET(CMAKE_REQUIRED_FLAGS "${SAFE_CMAKE_REQUIRED_FLAGS}")

ELSE()
  UNSET(CXX11_THREAD)
ENDIF()