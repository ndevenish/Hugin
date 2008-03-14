
#list all source files here
ADD_EXECUTABLE(matchpoint MatchPoint.cpp APImage.cpp HessianDetector.cpp Descriptor.cpp)
#need to link to some other libraries ? just add them here
TARGET_LINK_LIBRARIES(matchpoint ${image_libs})

# matchpoint is not ready for general use yet
#
#install(TARGETS matchpoint
#        DESTINATION ${BINDIR}) 
