# add Python directories. So far I haven't put  ${PYTHON_LIBRARIES} in
# into the 'IF (MSVC)' section, because I'm not sure if it's the right
# thing to do. Of course there should be a switch to only take in the
# Python libs if hsi is wanted.

FIND_PACKAGE(PythonLibs)
INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})

# define common sets of libraries, used by different subdirectories
IF (MSVC)
  IF(LAPACK_FOUND)
    # boost_thread is linked automatically
    # additionally link to our getopt
    set(common_libs huginbase hugingetopt
        ${PANO_LIBRARIES} ${LAPACK_LIBRARIES} huginlevmar)
  ELSE(LAPACK_FOUND)
    set(common_libs huginbase hugingetopt
        ${PANO_LIBRARIES} huginlevmar)
  ENDIF(LAPACK_FOUND)
  include_directories( ${CMAKE_SOURCE_DIR}/src/foreign/getopt/include )
ELSE (MSVC)
  IF(LAPACK_FOUND)
    # need to specify boost thread library
    set(common_libs huginbase
        ${Boost_LIBRARIES} ${PANO_LIBRARIES} ${LAPACK_LIBRARIES} ${GLEW_LIBRARIES} ${PYTHON_LIBRARIES} huginlevmar)
  ELSE(LAPACK_FOUND)
    # need to specify boost thread library
    set(common_libs huginbase 
        ${Boost_LIBRARIES} ${PANO_LIBRARIES} ${GLEW_LIBRARIES} ${PYTHON_LIBRARIES} huginlevmar)
  ENDIF(LAPACK_FOUND)
ENDIF (MSVC)

set(image_libs huginvigraimpex ${OPENEXR_LIBRARIES} ${JPEG_LIBRARIES} ${TIFF_LIBRARIES}
    ${PNG_LIBRARIES} ${ZLIB_LIBRARIES} ${EXIV2_LIBRARIES})

add_subdirectory(celeste)
add_subdirectory(foreign)
add_subdirectory(hugin_base)
add_subdirectory(tools)
add_subdirectory(matchpoint)
add_subdirectory(deghosting)
add_subdirectory(lens_calibrate)


# build vips stuff, only if vips was found
# not ready for the general public yet
IF(VIPS_FOUND)

#  add_subdirectory(vips)
ENDIF(VIPS_FOUND)

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
# hsi contains the Python interface. This add_ should be conditional,
# depending on availability of SWIG and Python.
add_subdirectory(hsi)