
# a list of all files belonging to hugin base library

# unused sources
# algorithms/external/PTStitcherStitcher.cpp
#algorithms/external/PTmenderStitcher.cpp

SET(HUGIN_BASE_SRC
algorithms/nona/NonaFileStitcher.cpp
algorithms/nona/NonaImageStitcher.cpp
algorithms/basic/CalculateCPStatistics.cpp
algorithms/basic/CalculateMeanExposure.cpp
algorithms/basic/CalculateOptimalScale.cpp
algorithms/basic/CalculateOptimalROI.cpp
algorithms/basic/RotatePanorama.cpp
algorithms/basic/StraightenPanorama.cpp
algorithms/nona/CalculateFOV.cpp
algorithms/nona/CenterHorizontally.cpp
algorithms/nona/FitPanorama.cpp
algorithms/nona/ComputeImageROI.cpp
algorithms/optimizer/ImageGraph.cpp
algorithms/optimizer/PhotometricOptimizer.cpp
algorithms/optimizer/PTOptimizer.cpp
algorithms/panorama_makefile/PanoramaMakefileExport.cpp
algorithms/point_sampler/PointSampler.cpp
algorithms/control_points/CleanCP.cpp
appbase/ExternalProgram.cpp
appbase/ExternalProgramSetup.cpp
appbase/ProgressDisplay.cpp
appbase/ProgressDisplayOld.cpp
appbase/ProgressReporterOld.cpp
huginapp/CachedImageRemapper.cpp
huginapp/ImageCache.cpp
hugin_math/eig_jacobi.cpp
hugin_math/graph.cpp
hugin_math/Matrix3.cpp
hugin_math/Vector3.cpp
hugin_utils/utils.cpp
hugin_utils/platform.cpp
nona/SpaceTransform.cpp
nona/Stitcher1.cpp
nona/Stitcher2.cpp
nona/Stitcher3.cpp
nona/Stitcher4.cpp
nona/Stitcher.cpp
panodata/ControlPoint.cpp
panodata/Lens.cpp
panodata/PanoImage.cpp
panodata/Panorama.cpp
panodata/PanoramaOptions.cpp
panodata/PanoramaVariable.cpp
panodata/PTScriptParsing.cpp
panodata/SrcPanoImage.cpp
panodata/ImageVariableGroup.cpp
panodata/StandardImageVariableGroups.cpp
panotools/PanoToolsInterface.cpp
panotools/PanoToolsOptimizerWrapper.cpp
panotools/PanoToolsUtils.cpp
panotools/PanoToolsTransformGPU.cpp
vigra_ext/emor.cpp
vigra_ext/MultiThreadOperations.cpp
vigra_ext/ImageTransformsGPU.cpp
)

SET(HUGIN_BASE_HEADER
algorithm/PanoramaAlgorithm.h
algorithms/nona/NonaFileStitcher.h
algorithms/nona/NonaImageStitcher.h
algorithms/basic/CalculateCPStatistics.h
algorithms/basic/CalculateMeanExposure.h
algorithms/basic/CalculateOptimalScale.h
algorithms/basic/CalculateOptimalROI.h
algorithms/basic/RotatePanorama.h
algorithms/basic/StraightenPanorama.h
algorithms/nona/CalculateFOV.h
algorithms/nona/CenterHorizontally.h
algorithms/nona/FitPanorama.h
algorithms/nona/ComputeImageROI.h
algorithms/optimizer/ImageGraph.h
algorithms/optimizer/PhotometricOptimizer.h
algorithms/optimizer/PTOptimizer.h
algorithms/panorama_makefile/PanoramaMakefileExport.h
algorithms/point_sampler/PointSampler.h
algorithms/control_points/CleanCP.h
appbase/ExternalProgram.h
appbase/ExternalProgramSetup.h
appbase/ProgressDisplay.h
appbase/ProgressDisplayOld.h
appbase/ProgressReporterOld.h
huginapp/CachedImageRemapper.h
huginapp/ImageCache.h
hugin_math/eig_jacobi.h
hugin_math/graph.h
hugin_math/Matrix3.h
hugin_math/Vector3.h
hugin_utils/utils.h
hugin_utils/platform.h
nona/SpaceTransform.h
nona/Stitcher.h
panodata/ControlPoint.h
panodata/Lens.h
panodata/PanoramaData.h
panodata/PanoImage.h
panodata/Panorama.h
panodata/PanoramaOptions.h
panodata/PanoramaVariable.h
panodata/PTScriptParsing.h
panodata/SrcPanoImage.h
panodata/ImageVariableGroup.h
panodata/StandardImageVariableGroups.h
panotools/PanoToolsInterface.h
panotools/PanoToolsOptimizerWrapper.h
panotools/PanoToolsUtils.h
vigra_ext/emor.h
vigra_ext/MultiThreadOperations.h
vigra_ext/ImageTransformsGPU.h
)

IF (${HUGIN_SHARED_LIBS})
  add_library(huginbase SHARED ${HUGIN_BASE_SRC} ${HUGIN_BASE_HEADER})
  target_link_libraries(huginbase ${Boost_LIBRARIES})
  set_target_properties(huginbase PROPERTIES VERSION ${HUGIN_LIB_VERSION})
  install(TARGETS huginbase DESTINATION ${LIBDIR})
ELSE (${HUGIN_SHARED_LIBS})
  add_library(huginbase STATIC ${HUGIN_BASE_SRC} ${HUGIN_BASE_HEADER})
ENDIF (${HUGIN_SHARED_LIBS})

add_subdirectory(test)

TARGET_LINK_LIBRARIES(huginbase huginlevmar huginvigraimpex
        ${Boost_LIBRARIES} ${EXIV2_LIBRARIES} ${PANO_LIBRARIES}
        ${TIFF_LIBRARIES} ${LAPACK_LIBRARIES} ${GLEW_LIBRARIES}
        ${OPENGL_LIBRARIES})
