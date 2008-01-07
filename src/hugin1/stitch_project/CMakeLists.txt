

IF (WIN32)
 add_executable(hugin_stitch_project WIN32 hugin_stitch_project.cpp hugin_stitch_project_rc.rc)
ELSE (WIN32)
 add_executable(hugin_stitch_project hugin_stitch_project.cpp)
ENDIF (WIN32)


target_link_libraries(hugin_stitch_project ${common_libs} ${image_libs} huginbasewx ${wxWidgets_LIBRARIES})

install(TARGETS hugin_stitch_project
        DESTINATION ${BINDIR})


IF (UNIX)
  INSTALL(FILES hugin_stitch_project.desktop DESTINATION ${DATADIR}/applications)
ENDIF (UNIX)
