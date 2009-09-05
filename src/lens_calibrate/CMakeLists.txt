ADD_EXECUTABLE(calibrate_lens 
Main.cpp
Globals.cpp
MapPoints.cpp
ProcessImage.cpp
Straighten.cpp
lensFunc.cpp
HermiteSpline.c
find_N8_lines.cpp
)

ADD_EXECUTABLE( test_lensFunc
test_lensFunc.cpp
lensFunc.cpp
HermiteSpline.c
)

TARGET_LINK_LIBRARIES( calibrate_lens huginANN ${image_libs} ${common_libs} )
INSTALL(TARGETS calibrate_lens DESTINATION ${BINDIR}) 
