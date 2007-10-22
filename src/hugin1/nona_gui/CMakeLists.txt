
add_executable(nona_gui nona_gui.cpp) 

target_link_libraries(nona_gui ${common_libs} ${image_libs} huginbasewx ${wxWidgets_LIBRARIES})

add_subdirectory(po) 
