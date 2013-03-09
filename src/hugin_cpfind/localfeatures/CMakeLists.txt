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
    set_target_properties(localfeatures PROPERTIES VERSION ${HUGIN_LIB_VERSION})
    IF(WIN32)
      install(TARGETS localfeatures RUNTIME DESTINATION ${BINDIR})
    ELSEIF(${HUGIN_LIBS_PRIVATE_DIR})
      install(TARGETS localfeatures LIBRARY DESTINATION ${LIBDIR}/hugin NAMELINK_SKIP)
    ELSE(WIN32)
      install(TARGETS localfeatures LIBRARY DESTINATION ${LIBDIR} NAMELINK_SKIP)
    ENDIF(WIN32)
ELSE (${HUGIN_SHARED_LIBS})
    add_library(localfeatures STATIC ${LF_SRC} ${LF_HEADER})
ENDIF (${HUGIN_SHARED_LIBS})

OPTION (BUILD_KEYPOINTS "Build keypoints program" OFF)

IF(BUILD_KEYPOINTS)
FIND_PACKAGE(Tclap)
IF(NOT TCLAP_FOUND)
  MESSAGE(FATAL_ERROR "Tclap library not found")
ENDIF(NOT TCLAP_FOUND)
INCLUDE_DIRECTORIES(${TCLAP_INCLUDEDIR})

add_executable(keypoints keypoints.cpp)

IF(WIN32)
	target_link_libraries(keypoints localfeatures ${ZTHREAD_LIBRARIES} ${image_libs} ${common_libs})
ELSE(WIN32)
	target_link_libraries(keypoints localfeatures ${ZTHREAD_LIBRARIES} pthread ${image_libs} ${common_libs})
ENDIF(WIN32)

install(TARGETS keypoints DESTINATION ${BINDIR})
ENDIF()
