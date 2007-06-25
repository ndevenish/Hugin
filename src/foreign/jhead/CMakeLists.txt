
#build a shared library
add_library(huginjhead SHARED jpgfile.cpp exif.cpp)
install(TARGETS huginjhead DESTINATION lib)