
#list all source files here
ADD_EXECUTABLE(matchpoint MatchPoint.cpp APImage.cpp HessianDetector.cpp Descriptor.cpp)
IF (MSVC)
	TARGET_LINK_LIBRARIES(matchpoint ${image_libs} hugingetopt vigraimpex)
	ADD_DEPENDENCIES(matchpoint hugingetopt)
#need to link to some other libraries ? just add them here
ELSE (MSVC)
	TARGET_LINK_LIBRARIES(matchpoint ${image_libs} vigraimpex)
ENDIF (MSVC)

# matchpoint is not ready for general use yet
#
#install(TARGETS matchpoint
#        DESTINATION ${BINDIR}) 
