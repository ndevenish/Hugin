# install plugin files into appropriate folder

FILE(GLOB PLUGINFILES *.py *.txt)
INSTALL(FILES ${PLUGINFILES} DESTINATION ${INSTALL_DATA_DIR}/plugins)
