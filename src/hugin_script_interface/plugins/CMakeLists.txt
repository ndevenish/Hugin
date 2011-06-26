# install plugin files into appropriate folder

FILE(GLOB PLUGINFILES *.py README*.txt)
INSTALL(PROGRAMS ${PLUGINFILES} DESTINATION ${INSTALL_DATA_DIR}/plugins)
