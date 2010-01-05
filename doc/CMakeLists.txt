
IF (WIN32)
INSTALL(FILES fulla.html DESTINATION doc)
INSTALL(FILES nona.txt DESTINATION doc)
ELSE (WIN32)
INSTALL(FILES align_image_stack.1 autooptimiser.1 celeste_standalone.1
    fulla.1 hugin.1 hugin_hdrmerge.1 hugin_stitch_project.1 nona.1
    nona_gui.1 PTBatcherGUI.1 pto2mk.1 tca_correct.1 vig_optimize.1
    calibrate_lens.1 cpclean.1 deghosting_mask.1 pano_trafo.1
    DESTINATION ${MANDIR}/man1)
ENDIF(WIN32)

