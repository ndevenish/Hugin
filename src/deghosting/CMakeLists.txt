
add_executable(deghosting_mask deghosting_mask.cpp deghosting.h deghosting.cpp denoise.h khan.h support.h threshold.h)
target_link_libraries(deghosting_mask ${common_libs} ${image_libs})

install(TARGETS deghosting_mask DESTINATION ${BINDIR})
