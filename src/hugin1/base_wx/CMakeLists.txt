
SET(HUGIN_WX_BASE_SRC ImageCache.cpp MyProgressDialog.cpp Plot2D.cpp PTWXDlg.cpp
                      huginConfig.cpp MyExternalCmdExecDialog.cpp platform.cpp
                      RunStitchPanel.cpp )
SET(HUGIN_WX_BASE_HEADER ImageCache.h MyProgressDialog.h Plot2D.h PTWXDlg.h
                      huginConfig.h MyExternalCmdExecDialog.h platform.h
                      RunStitchPanel.h )


IF ($HUGIN_SHARED_LIBS)
  add_library(huginbasewx SHARED ${HUGIN_WX_BASE_SRC} ${HUGIN_WX_BASE_HEADER})
  set_target_properties(huginbasewx VERSION ${HUGIN_LIB_VERSION})
  install(TARGETS huginbasewx DESTINATION ${LIBDIR})
ELSE ($HUGIN_SHARED_LIBS)
  add_library(huginbasewx STATIC ${HUGIN_WX_BASE_SRC} ${HUGIN_WX_BASE_HEADER})
ENDIF ($HUGIN_SHARED_LIBS)

