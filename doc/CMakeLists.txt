# Windows does not have a man page system
# TODO: it would be nice to convert the POD files to a text that is
#       useful for Windows, and put this documentation in a sensible
#       place on Windows machines
IF (WIN32)
  INSTALL(FILES fulla.html DESTINATION doc)
  INSTALL(FILES nona.txt DESTINATION doc)
ELSE (WIN32)

# For all other systems, generate and install the man pages

FIND_PROGRAM(POD2MAN pod2man)

MACRO(install_man manfile section)
  INSTALL(FILES ${manfile} DESTINATION ${MANDIR}/man${section})
ENDMACRO(install_man)

MACRO(do_pod podfile manfile section)
  IF(EXISTS "${CMAKE_SOURCE_DIR}/doc/${podfile}")
    IF(POD2MAN)
      SET(outfile "${manfile}.${section}")
      # TODO: fix the unaesthetic double quotes in which the HUGIN_PACKAGE_VERSION
      # is displayed on the resulting man page
      ADD_CUSTOM_COMMAND(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${outfile}"
        COMMAND ${POD2MAN} --section="${section}" --release="Version: ${HUGIN_PACKAGE_VERSION}" --center=HUGIN  "${CMAKE_CURRENT_BINARY_DIR}/${podfile}"  "${CMAKE_CURRENT_BINARY_DIR}/${outfile}"
        DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${podfile}"
	VERBATIM
      )
      install_man(${outfile} ${section})
      LIST(APPEND ManDeps "${CMAKE_CURRENT_BINARY_DIR}/${outfile}")
    ENDIF(POD2MAN)
  ENDIF()
ENDMACRO(do_pod)

MACRO(do_doc)
    # automatically include all pod files in the directory
    FILE(GLOB POD_FILES RELATIVE "${CMAKE_SOURCE_DIR}/doc" *.pod)
    # all our man pages go into section 1
    SET(MANSECTION 1)
    FOREACH(PODFILE ${POD_FILES})
        ADD_CUSTOM_COMMAND(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${PODFILE}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/doc/${PODFILE}" "${CMAKE_CURRENT_BINARY_DIR}/${PODFILE}"
         DEPENDS "${CMAKE_SOURCE_DIR}/doc/${PODFILE}"
	 COMMENT "copying ${CMAKE_SOURCE_DIR}/doc/${PODFILE}  to ${CMAKE_CURRENT_BINARY_DIR}/${PODFILE}"
	 VERBATIM
        )
	SET_SOURCE_FILES_PROPERTIES("${CMAKE_CURRENT_BINARY_DIR}/${PODFILE}" GENERATED)
        STRING(REGEX REPLACE "\\.pod$" "" MANFILE "${PODFILE}")
        # TODO: this currently messes up source tree as MANFILE is implicitly
        #       inside the same folder as PODFILE.  change that do to the
        #       man files in the build tree
        # MESSAGE(STATUS "generating man page ${CMAKE_CURRENT_BINARY_DIR}${MANFILE}")
        do_pod(${PODFILE} ${MANFILE} ${MANSECTION})
    ENDFOREACH(PODFILE ${POD_FILES})
ENDMACRO(do_doc)

set(ManDeps)
do_doc()
ADD_CUSTOM_TARGET(ManPages ALL DEPENDS ${ManDeps})

ENDIF(WIN32)
