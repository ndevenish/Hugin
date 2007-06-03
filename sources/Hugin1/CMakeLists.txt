
include_directories( ${CMAKE_BINARY_DIR}/src/include )
include_directories( ${CMAKE_SOURCE_DIR}/src/include )
include_directories( ${CMAKE_SOURCE_DIR}/src/foreign )

# define common sets of libraries, used by different subdirectories
set(common_libs huginjhead hugincommon huginpanorama
    huginvigra_ext boost_thread ${PANO13_LIBRARIES} huginlevmar)
set(image_libs huginvigraimpex ${JPEG_LIBRARIES} ${TIFF_LIBRARIES}
    ${PNG_LIBRARIES} ${OPENEXR_LIBRARIES})

add_subdirectory(jhead)
add_subdirectory(common)
add_subdirectory(foreign)
add_subdirectory(vigra_ext)
add_subdirectory(Panorama)
add_subdirectory(tools)

# build vips stuff, only if vips was found
# not ready for the general public yet
#IF(VIPSCC_FOUND)
#  add_subdirectory(tools_vips)
#ENDIF(VIPSCC_FOUND)

# build gui only if wxWidgets was found
IF(wxWidgets_FOUND)
  INCLUDE(${wxWidgets_USE_FILE})
  add_subdirectory(hugin)
  add_subdirectory(nona_gui)
ENDIF(wxWidgets_FOUND)

#add_subdirectory(tests)
