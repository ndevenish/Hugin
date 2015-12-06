# Try to find the glew libraries, setting these defines:
#  GLEW_FOUND - system has glew
#  GLEW_INCLUDE_DIR - glew include directory
#  GLEW_LIBRARIES - Libraries needed to use glew

IF(WIN32)
  FIND_PATH(GLEW_INCLUDE_DIR GL/glew.h PATHS ${SOURCE_BASE_DIR}/glew/include)
  include(FindLibraryWithDebug)
  # for dynamic build, it's glew32.lib and the dll must be copied into hugin's bin folder
  IF(${HUGIN_SHARED})
    find_library_with_debug(GLEW_LIBRARIES 
      WIN32_DEBUG_POSTFIX d
	  NAMES glew32 libglew32.dll
	  PATHS
	    ${SOURCE_BASE_DIR}/glew/lib
	)
  ELSE(${HUGIN_SHARED})
    find_library_with_debug(GLEW_LIBRARIES 
      WIN32_DEBUG_POSTFIX d
      NAMES glew32s libglew32
      PATHS
        ${SOURCE_BASE_DIR}/glew/lib
  )
  ENDIF(${HUGIN_SHARED})
ELSE(WIN32)
  FIND_PATH(GLEW_INCLUDE_DIR GL/glew.h PATHS /usr/include /usr/local/include)
  FIND_LIBRARY(GLEW_LIBRARIES GLEW PATHS ${SYSTEM_LIB_DIRS})
ENDIF(WIN32)

IF (GLEW_INCLUDE_DIR AND GLEW_LIBRARIES)
   SET(GLEW_FOUND TRUE)
ENDIF (GLEW_INCLUDE_DIR AND GLEW_LIBRARIES)

IF (GLEW_FOUND)
   IF (NOT GLEW_FIND_QUIETLY)
      MESSAGE(STATUS "Found Glew: ${GLEW_LIBRARIES}")
   ENDIF (NOT GLEW_FIND_QUIETLY)
ELSE (GLEW_FOUND)
   IF (GLEW_FIND_REQUIRED)
      MESSAGE(FATAL_ERROR "Could not find Glew, install it with your package manager, or get it from http://glew.sourceforge.net/.")
   ENDIF (GLEW_FIND_REQUIRED)
ENDIF (GLEW_FOUND)
