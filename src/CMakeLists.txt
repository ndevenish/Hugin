
# define common sets of libraries, used by different subdirectories
IF (MSVC)
  # boost_thread is linked automatically
  # additionally link to our getopt
  set(common_libs huginbase huginjhead hugingetopt
      ${PANO_LIBRARIES} huginlevmar)
  include_directories( ${CMAKE_SOURCE_DIR}/src/foreign/getopt/include )
ELSE (MSVC)
  # need to specify boost thread library
  set(common_libs huginbase huginjhead
      ${Boost_thread_LIBRARIES} ${PANO_LIBRARIES} huginlevmar)
ENDIF (MSVC)

set(image_libs huginvigraimpex ${OPENEXR_LIBRARIES} ${JPEG_LIBRARIES} ${TIFF_LIBRARIES}
    ${PNG_LIBRARIES} ${ZLIB_LIBRARIES} ${EXIV2_LIBRARIES})

add_subdirectory(celeste)
add_subdirectory(foreign)
add_subdirectory(hugin_base)
add_subdirectory(tools)
add_subdirectory(matchpoint)
add_subdirectory(deghosting)

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
  add_subdirectory(PTBatcher)
  add_subdirectory(PTBatcherGUI)
ENDIF(wxWidgets_FOUND)

add_subdirectory(translations)
