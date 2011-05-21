# install plugin files into approbiate folder

IF(WIN32)
    FILE(GLOB PLUGINFILES *.py *.txt)
    INSTALL(FILES ${PLUGINFILES} DESTINATION scripts)
ENDIF()
