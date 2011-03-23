
# install help/manual
IF(WIN32)

# running hhc with relative path does not work correctly, so we copy all manual file into temporary
# directory and run hhc there

FILE(GLOB HELPFILES 
  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  *.htm *.html *.ico *.gif *.JPG *.jpg *.png *.xpm hugin_help_en_EN.hhp help_index_en.hhk help_toc_en.hhc hhc.cmake *.css
)

SET(HELP_DIR ${CMAKE_BINARY_DIR}/help)
SET(HELPFILES2)  # empty list

FOREACH(_file ${HELPFILES})
  ADD_CUSTOM_COMMAND(
	OUTPUT "${HELP_DIR}/${_file}"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/${_file}" "${HELP_DIR}/${_file}"
    DEPENDS ${_file}
    COMMENT "Copy ${_file} to ${HELP_DIR}"
  )
  SET_SOURCE_FILES_PROPERTIES("${HELP_DIR}/${_file}" GENERATED)
  LIST(APPEND HELPFILES2 "${HELP_DIR}/${_file}")
ENDFOREACH()

ADD_CUSTOM_COMMAND(
  OUTPUT ${HELP_DIR}/hugin_help_en_EN.chm
  COMMAND ${CMAKE_COMMAND} -DHTML_HELP_COMPILER=${HTML_HELP_COMPILER} -P hhc.cmake
  # COMMAND ${HTML_HELP_COMPILER} hugin_help_en_EN.hhp
  DEPENDS ${HELPFILES2}
  WORKING_DIRECTORY ${HELP_DIR}
  COMMENT "Building help file"
)
SET_SOURCE_FILES_PROPERTIES("${HELP_DIR}/hugin_help_en_EN.chm" GENERATED)
ADD_CUSTOM_TARGET(help ALL DEPENDS ${HELPFILE2} ${HELP_DIR}/hugin_help_en_EN.chm )

INSTALL(FILES ${CMAKE_BINARY_DIR}/help/hugin_help_en_EN.chm DESTINATION ${HUGINDATADIR}/xrc/data/)

ELSE(WIN32)

FILE(GLOB DATAFILES *.css *.htm *.html *.ico *.gif *.JPG *.jpg *.png *.xpm *.hhc *.hhk *.hhp
*.manual)

INSTALL(FILES ${DATAFILES} DESTINATION ${HUGINDATADIR}/xrc/data/help_en_EN)

ENDIF(WIN32)
