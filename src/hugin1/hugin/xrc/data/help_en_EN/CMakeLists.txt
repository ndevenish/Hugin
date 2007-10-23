
FILE(GLOB DATAFILES *.htm *.html *.ico *.jpg *.png *.xpm *.txt *hhc *hhk *hhp)

INSTALL(FILES ${DATAFILES} DESTINATION ${HUGINDATADIR}/xrc/data/help_en_EN)

