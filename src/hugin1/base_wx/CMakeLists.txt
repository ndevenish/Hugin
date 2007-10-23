
SET(HUGIN_WX_BASE_SRC ImageCache.cpp MyProgressDialog.cpp Plot2D.cpp PTWXDlg.cpp
                      huginConfig.cpp MyExternalCmdExecDialog.cpp)

IF ($HUGIN_SHARED_LIBS)
  add_library(huginbasewx SHARED ${HUGIN_WX_BASE_SRC})
  install(TARGETS huginbasewx DESTINATION ${LIBDIR})
ELSE ($HUGIN_SHARED_LIBS)
  add_library(huginbasewx STATIC ${HUGIN_WX_BASE_SRC})
ENDIF ($HUGIN_SHARED_LIBS)

