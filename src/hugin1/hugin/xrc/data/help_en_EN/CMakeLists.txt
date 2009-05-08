
FILE(GLOB DATAFILES *.htm *.html *.ico *.gif *.JPG *.jpg *.png *.xpm *.hhc *.hhk *.hhp
*.manual)

INSTALL(FILES ${DATAFILES} DESTINATION ${HUGINDATADIR}/xrc/data/help_en_EN)

