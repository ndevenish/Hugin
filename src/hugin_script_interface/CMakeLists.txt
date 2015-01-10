# This CMakeList.txt is needed to make The Python 
# scripting interface.

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

MESSAGE(STATUS "Python libs version: ${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}.${PYTHON_VERSION_PATCH}")

# access to the swig runtime code in the hpi module is via
# inclusion of a generated header file, swigpyrun.h:

ADD_CUSTOM_COMMAND(OUTPUT swigpyrun.h
  COMMAND "${SWIG_EXECUTABLE}"
  ARGS "-c++" "-python" "-external-runtime"
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  COMMENT "Build header file..."
)

SET_SOURCE_FILES_PROPERTIES("${CMAKE_CURRENT_BINARY_DIR}/swigpyrun.h"
  GENERATED)

# this generated header file and a bit of C++ code make up
# the hpi sources. This is the part of hpi/hsi that actually
# gets linked into hugin to provide access to Python and the
# plugin interface, the remaining code (hsi) stands alone as
# a python module and is dynamically loaded.

SET(HUGIN_HPI_SOURCES
  hpi.h
  hpi_classes.h
  hpi.cpp
  hpi_classes.cpp
  "${CMAKE_CURRENT_BINARY_DIR}/swigpyrun.h"
)

# the hpi sources contitute a library:

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

# the next section deals with setting up hsi, the hugin scripting
# interface. This part generates the hsi Python module.

# some hugin headers use a technique for generating accessors
# to image variables which SWIG can't process. These sections
# of code have to be C-preprocessed so that SWIG recognizes the
# declarations they make and wrap them. The preprocessing is
# linited to sections of these headers, as will become clearer
# when you read on.

# first we define a variable for the hugin_base path

SET(HUGIN_BASE_DIR "${PROJECT_SOURCE_DIR}/src/hugin_base")

# to preprocess C/C++ code, we need the
# adequate flags which differ from MSVC to gcc:

IF(MSVC)
  SET(PREPROCESSOR_FLAGS "/EP")
ELSE()
  SET(PREPROCESSOR_FLAGS "-E")
ENDIF()

# we put together the compiler command, the preprocess-only flag
# and the definition of _HSI_IGNORE_SECTION to exclude sections of
# the preprocessed files

SET(HSI_C_PREPROCESS
    ${CMAKE_CXX_COMPILER}
    ${PREPROCESSOR_FLAGS}
    "-D_HSI_IGNORE_SECTION"
)

# this macro sets up the custom command to preprocess ${in}
# into ${out} and allows to specify additional dependencies,
# in this case that will be image_variables.h

MACRO(preprocess in out depends)
ADD_CUSTOM_COMMAND(OUTPUT ${out}
    COMMAND ${HSI_C_PREPROCESS} ${in} > ${out}
    DEPENDS ${in} ${depends}
)
SET_SOURCE_FILES_PROPERTIES(${out} GENERATED)
ENDMACRO(preprocess)

# we C-preprocess the sections from accessor-generating
# headers that SWIG needs to see, ignoring the parts
# where _HSI_IGNORE_SECTION is define. The resulting
# preprocessed code is put into the target tree.

preprocess("${HUGIN_BASE_DIR}/panodata/ImageVariableGroup.h"
           "${CMAKE_CURRENT_BINARY_DIR}/hsi_ImageVariableGroup.h"
           "${HUGIN_BASE_DIR}/panodata/image_variables.h"
)

preprocess("${HUGIN_BASE_DIR}/panodata/Panorama.h"
           "${CMAKE_CURRENT_BINARY_DIR}/hsi_Panorama.h"
           "${HUGIN_BASE_DIR}/panodata/image_variables.h"
)

preprocess("${HUGIN_BASE_DIR}/panodata/PanoramaData.h"
           "${CMAKE_CURRENT_BINARY_DIR}/hsi_PanoramaData.h"
           "${HUGIN_BASE_DIR}/panodata/image_variables.h"
)

preprocess("${HUGIN_BASE_DIR}/panodata/SrcPanoImage.h"
           "${CMAKE_CURRENT_BINARY_DIR}/hsi_SrcPanoImage.h"
           "${HUGIN_BASE_DIR}/panodata/image_variables.h"
)

# we make a variable containing all preprocessed header
# files that are to be wrapped

SET(hsi_preprocessed
  "${CMAKE_CURRENT_BINARY_DIR}/hsi_ImageVariableGroup.h"
  "${CMAKE_CURRENT_BINARY_DIR}/hsi_Panorama.h"
  "${CMAKE_CURRENT_BINARY_DIR}/hsi_PanoramaData.h"
  "${CMAKE_CURRENT_BINARY_DIR}/hsi_SrcPanoImage.h"
)

# and another variable with all unmodified header
# files that are to be wrapped

