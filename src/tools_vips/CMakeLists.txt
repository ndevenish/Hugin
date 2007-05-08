# various command line utilities

link_directories(
    ${VIPSCC_LIBRARY_DIRS}
)
include_directories(
    ${VIPSCC_INCLUDE_DIRS}
)

add_executable(img2vips img2vips.cpp)
target_link_libraries(img2vips ${common_libs} ${image_libs} ${VIPSCC_LIBRARIES})

add_executable(hdrmerge_vips hdrmerge_vips.cpp)
target_link_libraries(hdrmerge_vips ${common_libs} ${image_libs} ${VIPSCC_LIBRARIES})

install(TARGETS img2vips hdrmerge_vips 
        DESTINATION bin)
