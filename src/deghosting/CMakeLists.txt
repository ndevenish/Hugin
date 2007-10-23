# various command line utilities

add_executable(hugin_hdrmerge hugin_hdrmerge.cpp khan.cpp jbu.cpp khanSupport.cpp)
target_link_libraries(hugin_hdrmerge ${common_libs} ${image_libs})

install(TARGETS hugin_hdrmerge DESTINATION bin)