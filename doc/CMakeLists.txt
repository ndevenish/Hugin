# Windows does not have a man page system
# TODO: it would be nice to convert the POD files to a text that is
#       useful for Windows, and put this documentation in a sensible
#       place on Windows machines
IF (WIN32)
  INSTALL(FILES fulla.html DESTINATION doc)
  INSTALL(FILES nona.txt DESTINATION doc)
ELSE (WIN32)

# For all other systems, generate and install the man pages
ADD_CUSTOM_TARGET(ManPages ALL)

FIND_PROGRAM(POD2MAN pod2man)

MACRO(install_man manfile section)
  INSTALL(FILES ${manfile} DESTINATION ${MANDIR}/man${section})
ENDMACRO(install_man)

MACRO(do_pod podfile manfile section)
  IF(EXISTS ${podfile})
    IF(POD2MAN)
      SET(outfile "${manfile}.${section}")
      # TODO: fix the unaesthetic double quotes in which the HUGIN_PACKAGE_VERSION
      # is displayed on the resulting man page
      EXECUTE_PROCESS(COMMAND ${POD2MAN} --section="${section}" --release="Version: ${HUGIN_PACKAGE_VERSION}" --center=HUGIN ${podfile} ${outfile})
      install_man(${outfile} ${section})
    ENDIF(POD2MAN)
  ENDIF(EXISTS ${podfile})
ENDMACRO(do_pod)

MACRO(do_doc)
    # automatically include all pod files in the directory
    FILE(GLOB POD_FILES *.pod)
    # all our man pages go into section 1
    SET(MANSECTION 1)
    FOREACH(PODFILE ${POD_FILES})
        STRING(REGEX REPLACE "\\.pod$" "" MANFILE "${PODFILE}")
        # TODO: this currently messes up source tree as MANFILE is implicitly
        #       inside the same folder as PODFILE.  change that do to the
        #       man files in the build tree
        # MESSAGE(STATUS "generating man page ${CMAKE_CURRENT_BINARY_DIR}${MANFILE}")
        do_pod(${PODFILE} ${MANFILE} ${MANSECTION})
    ENDFOREACH(PODFILE ${POD_FILES})
ENDMACRO(do_doc)

do_doc()

ENDIF(WIN32)
