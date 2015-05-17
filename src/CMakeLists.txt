
# define common sets of libraries, used by different subdirectories
IF (MSVC)
  SET(common_libs huginbase hugingetopt ${PANO_LIBRARIES} huginlevmar)
  include_directories( ${CMAKE_SOURCE_DIR}/src/foreign/getopt/include )
ELSE (MSVC)
  # need to specify boost thread library
  SET(common_libs huginbase ${Boost_LIBRARIES} ${PANO_LIBRARIES} ${GLEW_LIBRARIES} huginlevmar ${CXX11_THREAD_ADDITIONAL_LIB})
ENDIF (MSVC)
IF(LAPACK_FOUND)
  SET(common_libs ${common_libs} ${LAPACK_LIBRARIES})
ENDIF()
IF(FFTW_FOUND)
  SET(common_libs ${common_libs} ${FFTW_LIBRARIES})
ENDIF()

set(image_libs ${VIGRA_LIBRARIES} ${OPENEXR_LIBRARIES} ${JPEG_LIBRARIES} ${TIFF_LIBRARIES}
    ${PNG_LIBRARIES} ${ZLIB_LIBRARIES} ${EXIV2_LIBRARIES})

SET(common_libs ${common_libs} ${image_libs} ${LCMS2_LIBRARIES})

add_subdirectory(celeste)
add_subdirectory(foreign)
add_subdirectory(hugin_base)
add_subdirectory(tools)
add_subdirectory(deghosting)

# build vips stuff, only if vips was found
# not ready for the general public yet
IF(VIPS_FOUND)
#  add_subdirectory(vips)
ENDIF(VIPS_FOUND)

# hsi contains the Python interface. This add_ should be conditional,
# depending on availability of SWIG and Python.
# this have to come before hugin1 directory
IF(BUILD_HSI)
add_subdirectory(hugin_script_interface)
ENDIF()

# build gui only if wxWidgets was found
IF(wxWidgets_FOUND)
  INCLUDE(${wxWidgets_USE_FILE})
  add_subdirectory(hugin1)

  include_directories( ${CMAKE_SOURCE_DIR}/src/hugin1 )
  # do we need to define _UNICODE on windows?
  IF(WIN32)
    IF ("${wxWidgets_CONFIGURATION}" STREQUAL "mswu")
      ADD_DEFINITIONS("-D_UNICODE")
    ENDIF("${wxWidgets_CONFIGURATION}" STREQUAL "mswu")
  ENDIF(WIN32)
ENDIF(wxWidgets_FOUND)

add_subdirectory(hugin_cpfind)
add_subdirectory(translations)
