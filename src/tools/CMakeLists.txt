
add_executable(fulla fulla.cpp)
target_link_libraries(fulla ${common_libs} huginlensdb ${image_libs})

add_executable(nona nona.cpp)
target_link_libraries(nona ${common_libs} ${image_libs})

#add_executable(autocreate autocreate.cpp)
#target_link_libraries(autocreate ${common_libs} ${image_libs})

add_executable(autooptimiser autooptimiser.cpp)
target_link_libraries(autooptimiser ${common_libs} ${image_libs})

add_executable(vig_optimize vig_optimize.cpp)
target_link_libraries(vig_optimize ${common_libs} ${image_libs})

add_executable(align_image_stack align_image_stack.cpp)
target_link_libraries(align_image_stack ${common_libs} ${image_libs})

install(TARGETS nona vig_optimize autooptimiser fulla align_image_stack
        DESTINATION ${BINDIR})

