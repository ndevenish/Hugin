
add_executable(hugin_stitch_project hugin_stitch_project.cpp)

target_link_libraries(hugin_stitch_project ${common_libs} ${image_libs} huginbasewx ${wxWidgets_LIBRARIES})

install(TARGETS hugin_stitch_project
        DESTINATION bin)


