# various command line utilities

#add_executable(hugin_merge_images hugin_merge_images.cpp)
#target_link_libraries(hugin_merge_images ${common_libs} ${image_libs})

add_executable(nona nona.cpp)
target_link_libraries(nona ${common_libs} ${image_libs})

add_executable(vig_optimize vig_optimize.cpp)
target_link_libraries(vig_optimize ${common_libs} ${image_libs})

add_executable(align_image_stack align_image_stack.cpp)
target_link_libraries(align_image_stack ${common_libs} ${image_libs})

add_executable(autooptimiser autooptimiser.cpp)
target_link_libraries(autooptimiser ${common_libs} ${image_libs})

add_executable(fulla fulla.cpp)
target_link_libraries(fulla ${common_libs} ${image_libs})

install(TARGETS nona vig_optimize align_image_stack autooptimiser fulla 
        DESTINATION bin)
