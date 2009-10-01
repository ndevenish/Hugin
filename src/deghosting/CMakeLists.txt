
add_executable(deghosting_mask deghosting_mask.cpp deghosting.cpp)
target_link_libraries(deghosting_mask ${common_libs} ${image_libs})

install(TARGETS deghosting_mask DESTINATION ${BINDIR})
