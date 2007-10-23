
FILE(GLOB DATAFILES *.htm *.html *.ico *.jpg *.png *.xpm *.txt)

INSTALL(FILES ${DATAFILES} DESTINATION ${DATADIR}/xrc/data/help_en_EN)

