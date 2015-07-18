# install plugin files into appropriate folder

FILE(GLOB PLUGINFILES *.py README*.txt)
INSTALL(PROGRAMS ${PLUGINFILES} DESTINATION ${HUGINDATADIR}/data/plugins-templates)
