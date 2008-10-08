# Try to find the glew libraries, setting these defines:
#  GLEW_FOUND - system has glew
#  GLEW_INCLUDE_DIR - glew include directory
#  GLEW_LIBRARIES - Libraries needed to use glew

FIND_PATH(GLEW_INCLUDE_DIR GL/glew.h PATHS /usr/include /usr/local/include)
  
FIND_LIBRARY(GLEW_LIBRARIES GLEW PATHS /usr/lib /usr/local/lib)
IF (GLEW_INCLUDE_DIR AND GLEW_LIBRARIES)
   SET(GLEW_FOUND TRUE)
ENDIF (GLEW_INCLUDE_DIR AND GLEW_LIBRARIES)


IF (GLEW_FOUND)
   IF (NOT GLEW_FIND_QUIETLY)
      MESSAGE(STATUS "Found Glew: ${FOO_LIBRARY}")
   ENDIF (NOT GLEW_FIND_QUIETLY)
ELSE (GLEW_FOUND)
   IF (GLEW_FIND_REQUIRED)
      MESSAGE(FATAL_ERROR "Could not find Glew, install it with your package manager, or get it from http://glew.sourceforge.net/.")
   ENDIF (GLEW_FIND_REQUIRED)
ENDIF (GLEW_FOUND)

