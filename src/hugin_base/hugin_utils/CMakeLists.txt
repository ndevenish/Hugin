#this is just a basic CMakeLists.txt, for more information see the cmake manpage

# parser stuff

# set include path to include the generated parser.h file
#include_directories( ${CMAKE_SOURCE_DIR}/src/include/common )

SET(HUGIN_COMMON_SRC eig_jacobi.cpp graph.cpp PTLensDB.c utils.cpp platform.cpp)


IF (${HUGIN_SHARED_LIBS})
  add_library(hugincommon SHARED ${HUGIN_COMMON_SRC})
  set_target_properties(hugincommon VERSION ${HUGIN_LIB_VERSION})
  install(TARGETS hugincommon DESTINATION ${LIBDIR})
ELSE (${HUGIN_SHARED_LIBS})
  add_library(hugincommon STATIC ${HUGIN_COMMON_SRC})
ENDIF (${HUGIN_SHARED_LIBS})

