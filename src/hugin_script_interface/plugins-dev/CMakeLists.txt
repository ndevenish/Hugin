# install plugin files into appropriate folder

FILE(GLOB PLUGINFILES *.py README*.txt)
INSTALL(FILES ${PLUGINFILES} DESTINATION ${INSTALL_DATA_DIR}/plugins-templates)
