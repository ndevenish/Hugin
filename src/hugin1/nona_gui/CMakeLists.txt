
add_executable(nona_gui nona_gui.cpp) 

target_link_libraries(nona_gui ${common_libs} ${image_libs} huginbasewx ${wxWidgets_LIBRARIES})

install(TARGETS nona_gui DESTINATION bin)

add_subdirectory(po) 
