
include_directories( ${CMAKE_BINARY_DIR}/src )
include_directories( ${CMAKE_SOURCE_DIR}/src/hugin_base )
include_directories( ${CMAKE_SOURCE_DIR}/src/foreign )
include_directories( ${CMAKE_SOURCE_DIR}/src/foreign/vigra )

# define common sets of libraries, used by different subdirectories
IF (WIN32)
  # boost_thread is linked automatically
  # additionally link to our getopt
  set(common_libs huginbase huginjhead hugingetopt
      ${PANO_LIBRARIES} huginlevmar)
  include_directories( ${CMAKE_SOURCE_DIR}/src/foreign/getopt/include )
ELSE (WIN32)
  # need to specify boost thread library
  set(common_libs huginbase huginjhead
      boost_thread ${PANO_LIBRARIES} huginlevmar)
ENDIF (WIN32)

set(image_libs huginvigraimpex ${JPEG_LIBRARIES} ${TIFF_LIBRARIES}
    ${PNG_LIBRARIES} ${OPENEXR_LIBRARIES})

add_subdirectory(foreign)
add_subdirectory(hugin_base)
add_subdirectory(tools)
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
ENDIF(wxWidgets_FOUND)

