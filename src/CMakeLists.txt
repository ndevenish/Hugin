
include_directories( ${CMAKE_BINARY_DIR}/src )
include_directories( ${CMAKE_SOURCE_DIR}/src/hugin_base )
include_directories( ${CMAKE_SOURCE_DIR}/src/foreign )
include_directories( ${CMAKE_SOURCE_DIR}/src/foreign/vigra )

# define common sets of libraries, used by different subdirectories
set(common_libs huginjhead huginbase
    boost_thread ${PANO13_LIBRARIES} huginlevmar)
set(image_libs huginvigraimpex ${JPEG_LIBRARIES} ${TIFF_LIBRARIES}
    ${PNG_LIBRARIES} ${OPENEXR_LIBRARIES})

add_subdirectory(foreign)
add_subdirectory(hugin_base)
add_subdirectory(tools)

# build vips stuff, only if vips was found
# not ready for the general public yet
IF(VIPS_FOUND)
#  add_subdirectory(vips)
ENDIF(VIPS_FOUND)

# build gui only if wxWidgets was found
IF(wxWidgets_FOUND)
  INCLUDE(${wxWidgets_USE_FILE})
#  add_subdirectory(hugin)
#  add_subdirectory(nona_gui)
ENDIF(wxWidgets_FOUND)

add_subdirectory(hugin1)
