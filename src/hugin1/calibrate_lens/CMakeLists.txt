
set(CALLENSGUI_SOURCE LensCalApp.cpp LensCalFrame.cpp LensCalImageCtrl.cpp LensCalTypes.cpp)
set(CALLENSGUI_HEADER LensCalApp.h LensCalFrame.h LensCalImageCtrl.h LensCalTypes.h)

IF (WIN32)
  add_executable(calibrate_lens_gui WIN32 ${CALLENSGUI_SOURCE} ${CALLENSGUI_HEADER} calibrate_lens_gui_rc.rc)
ELSE (WIN32)
 add_executable(calibrate_lens_gui ${CALLENSGUI_SOURCE} ${CALLENSGUI_HEADER})
ENDIF (WIN32)


target_link_libraries(calibrate_lens_gui huginbasewx ${common_libs} ${image_libs} ${wxWidgets_LIBRARIES} huginlines)

INSTALL(TARGETS calibrate_lens_gui DESTINATION ${BINDIR})
IF(UNIX)
    # install further stuff (icons etc.)
    INSTALL(FILES calibrate_lens_gui.desktop DESTINATION ${DATADIR}/applications)
ENDIF(UNIX)
