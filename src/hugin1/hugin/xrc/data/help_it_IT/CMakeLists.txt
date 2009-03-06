
FILE(GLOB DATAFILES *.htm *.html *.ico *.gif *.JPG *.jpg *.png *.xpm *.txt *hhc *hhk *hhp
*.manual)

INSTALL(FILES ${DATAFILES} DESTINATION ${HUGINDATADIR}/xrc/data/help_it_IT)