SET(hsi_wrappees
  "${HUGIN_BASE_DIR}/appbase/DocumentData.h"
  "${HUGIN_BASE_DIR}/panodata/PanoramaData.h"
  "${HUGIN_BASE_DIR}/panodata/ImageVariable.h"
  "${HUGIN_BASE_DIR}/panodata/ImageVariableGroup.h"
  "${HUGIN_BASE_DIR}/panodata/StandardImageVariableGroups.h"
  "${HUGIN_BASE_DIR}/panodata/ImageVariableTranslate.h"
  "${HUGIN_BASE_DIR}/panodata/Lens.h"
  "${HUGIN_BASE_DIR}/panodata/SrcPanoImage.h"
  "${HUGIN_BASE_DIR}/panodata/PanoramaVariable.h"
  "${HUGIN_BASE_DIR}/panodata/ControlPoint.h"
  "${HUGIN_BASE_DIR}/panodata/PanoramaOptions.h"
  "${HUGIN_BASE_DIR}/panodata/PanoramaData.h"
  "${HUGIN_BASE_DIR}/panodata/Panorama.h"
  "${HUGIN_BASE_DIR}/panotools/PanoToolsUtils.h"
  "${HUGIN_BASE_DIR}/panotools/PanoToolsInterface.h"
  "${HUGIN_BASE_DIR}/panotools/PanoToolsOptimizerWrapper.h"
  "${HUGIN_BASE_DIR}/algorithms/PanoramaAlgorithm.h"
  "${HUGIN_BASE_DIR}/algorithms/StitcherAlgorithm.h"
  "${HUGIN_BASE_DIR}/algorithms/basic/CalculateCPStatistics.h"
  "${HUGIN_BASE_DIR}/algorithms/basic/CalculateMeanExposure.h"
  "${HUGIN_BASE_DIR}/algorithms/basic/CalculateOptimalROI.h"
  "${HUGIN_BASE_DIR}/algorithms/basic/CalculateOptimalScale.h"
  "${HUGIN_BASE_DIR}/algorithms/basic/CalculateOverlap.h"
  "${HUGIN_BASE_DIR}/algorithms/basic/RotatePanorama.h"
  "${HUGIN_BASE_DIR}/algorithms/basic/LayerStacks.h"
  "${HUGIN_BASE_DIR}/algorithms/basic/StraightenPanorama.h"
  "${HUGIN_BASE_DIR}/algorithms/basic/TranslatePanorama.h"
  "${HUGIN_BASE_DIR}/algorithms/control_points/CleanCP.h"
  "${HUGIN_BASE_DIR}/algorithms/nona/CalculateFOV.h"
  "${HUGIN_BASE_DIR}/algorithms/nona/CenterHorizontally.h"
  "${HUGIN_BASE_DIR}/algorithms/nona/ComputeImageROI.h"
  "${HUGIN_BASE_DIR}/algorithms/nona/FitPanorama.h"
  "${HUGIN_BASE_DIR}/algorithms/nona/NonaFileStitcher.h"
  "${HUGIN_BASE_DIR}/algorithms/optimizer/PhotometricOptimizer.h"
  "${HUGIN_BASE_DIR}/algorithms/optimizer/PTOptimizer.h"
  "${HUGIN_BASE_DIR}/algorithms/point_sampler/PointSampler.h"
)

# we tell SWIG to also look in the target path

SET(CMAKE_SWIG_FLAGS "-I${CMAKE_CURRENT_BINARY_DIR}")

# now we add the dependencies of the hsi SWIG module
# from all the wrapped files.

SET(SWIG_MODULE_hsi_EXTRA_DEPS
  ${hsi_preprocessed}
  ${hsi_wrappees}
)

# finally, we specify the hsi module itself

SET_SOURCE_FILES_PROPERTIES(hsi.i PROPERTIES CPLUSPLUS ON)
SWIG_ADD_MODULE(hsi python hsi.i)
SWIG_LINK_LIBRARIES(hsi ${PYTHON_LIBRARIES} ${common_libs})

# The remainder defines where the files are installed.
# TODO this section is incomplete - non-windows install is missing

IF(WIN32)
    INSTALL(TARGETS ${SWIG_MODULE_hsi_REAL_NAME} DESTINATION ${BINDIR})
    INSTALL(FILES hpi.py DESTINATION ${BINDIR})
    INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/hsi.py DESTINATION ${BINDIR})
ELSE()
    # Linux/MacOS
    # install Python related files into folder depending on used Python version

    EXECUTE_PROCESS( COMMAND ${PYTHON_EXECUTABLE} -c "from distutils.sysconfig import get_python_lib; print(get_python_lib(1))"
                     OUTPUT_VARIABLE pyinstalldir
                     OUTPUT_STRIP_TRAILING_WHITESPACE)

    # replaced by the above EXECUTE_PROCESS command.
    #SET(pynstalldir
    #    ${LIBDIR}/python${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}/dist-packages
    #    )

    MESSAGE("-- Install Python libs into ${pyinstalldir}")

    INSTALL(TARGETS ${SWIG_MODULE_hsi_REAL_NAME}
            LIBRARY DESTINATION ${pyinstalldir}
            NAMELINK_SKIP
            )

    INSTALL(PROGRAMS hpi.py
            DESTINATION ${pyinstalldir}
            )

    INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/hsi.py
            DESTINATION ${pyinstalldir}
            )

ENDIF()

ADD_SUBDIRECTORY(plugins)
ADD_SUBDIRECTORY(plugins-dev)
