

include_directories( ${CMAKE_SOURCE_DIR} )

# the liblocalfeature library
add_executable(keypoints keypoints.cpp)

# the python interface for it
#target_link_libraries(keypoints ${BOOST_PYTHON_LIBS} localfeatures vigraimpex)
target_link_libraries(keypoints localfeatures ${VIGRA_LIBRARIES})

INSTALL(TARGETS keypoints DESTINATION ${BINDIR})