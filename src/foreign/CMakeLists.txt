
add_subdirectory(ANN)
add_subdirectory(vigra)
add_subdirectory(levmar)
add_subdirectory(lensdb)
add_subdirectory(flann)
add_subdirectory(zthread/src)

IF (WIN32 AND NOT MINGW)
  add_subdirectory(getopt)
ENDIF (WIN32 AND NOT MINGW)

