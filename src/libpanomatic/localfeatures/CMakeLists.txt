# the liblocalfeature library
set (LF_SRC  RansacFiltering.cpp Homography.cpp Image.cpp
             CircularKeyPointDescriptor.cpp
	      KeyPointDetector.cpp KeyPointIO.cpp MathStuff.cpp)

set (LF_HEADER BoundedSet.h BoxFilter.h CircularKeyPointDescriptor.h
               Homography.h Image.h KeyPoint.h KeyPointDescriptor.h
               KeyPointDetector.h KeyPointIO.h MathStuff.h
               PointMatch.h RansacFiltering.h Sieve.h WaveFilter.h )

IF (${HUGIN_SHARED_LIBS})
  add_library(localfeatures SHARED ${LF_SRC} ${LF_HEADER})
ELSE (${HUGIN_SHARED_LIBS})
  add_library(localfeatures STATIC ${LF_SRC} ${LF_HEADER})
ENDIF (${HUGIN_SHARED_LIBS})
 
IF(WIN32)
  INSTALL(TARGETS localfeatures DESTINATION ${BINDIR})
ELSE(WIN32)
  INSTALL(TARGETS localfeatures DESTINATION ${LIBDIR})
ENDIF(WIN32)


# the python interface for it
# temporarily disabled, it is not working anyway.
#IF (BOOST_PYTHON_FOUND)
#  add_library(pylf SHARED python_wrapper.cpp)
#  SET_TARGET_PROPERTIES(pylf PROPERTIES PREFIX "")
#  TARGET_LINK_LIBRARIES(pylf ${BOOST_PYTHON_LIBS} localfeatures)
#ENDIF (BOOST_PYTHON_FOUND)
