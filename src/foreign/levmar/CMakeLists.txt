add_library(huginlevmar STATIC lm.c Axb.c misc.c lmlec.c lmbc.c)
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
