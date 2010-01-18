
#list all source files here
ADD_EXECUTABLE(matchpoint MatchPoint.cpp APImage.h APImage.cpp HessianDetector.h HessianDetector.cpp Descriptor.h Descriptor.cpp edgedetection.hxx)
IF (MSVC)
	TARGET_LINK_LIBRARIES(matchpoint ${image_libs} hugingetopt)
#need to link to some other libraries ? just add them here
ELSE (MSVC)
	TARGET_LINK_LIBRARIES(matchpoint ${image_libs})
ENDIF (MSVC)

install(TARGETS matchpoint
        DESTINATION ${BINDIR}) 
