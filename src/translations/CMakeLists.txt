# This file is part of hugin, a GUI for the panorama tools suite
# Licence details can be found in the file COPYING.txt.
#
# Copyright (c) 2013 Kornel Benko, <Kornel.Benko@berlin.de>
#

OPTION(REBUILD_TRANSLATIONS "Build hugin.pot and <lang>.po files inside the build dir" OFF)

find_package(MSGFMT QUIET REQUIRED)

# Check for needed programms python2, wxrc, xgettext, msgmerge, msguniq
if(REBUILD_TRANSLATIONS)
  # Check for needed python version 2
  set(prg_err_msg)
  find_package(PythonInterp)
  if (NOT PYTHONINTERP_FOUND)
    set(prg_err_msg "No python interpreter found")
  endif()

  if(NOT prg_err_msg)
    find_program(XGETTEXT_EXECUTABLE
      NAMES xgettext
      PATHS /bin /usr/bin /usr/local/bin c:/MinGW/bin ${SOURCE_BASE_DIR}/gettext/bin)
    if(NOT XGETTEXT_EXECUTABLE)
      set(prg_err_msg "xgettext not found")
    endif()
  endif()

  if(NOT prg_err_msg)
    find_program(MSGMERGE_EXECUTABLE
      NAMES msgmerge gmsgmerge msgmerge.exe
      PATHS /bin /usr/bin /usr/local/bin c:/MinGW/bin ${SOURCE_BASE_DIR}/gettext/bin)
    if(NOT MSGMERGE_EXECUTABLE)
      set(prg_err_msg "msgmerge not found")
    endif()
  endif()

  if(NOT prg_err_msg)
    find_program(MSGUNIQ_EXECUTABLE
      NAMES msguniq gmsguniq msguniq.exe
      PATHS /bin /usr/bin /usr/local/bin c:/MinGW/bin ${SOURCE_BASE_DIR}/gettext/bin)
    if(NOT MSGUNIQ_EXECUTABLE)
      set(prg_err_msg "msguniq not found")
    endif()
  endif()

  if(prg_err_msg)
    message(STATUS ${prg_err_msg} ", disabling REBUILD_TRANSLATIONS")
    set(REBUILD_TRANSLATIONS OFF)
  else()
    message(STATUS "Needed programs for REBUILD_TRANSLATIONS found")
  endif()
endif()

if(REBUILD_TRANSLATIONS)

  # create updated template hugin.pot and remerged po-files
  # in the build-directory

  # This part should do the same, as extract-messages.sh
  set(BUGADDR "https://bugs.launchpad.net/hugin/")
  set(COPYRIGHT "Pablo dAngelo")
  # create xrc.cpp
  # Preparing rc files
  file(GLOB_RECURSE xrc_files RELATIVE "${CMAKE_SOURCE_DIR}/src" "${CMAKE_SOURCE_DIR}/src/*.xrc")
  list(SORT xrc_files)

  # creating infiles.list
  file(GLOB_RECURSE _infiles RELATIVE "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/*.h"
    "${CMAKE_SOURCE_DIR}/src/*.c")
