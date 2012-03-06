
SET(HUGIN_WX_BASE_SRC wxImageCache.cpp MyProgressDialog.cpp PTWXDlg.cpp
                      huginConfig.cpp MyExternalCmdExecDialog.cpp platform.cpp
                      RunStitchPanel.cpp LensTools.cpp wxLensDB.cpp)
SET(HUGIN_WX_BASE_HEADER wxImageCache.h MyProgressDialog.h PTWXDlg.h
                      huginConfig.h MyExternalCmdExecDialog.h platform.h
                      RunStitchPanel.h LensTools.h wxLensDB.h)


IF (${HUGIN_SHARED_LIBS})
  add_library(huginbasewx SHARED ${HUGIN_WX_BASE_SRC} ${HUGIN_WX_BASE_HEADER})
  target_link_libraries(huginbasewx ${wxWidgets_LIBRARIES} huginbase ${common_libs})
  set_target_properties(huginbasewx PROPERTIES VERSION ${HUGIN_LIB_VERSION})
  IF(WIN32)
    install(TARGETS huginbasewx RUNTIME DESTINATION ${BINDIR})
  ELSEIF(${HUGIN_LIBS_PRIVATE_DIR})
    install(TARGETS huginbasewx LIBRARY DESTINATION ${LIBDIR}/hugin NAMELINK_SKIP)
  ELSE(WIN32)
    install(TARGETS huginbasewx LIBRARY DESTINATION ${LIBDIR} NAMELINK_SKIP)
  ENDIF(WIN32)
ELSE (${HUGIN_SHARED_LIBS})
  add_library(huginbasewx STATIC ${HUGIN_WX_BASE_SRC} ${HUGIN_WX_BASE_HEADER})
ENDIF (${HUGIN_SHARED_LIBS})

