
add_subdirectory(ANN)
add_subdirectory(vigra)
add_subdirectory(levmar)
add_subdirectory(jhead)
add_subdirectory(lensdb)

IF (WIN32 AND NOT MINGW)
  add_subdirectory(getopt)
ENDIF (WIN32 AND NOT MINGW)

