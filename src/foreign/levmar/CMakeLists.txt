
# levmar library source files
set(LEVMAR_SOURCE
    lm.c Axb.c misc.c 
    # the following files are not needed for Hugin
    # therefore commenting out
    # lmlec.c lmbc.c lmblec.c lmbleic.c
)
set(LEVMAR_HEADER
    levmar.h misc.h compiler.h
)

ADD_LIBRARY(huginlevmar STATIC ${LEVMAR_SOURCE} ${LEVMAR_HEADER})

IF (NOT MSVC AND NOT MINGW)
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
  SET_TARGET_PROPERTIES(huginlevmar PROPERTIES COMPILE_FLAGS -fPIC)
ENDIF (NOT MSVC AND NOT MINGW)

IF(LAPACK_FOUND)
  ADD_DEFINITIONS(-DHAVE_LAPACK)
ENDIF(LAPACK_FOUND)

