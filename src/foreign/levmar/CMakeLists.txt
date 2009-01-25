add_library(huginlevmar STATIC lm.c Axb.c misc.c lmlec.c lmbc.c)
IF (NOT MSVC)
  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
ENDIF (NOT MSVC)