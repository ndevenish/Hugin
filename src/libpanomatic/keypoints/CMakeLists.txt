include_directories( ${CMAKE_SOURCE_DIR} )

# the liblocalfeature library
add_executable(keypoints keypoints.cpp)

target_link_libraries(keypoints localfeatures ${image_libs})

INSTALL(TARGETS keypoints DESTINATION ${BINDIR})
