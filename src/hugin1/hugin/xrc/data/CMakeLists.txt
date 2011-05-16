
FILE(GLOB DATAFILES *.htm *.html *.ico *.jpg *.png tips.txt upstream.txt *.mk)
INSTALL(FILES ${DATAFILES} DESTINATION ${HUGINDATADIR}/xrc/data)

# parse contributors list
file(READ "${PROJECT_SOURCE_DIR}/authors.txt" AUTHORS_LIST)

# replace tab with next table cell
string(REGEX REPLACE "\t"
         "</td><td>" AUTHORS_LIST "${AUTHORS_LIST}")
# replace newline with next table row
string(REGEX REPLACE "\n"
         "</td></tr><tr><td>" AUTHORS_LIST "${AUTHORS_LIST}")
# remove extra last row
string(REGEX REPLACE "<tr><td>$"
         "" AUTHORS_LIST "${AUTHORS_LIST}")

configure_file(about.htm.in ${CMAKE_BINARY_DIR}/src/hugin1/hugin/xrc/data/about.htm)
INSTALL(FILES ${CMAKE_BINARY_DIR}/src/hugin1/hugin/xrc/data/about.htm DESTINATION ${HUGINDATADIR}/xrc/data)

# license text for about dialog
INSTALL(FILES ${CMAKE_SOURCE_DIR}/COPYING DESTINATION ${HUGINDATADIR}/xrc/data/)

IF (UNIX)
  INSTALL(FILES hugin.png ptbatcher.png DESTINATION ${DATADIR}/pixmaps)
ENDIF (UNIX)

# install cursor on windows
IF(WIN32)
  INSTALL(FILES cursor_cp_pick.cur DESTINATION ${HUGINDATADIR}/xrc/data/)
ENDIF(WIN32)

ADD_SUBDIRECTORY(help_en_EN)
ADD_SUBDIRECTORY(help_it_IT)
