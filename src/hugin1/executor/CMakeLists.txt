ADD_EXECUTABLE(hugin_executor hugin_executor.cpp)

TARGET_LINK_LIBRARIES(hugin_executor huginbase huginbasewx ${wxWidgets_LIBRARIES}  ${common_libs})
INSTALL(TARGETS hugin_executor DESTINATION ${BINDIR})

FILE(GLOB EXECUTOR_FILES *.executor)
INSTALL(FILES ${EXECUTOR_FILES} DESTINATION ${HUGINDATADIR}/data)

