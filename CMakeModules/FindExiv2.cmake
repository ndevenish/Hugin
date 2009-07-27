# - Try to find the Exiv2 library
# Once done this will define
#
#  EXIV2_FOUND - system has libexiv2
#  EXIV2_INCLUDE_DIR - the libexiv2 include directory
#  EXIV2_LIBRARIES - Link these to use libexiv2
#  EXIV2_DEFINITIONS - Compiler switches required for using libexiv2
#

if (EXIV2_INCLUDE_DIR AND EXIV2_LIBRARIES)

  # in cache already
  SET(EXIV2_FOUND TRUE)

else (EXIV2_INCLUDE_DIR AND EXIV2_LIBRARIES)
  if (NOT WIN32)
    # use pkg-config to get the directories and then use these values
    # in the FIND_PATH() and FIND_LIBRARY() calls
    INCLUDE(UsePkgConfig)
  
    PKGCONFIG(exiv2 _EXIV2IncDir _EXIV2LinkDir _EXIV2LinkFlags _EXIV2Cflags)

    if(_EXIV2LinkFlags)
      # query pkg-config asking for a Exiv2 >= 0.12
      EXEC_PROGRAM(${PKGCONFIG_EXECUTABLE} ARGS --atleast-version=0.12 exiv2 RETURN_VALUE _return_VALUE OUTPUT_VARIABLE _pkgconfigDevNull )
      if(_return_VALUE STREQUAL "0")
        message(STATUS "Found Exiv2 release >= 0.12")
        set(EXIV2_VERSION_GOOD_FOUND TRUE)
      else(_return_VALUE STREQUAL "0")
        message(STATUS "Found Exiv2 release < 0.12")
      endif(_return_VALUE STREQUAL "0")
    else(_EXIV2LinkFlags)
        set(EXIV2_FOUND FALSE)
        set(EXIV2_VERSION_GOOD_FOUND FALSE)
        message(STATUS "Cannot find Exiv2 library!")
    endif(_EXIV2LinkFlags)

  if(EXIV2_VERSION_GOOD_FOUND)
     set(EXIV2_DEFINITIONS ${_EXIV2Cflags})
 
     FIND_PATH(EXIV2_INCLUDE_DIR exiv2/exif.hpp
       ${_EXIV2IncDir}
     )
  
     FIND_LIBRARY(EXIV2_LIBRARIES NAMES exiv2 libexiv2
       PATHS
       ${_EXIV2LinkDir}
     )
  
     if (EXIV2_INCLUDE_DIR AND EXIV2_LIBRARIES)
        set(EXIV2_FOUND TRUE)
     endif (EXIV2_INCLUDE_DIR AND EXIV2_LIBRARIES)
   endif(EXIV2_VERSION_GOOD_FOUND)
   if (EXIV2_FOUND)
      if (NOT Exiv2_FIND_QUIETLY)
       message(STATUS "Found Exiv2: ${EXIV2_LIBRARIES}")
      endif (NOT Exiv2_FIND_QUIETLY)
   else (EXIV2_FOUND)
     if (Exiv2_FIND_REQUIRED)
       if (NOT EXIV2_INCLUDE_DIR)
         message(FATAL_ERROR "Could NOT find Exiv2 header files")
       endif (NOT EXIV2_INCLUDE_DIR)
       if (NOT EXIV2_LIBRARIES)
           message(FATAL_ERROR "Could NOT find Exiv2 library")
       endif (NOT EXIV2_LIBRARIES)
     endif (Exiv2_FIND_REQUIRED)
   endif (EXIV2_FOUND)

  else(NOT WIN32)
     FIND_PATH(EXIV2_INCLUDE_DIR exiv2/exif.hpp
               /usr/local/include
               /usr/include
               ${SOURCE_BASE_DIR}/exiv2/msvc/include
               ${SOURCE_BASE_DIR}/exiv2-0.16/msvc/include
               ${SOURCE_BASE_DIR}/exiv2-0.18/msvc/include
               ${SOURCE_BASE_DIR}/exiv2-0.18.1/msvc/include
               ${SOURCE_BASE_DIR}/exiv2-0.18.2/msvc/include
              )

     include(FindLibraryWithDebug)

     find_library_with_debug(EXIV2_LIBRARIES
        WIN32_DEBUG_POSTFIX d
        NAMES exiv2 libexiv2
        PATHS /usr/lib /usr/local/lib ${SOURCE_BASE_DIR}/exiv2/msvc/lib ${SOURCE_BASE_DIR}/exiv2-0.16/msvc/lib ${SOURCE_BASE_DIR}/exiv2-0.18/msvc/lib ${SOURCE_BASE_DIR}/exiv2-0.18.1/msvc/lib ${SOURCE_BASE_DIR}/exiv2-0.18.2/msvc/lib
     )

     find_library_with_debug(EXIV2EXPAT_LIBRARIES
        WIN32_DEBUG_POSTFIX d
        NAMES libexpat
        PATHS /usr/lib /usr/local/lib ${SOURCE_BASE_DIR}/exiv2/msvc/lib ${SOURCE_BASE_DIR}/exiv2-0.16/msvc/lib ${SOURCE_BASE_DIR}/exiv2-0.18/msvc/lib ${SOURCE_BASE_DIR}/exiv2-0.18.1/msvc/lib ${SOURCE_BASE_DIR}/exiv2-0.18.2/msvc/lib
     )

     SET(EXIV2_LIBRARIES ${EXIV2_LIBRARIES} ${EXIV2EXPAT_LIBRARIES})

     include(FindPackageHandleStandardArgs)
     find_package_handle_standard_args(EXIV2 DEFAULT_MSG 
                                       EXIV2_INCLUDE_DIR EXIV2_LIBRARIES)

  endif (NOT WIN32)

  MARK_AS_ADVANCED(EXIV2_INCLUDE_DIR EXIV2_LIBRARIES)

endif (EXIV2_INCLUDE_DIR AND EXIV2_LIBRARIES)

