
# levmar library source files
ADD_LIBRARY(huginlevmar STATIC
  lm.c Axb.c misc.c lmlec.c lmbc.c lmblec.c lmbleic.c
  levmar.h misc.h compiler.h
)

IF(LAPACK_FOUND)
  ADD_DEFINITIONS(-DHAVE_LAPACK)
ENDIF(LAPACK_FOUND)

