
add_subdirectory(levmar)

IF (WIN32 AND NOT MINGW)
  add_subdirectory(getopt)
ENDIF (WIN32 AND NOT MINGW)

