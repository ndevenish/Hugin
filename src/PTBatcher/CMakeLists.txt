include_directories( ${CMAKE_SOURCE_DIR}/src/hugin1 )

set( PROGNAME PTBatcher )

add_executable(PTBatcher PTBatcher.cpp PTBatcher.h PTBatcher_rc.rc Batch.cpp Batch.h RunStitchFrame.cpp RunStitchFrame.h ProjectArray.cpp ProjectArray.h)
target_link_libraries(${PROGNAME} huginbasewx ${common_libs} ${image_libs} ${wxWidgets_LIBRARIES})
install(TARGETS PTBatcher
        DESTINATION ${BINDIR})