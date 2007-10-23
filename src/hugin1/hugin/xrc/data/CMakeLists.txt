
FILE(GLOB DATAFILES *.htm *.html *.ico *.jpg *.png *.xpm *.txt *.mk)
INSTALL(FILES ${DATAFILES} DESTINATION ${DATADIR}/xrc/data)

INSTALL(FILES hugin.png DESTINATION ${DATADIR}/pixmaps)
ADD_SUBDIRECTORY(help_en_EN)
