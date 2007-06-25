#this is just a basic CMakeLists.txt, for more information see the cmake manpage

# parser stuff

# set include path to include the generated parser.h file
#include_directories( ${CMAKE_SOURCE_DIR}/src/include/common )


#build a shared library
add_library(hugincommon SHARED eig_jacobi.cpp graph.cpp PTLensDB.c utils.cpp)

install(TARGETS hugincommon DESTINATION lib)