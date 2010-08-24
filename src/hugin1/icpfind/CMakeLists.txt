SET(ICPFIND_LIB_SRC AutoCtrlPointCreator.cpp CPDetectorConfig.cpp)
SET(ICPFIND_LIB_HEADER AutoCtrlPointCreator.h CPDetectorConfig.h CPDetectorConfig_default.h)

IF (${HUGIN_SHARED_LIBS})
  ADD_LIBRARY(icpfindlib SHARED ${ICPFIND_LIB_SRC} ${ICPFIND_LIB_HEADER})
  TARGET_LINK_LIBRARIES(icpfindlib ${Boost_LIBRARIES})
  SET_TARGET_PROPERTIES(icpfindlib PROPERTIES VERSION ${HUGIN_LIB_VERSION})
  IF(WIN32)
    INSTALL(TARGETS icpfindlib RUNTIME DESTINATION ${BINDIR})
  ELSE(WIN32)
    INSTALL(TARGETS icpfindlib DESTINATION ${LIBDIR})
  ENDIF(WIN32)
ELSE (${HUGIN_SHARED_LIBS})
  ADD_LIBRARY(icpfindlib STATIC ${ICPFIND_LIB_SRC} ${ICPFIND_LIB_HEADER})
ENDIF (${HUGIN_SHARED_LIBS})

TARGET_LINK_LIBRARIES(icpfindlib huginbase makefilelib ${wxWidgets_LIBRARIES} huginbasewx)

ADD_EXECUTABLE(icpfind icpfind.h icpfind.cpp)

TARGET_LINK_LIBRARIES(icpfind icpfindlib ${wxWidgets_LIBRARIES}  ${common_libs})
INSTALL(TARGETS icpfind DESTINATION ${BINDIR})

        
