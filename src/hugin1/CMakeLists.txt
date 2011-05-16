
include_directories( ${CMAKE_SOURCE_DIR}/src/hugin1 )

# build gui only if wxWidgets was found
IF(wxWidgets_FOUND)
#  INCLUDE(${wxWidgets_USE_FILE})

  # do we need to define _UNICODE on windows?
  IF(WIN32)
    IF ("${wxWidgets_CONFIGURATION}" STREQUAL "mswu")
      ADD_DEFINITIONS("-D_UNICODE")
    ENDIF("${wxWidgets_CONFIGURATION}" STREQUAL "mswu")
  ENDIF(WIN32)

  add_subdirectory(base_wx)
  add_subdirectory(stitch_project)
  add_subdirectory(icpfind)
  # build  Hugin only if OpenGL was found
  IF(OPENGL_FOUND)
    add_subdirectory(hugin)
  ENDIF(OPENGL_FOUND)  
  add_subdirectory(nona_gui)
  add_subdirectory(ptbatcher)
  add_subdirectory(calibrate_lens)
ENDIF(wxWidgets_FOUND)

IF(VIPS_FOUND)
#  add_subdirectory(tools_vips)
ENDIF(VIPS_FOUND)

