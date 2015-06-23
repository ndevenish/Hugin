
# levmar library source files
ADD_LIBRARY(huginlevmar STATIC
  lm.c Axb.c misc.c lmlec.c lmbc.c lmblec.c lmbleic.c
  levmar.h misc.h compiler.h
)

IF (NOT MSVC)
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
  SET_TARGET_PROPERTIES(huginlevmar PROPERTIES COMPILE_FLAGS -fPIC)
ENDIF (NOT MSVC)

IF(LAPACK_FOUND)
  ADD_DEFINITIONS(-DHAVE_LAPACK)
ENDIF(LAPACK_FOUND)