#  list(APPEND _infiles "translations/xrc.cpp")
  list(REMOVE_ITEM _infiles "translations/xrc.cpp")
  list(REMOVE_DUPLICATES _infiles)
  list(SORT _infiles)

  # create infiles.list, containing the list of input source files
  file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/infiles.tmp")
  set(infiles_depend)
  foreach(_f ${_infiles})
    if(NOT _f STREQUAL "translations/xrc.cpp")
      list(APPEND infiles_depend "${CMAKE_SOURCE_DIR}/src/${_f}")
    endif()
    file(APPEND "${CMAKE_CURRENT_BINARY_DIR}/infiles.tmp" "./${_f}\n")
  endforeach()
  file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/POTFILES.in" POTFILE)
  foreach(_p ${POTFILE})
    list(APPEND infiles_depend "${CMAKE_SOURCE_DIR}/src/${_p}")
    file(APPEND "${CMAKE_CURRENT_BINARY_DIR}/infiles.tmp" "${_p}\n")
  endforeach()
  execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_BINARY_DIR}/infiles.tmp" "${CMAKE_CURRENT_BINARY_DIR}/infiles.list")

  # create in_xrc_files.list, containing the list of searched xrc source files
  file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/in_xrc_files.tmp")
  set(inxrcdepend)
  foreach(_f ${xrc_files})
    set(_fabs "${CMAKE_SOURCE_DIR}/src/${_f}")
    list(APPEND inxrcdepend ${_fabs})
    file(APPEND "${CMAKE_CURRENT_BINARY_DIR}/in_xrc_files.tmp" "${_fabs}\n")
  endforeach()
  execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_BINARY_DIR}/in_xrc_files.tmp" "${CMAKE_CURRENT_BINARY_DIR}/in_xrc_files.list")

  # now create the hugintmp.pot file, with possible duplicated entries
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/hugintmp.pot"
    COMMAND ${XGETTEXT_EXECUTABLE} --from-code=UTF-8 -C -k_ --copyright-holder=${COPYRIGHT}
      --msgid-bugs-address="${BUGADDR}"
      --files-from="${CMAKE_CURRENT_BINARY_DIR}/infiles.list"
      -D "${CMAKE_SOURCE_DIR}/src"
      -o "${CMAKE_CURRENT_BINARY_DIR}/hugintmp1.pot"
    COMMAND ${PYTHON_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/xrc_pot.py"
      -o "${CMAKE_CURRENT_BINARY_DIR}/hugintmp1.pot"
      -b "${CMAKE_SOURCE_DIR}/src"
      -t xrc -s "${CMAKE_CURRENT_BINARY_DIR}/in_xrc_files.list"
    COMMAND ${PYTHON_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/xrc_correct.py"
      -o "${CMAKE_CURRENT_BINARY_DIR}/hugintmp.pot"
      "${CMAKE_CURRENT_BINARY_DIR}/hugintmp1.pot"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/infiles.list"
      "${CMAKE_CURRENT_BINARY_DIR}/in_xrc_files.list"
      "${CMAKE_CURRENT_SOURCE_DIR}/xrc_pot.py"
      ${infiles_depend} ${inxrcdepend})

  # Last step in creating hugin.pot
  # concatenate duplicated entries and correct some non-ascii not handled by msguniq
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/hugin.pot"
    COMMAND ${MSGUNIQ_EXECUTABLE} -o "${CMAKE_CURRENT_BINARY_DIR}/hugin.pot"
      "${CMAKE_CURRENT_BINARY_DIR}/hugintmp.pot"
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/hugintmp.pot"
      "${CMAKE_CURRENT_SOURCE_DIR}/xrc_correct.py")

  #now get the po-files
  file(GLOB pofiles RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/*.po")
  set(PO_FILES)
  foreach(_po ${pofiles})
    list(APPEND PO_FILES "${CMAKE_CURRENT_BINARY_DIR}/${_po}")
    add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${_po}"
      COMMAND ${MSGMERGE_EXECUTABLE} --verbose -o "${CMAKE_CURRENT_BINARY_DIR}/${_po}"
        "${CMAKE_CURRENT_SOURCE_DIR}/${_po}" "${CMAKE_CURRENT_BINARY_DIR}/hugin.pot"
      DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${_po}" "${CMAKE_CURRENT_BINARY_DIR}/hugin.pot"
      COMMENT "${MSGMERGE_EXECUTABLE} --verbose -o ${CMAKE_CURRENT_BINARY_DIR}/${_po} ${CMAKE_CURRENT_SOURCE_DIR}/${_po} ${CMAKE_CURRENT_BINARY_DIR}/hugin.pot"
       )
  endforeach()

  ADD_TRANSLATIONS("${CMAKE_CURRENT_BINARY_DIR}/hugin.pot" ${PO_FILES})

else()
  # simply use all po files and hugin.pot from the source directory
  FILE(GLOB PO_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.po")

  ADD_TRANSLATIONS("${CMAKE_CURRENT_SOURCE_DIR}/hugin.pot" ${PO_FILES})
endif()

IF(APPLE)
  add_custom_command(
    TARGET translations
    POST_BUILD
    COMMAND ${CMAKE_SOURCE_DIR}/mac/CopyTranslations.sh ${CMAKE_CURRENT_BINARY_DIR})
ENDIF()
