
add_executable(fulla fulla.cpp)
target_link_libraries(fulla ${common_libs} huginlensdb ${image_libs})

add_executable(nona nona.cpp)
target_link_libraries(nona ${common_libs} ${image_libs})

add_executable(autooptimiser autooptimiser.cpp)
target_link_libraries(autooptimiser ${common_libs} ${image_libs})

add_executable(vig_optimize vig_optimize.cpp)
target_link_libraries(vig_optimize ${common_libs} ${image_libs})
