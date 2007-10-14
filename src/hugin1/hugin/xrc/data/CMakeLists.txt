
FILE(GLOB DATAFILES *.htm *.html *.ico *.jpg *.png *.xpm *.txt *.mk)

INSTALL(FILES ${DATAFILES} DESTINATION ${DATADIR}/xrc/data)

ADD_SUBDIRECTORY(help_en_EN)
