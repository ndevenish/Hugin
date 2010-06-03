include_directories( ${CMAKE_SOURCE_DIR} )

# the liblocalfeature library
add_executable(keypoints keypoints.cpp)

IF (${HUGIN_SHARED_LIBS})
	target_link_libraries(keypoints localfeatures ${VIGRA_LIBRARIES})
ENDIF(${HUGIN_SHARED_LIBS})

INSTALL(TARGETS keypoints DESTINATION ${BINDIR})
