
include_directories( ${CMAKE_SOURCE_DIR}/src/hugin1 )

# build gui only if wxWidgets was found
IF(wxWidgets_FOUND)
  INCLUDE(${wxWidgets_USE_FILE})

  # do we need to define _UNICODE on windows?
  IF(WIN32)
    ADD_DEFINITIONS(-D_UNICODE -DUNICODE)
  ENDIF(WIN32)

  add_subdirectory(base_wx)
  add_subdirectory(stitch_project)
  add_subdirectory(icpfind)
  add_subdirectory(executor)
  add_subdirectory(hugin)
  add_subdirectory(ptbatcher)
  add_subdirectory(calibrate_lens)
ENDIF(wxWidgets_FOUND)

