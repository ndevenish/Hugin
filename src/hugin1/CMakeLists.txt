
include_directories( ${CMAKE_SOURCE_DIR}/src/hugin1 )

# build gui only if wxWidgets was found
IF(wxWidgets_FOUND)
#  INCLUDE(${wxWidgets_USE_FILE})
  add_subdirectory(base_wx)
  add_subdirectory(stitch_project)
  add_subdirectory(hugin)
  add_subdirectory(nona_gui)
ENDIF(wxWidgets_FOUND)

IF(VIPS_FOUND)
#  add_subdirectory(tools_vips)
ENDIF(VIPS_FOUND)

#add_subdirectory(tools)
#add_subdirectory(tests)
