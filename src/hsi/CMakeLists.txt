# This CMakeList.txt is needed to make The Python 
# scripting interface. The CMake code has been taken from
# http://www.swig.org/Doc1.3/Introduction.html#Introduction
# section 2.6, the only modification apart from adapting the
# names was the inclusion of ${common_libs}

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

SET(HUGIN_HPI_SOURCES hpi.h hpi_classes.h hpi.cpp)
IF (${HUGIN_SHARED_LIBS})
  add_library(hugin_python_interface SHARED ${HUGIN_HPI_SOURCES})
  set_target_properties(hugin_python_interface PROPERTIES VERSION ${HUGIN_LIB_VERSION})
  IF(WIN32)
    install(TARGETS hugin_python_interface RUNTIME DESTINATION ${BINDIR})
  ELSEIF(${HUGIN_LIBS_PRIVATE_DIR})
    install(TARGETS hugin_python_interface LIBRARY DESTINATION ${LIBDIR}/hugin NAMELINK_SKIP)
  ELSE(WIN32)
    install(TARGETS hugin_python_interface LIBRARY DESTINATION ${LIBDIR} NAMELINK_SKIP)
  ENDIF(WIN32)
ELSE (${HUGIN_SHARED_LIBS})
  add_library(hugin_python_interface STATIC  ${HUGIN_HPI_SOURCES})
ENDIF (${HUGIN_SHARED_LIBS})
TARGET_LINK_LIBRARIES(hugin_python_interface huginbase ${PYTHON_LIBRARIES})

IF(MSVC)
SET(PREPROCESSOR_FLAGS "/EP")
ELSE()
SET(PREPROCESSOR_FLAGS "/E")
ENDIF()
ADD_CUSTOM_COMMAND(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/Accessor_preprocessed.txt"
    COMMAND "${CMAKE_CXX_COMPILER}"
    ARGS ${PREPROCESSOR_FLAGS} "../hugin_base/panodata/SrcPanoImage_accessor_macros.h" ">${CMAKE_CURRENT_BINARY_DIR}/Accessor_preprocessed.txt"
	WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Preprocessing SrcPanoImage.h"
    DEPENDS ../hugin_base/panodata/SrcPanoImage_accessor_macros.h ../hugin_base/panodata/image_variables.h
)
SET_SOURCE_FILES_PROPERTIES("${CMAKE_CURRENT_BINARY_DIR}/Accessor_preprocessed.txt" GENERATED)

ADD_CUSTOM_COMMAND(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/hsi_SrcPanoImage.h
  COMMAND ${CMAKE_COMMAND} -DPROCESS_DIR=${CMAKE_CURRENT_BINARY_DIR} -P preprocess.cmake
  DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/Accessor_preprocessed.txt"
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Building hsi_SrcPanoImage.h"
)
SET_SOURCE_FILES_PROPERTIES("${CMAKE_CURRENT_BINARY_DIR}/hsi_SrcPanoImage.h" GENERATED)

SET(CMAKE_SWIG_FLAGS "-I${CMAKE_CURRENT_BINARY_DIR}")

SET_SOURCE_FILES_PROPERTIES(hsi.i PROPERTIES CPLUSPLUS ON)
SWIG_ADD_MODULE(hsi python hsi.i hsi.cpp)
SWIG_LINK_LIBRARIES(hsi ${PYTHON_LIBRARIES} ${common_libs})

# create swigpyrun.h
ADD_CUSTOM_COMMAND(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/swigpyrun.h"
  COMMAND "${SWIG_EXECUTABLE}"
  ARGS "-c++" "-python" "-external-runtime" 
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  COMMENT "Build header file..."
)
SET_SOURCE_FILES_PROPERTIES("${CMAKE_CURRENT_BINARY_DIR}/swigpyrun.h" GENERATED)
ADD_CUSTOM_TARGET(hsi_header 
  DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/swigpyrun.h" "${CMAKE_CURRENT_BINARY_DIR}/hsi_SrcPanoImage.h"
)

ADD_DEPENDENCIES(${SWIG_MODULE_hsi_REAL_NAME} hsi_header)
ADD_DEPENDENCIES(hugin_python_interface hsi_header)

IF(WIN32)
    INSTALL(TARGETS ${SWIG_MODULE_hsi_REAL_NAME} DESTINATION ${BINDIR})
    INSTALL(FILES hpi.py DESTINATION ${BINDIR})
    INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/hsi.py DESTINATION ${BINDIR})
ELSEIF(${HUGIN_LIBS_PRIVATE_DIR})
    # paths for unix needs to be checked
    # INSTALL(TARGETS ${SWIG_MODULE_hsi_REAL_NAME} LIBRARY DESTINATION ${LIBDIR}/hugin NAMELINK_SKIP)
    # INSTALL(FILES hpi.py DESTINATION ${LIBDIR}/hugin)
    # INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/hsi.py DESTINATION ${LIBDIR}/hugin)
ELSE(WIN32)
    # INSTALL(TARGETS ${SWIG_MODULE_hsi_REAL_NAME} LIBRARY DESTINATION ${LIBDIR} NAMELINK_SKIP)
    # INSTALL(FILES hpi.py DESTINATION ${LIBDIR})
    # INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/hsi.py DESTINATION ${LIBDIR})
ENDIF(WIN32)

# this would make a standalone program 'hsi', if #define standalone
# is set in hpi.cpp (that would be for testing purposes, if linking
# to hugin isn't an option):

# add_executable(hpi hpi.cpp)
# target_link_libraries(hpi ${PYTHON_LIBRARIES} ${common_libs} )
