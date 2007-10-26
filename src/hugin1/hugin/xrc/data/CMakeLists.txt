
FILE(GLOB DATAFILES *.htm *.html *.ico *.jpg *.png *.xpm *.txt *.mk)
INSTALL(FILES ${DATAFILES} DESTINATION ${HUGINDATADIR}/xrc/data)

IF (UNIX)
  INSTALL(FILES hugin.png DESTINATION ${DATADIR}/pixmaps)
ENDIF (UNIX)

ADD_SUBDIRECTORY(help_en_EN)
