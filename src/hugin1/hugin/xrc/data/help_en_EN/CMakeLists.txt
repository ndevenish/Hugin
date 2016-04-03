
# install help/manual
IF(WIN32)

# running hhc with relative path does not work correctly, so we copy all manual file into temporary
# directory and run hhc there

FILE(GLOB HELPFILES 
  *.htm *.html *.ico *.gif *.JPG *.jpg *.png *.xpm hugin_help_en_EN.hhp help_index_en.hhk help_toc_en.hhc hhc.cmake *.css
)

SET(HELP_DIR ${CMAKE_BINARY_DIR}/help)
SET(HELPFILES2)  # empty list

IF(EXISTS "${ENBLEND_DIR}/doc/enblend.html")
  # when enblend has new documentation, we integate this new doc instead of files from wiki
  LIST(REMOVE_ITEM HELPFILES "${CMAKE_CURRENT_SOURCE_DIR}/Enblend.html")
  LIST(REMOVE_ITEM HELPFILES "${CMAKE_CURRENT_SOURCE_DIR}/Enfuse.html")
  LIST(APPEND HELPFILES "${ENBLEND_DIR}/doc/enblend.html")
  LIST(APPEND HELPFILES "${ENBLEND_DIR}/doc/enfuse.html")
  FILE(GLOB ENBLEND_DOC_IMAGES ${ENBLEND_DIR}/doc/*.png)
  LIST(APPEND HELPFILES ${ENBLEND_DOC_IMAGES})
ENDIF()

FOREACH(_file ${HELPFILES})
  get_filename_component(_filename ${_file} NAME)
  ADD_CUSTOM_COMMAND(
    OUTPUT "${HELP_DIR}/${_filename}"
    COMMAND ${CMAKE_COMMAND} -E copy "${_file}" "${HELP_DIR}/${_filename}"
    DEPENDS ${_file}
    COMMENT "Copy ${_filename} to ${HELP_DIR}"
  )
  SET_SOURCE_FILES_PROPERTIES("${HELP_DIR}/${_filename}" GENERATED)
  LIST(APPEND HELPFILES2 "${HELP_DIR}/${_filename}")
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
ADD_CUSTOM_TARGET(chmhelp ALL DEPENDS ${HELPFILE2} ${HELP_DIR}/hugin_help_en_EN.chm )

INSTALL(FILES ${CMAKE_BINARY_DIR}/help/hugin_help_en_EN.chm DESTINATION ${HUGINDATADIR}/xrc/data/)

ELSE(WIN32)

FILE(GLOB DATAFILES *.css *.htm *.html *.ico *.gif *.JPG *.jpg *.png *.xpm *.hhc *.hhk *.hhp
*.manual)

INSTALL(FILES ${DATAFILES} DESTINATION ${HUGINDATADIR}/xrc/data/help_en_EN)

ENDIF(WIN32)
