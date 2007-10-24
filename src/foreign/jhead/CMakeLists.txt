
SET(HUGIN_JHEAD_SRC jpgfile.cpp exif.cpp)
IF (${HUGIN_SHARED_LIBS})
  add_library(huginjhead SHARED ${HUGIN_JHEAD_SRC})
  set_target_properties(huginjhead PROPERTIES VERSION ${HUGIN_LIB_VERSION})
  install(TARGETS huginjhead DESTINATION ${LIBDIR})
ELSE (${HUGIN_SHARED_LIBS})
  add_library(huginjhead STATIC ${HUGIN_JHEAD_SRC})
ENDIF (${HUGIN_SHARED_LIBS})
