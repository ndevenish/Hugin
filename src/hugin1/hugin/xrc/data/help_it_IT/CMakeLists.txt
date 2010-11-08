
IF(WIN32)

#TODO generate italian version of help file

ELSE(WIN32)

FILE(GLOB DATAFILES *.htm *.html *.ico *.gif *.JPG *.jpg *.png *.xpm *.hhc *.hhk *.hhp
*.manual)

INSTALL(FILES ${DATAFILES} DESTINATION ${HUGINDATADIR}/xrc/data/help_it_IT)

ENDIF(WIN32)