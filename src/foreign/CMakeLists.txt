
add_subdirectory(vigra)
add_subdirectory(levmar)
add_subdirectory(lensdb)

IF(NOT ZTHREAD_FOUND)
add_subdirectory(zthread/src)
ENDIF(NOT ZTHREAD_FOUND)

IF (WIN32 AND NOT MINGW)
  add_subdirectory(getopt)
ENDIF (WIN32 AND NOT MINGW)

