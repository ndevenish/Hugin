# This CMakeList.txt is needed to make The Python 
# scripting interface. The CMake code has been taken from
# http://www.swig.org/Doc1.3/Introduction.html#Introduction
# section 2.6, the only modification apart from adapting the
# names was the inclusion of ${common_libs}

FIND_PACKAGE(SWIG REQUIRED)
INCLUDE(${SWIG_USE_FILE})

FIND_PACKAGE(PythonLibs)
INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

SET(CMAKE_SWIG_FLAGS "")

SET_SOURCE_FILES_PROPERTIES(hsi.i PROPERTIES CPLUSPLUS ON)
SWIG_ADD_MODULE(hsi python hsi.i hsi.cpp)
SWIG_LINK_LIBRARIES(hsi ${PYTHON_LIBRARIES} ${common_libs})

# TODO: I haven' found a way to have Cmake make swigpyrun.h
# TODO: figure out how to only compile hsi.i if SWIG is available


# this would make a standalone program 'hsi', if #define standalone
# is set in hpi.cpp (that would be for testing purposes, if linking
# to hugin isn't an option):

# add_executable(hpi hpi.cpp)
# target_link_libraries(hpi ${PYTHON_LIBRARIES} ${common_libs} )
