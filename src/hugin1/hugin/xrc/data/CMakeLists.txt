
FILE(GLOB DATAFILES *.htm *.html *.ico *.jpg *.png *.xpm tips.txt *.mk)
INSTALL(FILES ${DATAFILES} DESTINATION ${HUGINDATADIR}/xrc/data)


configure_file(about.htm.in ${CMAKE_BINARY_DIR}/src/hugin1/hugin/xrc/data/about.htm)
INSTALL(FILES ${CMAKE_BINARY_DIR}/src/hugin1/hugin/xrc/data/about.htm DESTINATION ${HUGINDATADIR}/xrc/data)

IF (UNIX)
  INSTALL(FILES hugin.png DESTINATION ${DATADIR}/pixmaps)
ENDIF (UNIX)

# install cursor on windows
IF(WIN32)
  INSTALL(FILES cursor_cp_pick.cur DESTINATION ${HUGINDATADIR}/xrc/data/)
ENDIF(WIN32)

ADD_SUBDIRECTORY(help_en_EN)
ADD_SUBDIRECTORY(help_it_IT)
