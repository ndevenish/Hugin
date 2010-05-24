if (ENABLE_SURF)
  set(SURF_SRC SurfKeyPointDescriptor.cpp)
  set(SURF_HEADER SurfKeyPointDescriptor.h)
endif (ENABLE_SURF)
  
  # the liblocalfeature library
set (LF_SRC  RansacFiltering.cpp Homography.cpp Image.cpp
             ${SURF_SRC} CircularKeyPointDescriptor.cpp
	     KeyPointDetector.cpp KeyPointIO.cpp MathStuff.cpp)

set (LF_HEADER BoundedSet.h BoxFilter.h CircularKeyPointDescriptor.h
               ${SURF_HEADER} Homography.h Image.h KeyPoint.h KeyPointDescriptor.h
               KeyPointDetector.h KeyPointIO.h MathStuff.h
               PointMatch.h RansacFiltering.h Sieve.h WaveFilter.h )

add_library(localfeatures SHARED ${LF_SRC} ${LF_HEADER})
 
IF(WIN32)
  INSTALL(TARGETS localfeatures DESTINATION ${BINDIR})
ELSE(WIN32)
  INSTALL(TARGETS localfeatures DESTINATION ${LIBDIR})
ENDIF(WIN32)

# install headers (for development)
INSTALL(FILES  ${LF_HEADER} DESTINATION ${INCDIR}/localfeatures)

# the python interface for it
# temporarily disabled, it is not working anyway.
#IF (BOOST_PYTHON_FOUND)
#  add_library(pylf SHARED python_wrapper.cpp)
#  SET_TARGET_PROPERTIES(pylf PROPERTIES PREFIX "")
#  TARGET_LINK_LIBRARIES(pylf ${BOOST_PYTHON_LIBS} localfeatures)
#ENDIF (BOOST_PYTHON_FOUND)
