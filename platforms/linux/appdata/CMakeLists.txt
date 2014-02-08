IF (UNIX)
# install appdata files
  INSTALL(FILES calibrate_lens_gui.appdata.xml hugin.appdata.xml PTBatcherGUI.appdata.xml DESTINATION ${DATADIR}/appdata)
ENDIF (UNIX)

