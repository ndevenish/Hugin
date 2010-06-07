include_directories( ${CMAKE_SOURCE_DIR}/src/foreign/tclap )
include_directories( ${CMAKE_SOURCE_DIR}/src/foreign/zthread/include )
include_directories(.)

add_subdirectory(localfeatures)
add_subdirectory(keypoints)
add_subdirectory(panomatic)
