
# a list of all files belonging to hugin base library

SET(HUGIN_BASE_SRC
algorithms/nona/NonaFileStitcher.cpp
algorithms/basic/CalculateCPStatistics.cpp
algorithms/basic/CalculateMeanExposure.cpp
algorithms/basic/CalculateOptimalScale.cpp
algorithms/basic/CalculateOptimalROI.cpp
algorithms/basic/RotatePanorama.cpp
algorithms/basic/TranslatePanorama.cpp
algorithms/basic/StraightenPanorama.cpp
algorithms/basic/CalculateOverlap.cpp
algorithms/basic/LayerStacks.cpp
algorithms/nona/CalculateFOV.cpp
algorithms/nona/CenterHorizontally.cpp
algorithms/nona/FitPanorama.cpp
algorithms/nona/ComputeImageROI.cpp
algorithms/optimizer/ImageGraph.cpp
algorithms/optimizer/PhotometricOptimizer.cpp
algorithms/optimizer/PTOptimizer.cpp
algorithms/panorama_makefile/PanoramaMakefilelibExport.cpp
algorithms/assistant_makefile/AssistantMakefilelibExport.cpp
algorithms/point_sampler/PointSampler.cpp
algorithms/control_points/CleanCP.cpp
appbase/ProgressDisplay.cpp
appbase/ProgressDisplayOld.cpp
appbase/ProgressReporterOld.cpp
huginapp/CachedImageRemapper.cpp
huginapp/ImageCache.cpp
hugin_math/eig_jacobi.cpp
hugin_math/graph.cpp
hugin_math/Matrix3.cpp
hugin_math/Vector3.cpp
hugin_utils/alphanum.cpp
hugin_utils/utils.cpp
hugin_utils/platform.cpp
lensdb/LensDB.cpp
nona/SpaceTransform.cpp
nona/Stitcher1.cpp
nona/Stitcher2.cpp
nona/Stitcher3.cpp
nona/Stitcher4.cpp
nona/Stitcher.cpp
panodata/ControlPoint.cpp
panodata/Lens.cpp
panodata/Mask.cpp
panodata/Panorama.cpp
panodata/PanoramaOptions.cpp
panodata/PanoramaVariable.cpp
panodata/PTScriptParsing.cpp
panodata/SrcPanoImage.cpp
panodata/ImageVariableGroup.cpp
panodata/StandardImageVariableGroups.cpp
panodata/Exiv2Helper.cpp
panotools/PanoToolsInterface.cpp
panotools/PanoToolsOptimizerWrapper.cpp
panotools/PanoToolsUtils.cpp
panotools/PanoToolsTransformGPU.cpp
vigra_ext/emor.cpp
vigra_ext/MultiThreadOperations.cpp
vigra_ext/ImageTransformsGPU.cpp
)

SET(HUGIN_BASE_HEADER
algorithms/PanoramaAlgorithm.h
algorithms/StitcherAlgorithm.h
algorithms/nona/NonaFileStitcher.h
algorithms/basic/CalculateCPStatistics.h
algorithms/basic/CalculateMeanExposure.h
algorithms/basic/CalculateOptimalScale.h
algorithms/basic/CalculateOptimalROI.h
algorithms/basic/RotatePanorama.h
algorithms/basic/TranslatePanorama.h
algorithms/basic/StraightenPanorama.h
algorithms/basic/CalculateOverlap.h
algorithms/basic/LayerStacks.h
algorithms/nona/CalculateFOV.h
algorithms/nona/CenterHorizontally.h
algorithms/nona/FitPanorama.h
algorithms/nona/ComputeImageROI.h
algorithms/optimizer/ImageGraph.h
algorithms/optimizer/PhotometricOptimizer.h
algorithms/optimizer/PTOptimizer.h
algorithms/panorama_makefile/PanoramaMakefilelibExport.h
algorithms/assistant_makefile/AssistantMakefilelibExport.h
algorithms/point_sampler/PointSampler.h
algorithms/control_points/CleanCP.h
appbase/Command.h
appbase/DocumentData.h
appbase/ProgressDisplay.h
appbase/ProgressDisplayOld.h
appbase/ProgressReporterOld.h
huginapp/CachedImageRemapper.h
huginapp/ImageCache.h
hugin_math/eig_jacobi.h
hugin_math/graph.h
hugin_math/Matrix3.h
hugin_math/Vector3.h
hugin_utils/alphanum.h
hugin_utils/utils.h
hugin_utils/platform.h
lensdb/LensDB.h
nona/ImageRemapper.h
nona/RemappedPanoImage.h
nona/SpaceTransform.h
nona/Stitcher.h
panodata/ControlPoint.h
panodata/Lens.h
panodata/Mask.h
panodata/OptimizerSwitches.h
panodata/PanoramaData.h
panodata/Panorama.h
panodata/PanoramaOptions.h
panodata/PanoramaVariable.h
panodata/PTScriptParsing.h
panodata/SrcPanoImage.h
panodata/image_variables.h
panodata/ImageVariable.h
panodata/ImageVariableGroup.h
panodata/ImageVariableTranslate.h
panodata/StandardImageVariableGroups.h
panodata/Exiv2Helper.h
panotools/PanoToolsInterface.h
panotools/PanoToolsOptimizerWrapper.h
panotools/PanoToolsUtils.h
vigra_ext/emor.h
vigra_ext/MultiThreadOperations.h
vigra_ext/ImageTransformsGPU.h
)

IF (${HUGIN_SHARED_LIBS})
  add_library(huginbase SHARED ${HUGIN_BASE_SRC} ${HUGIN_BASE_HEADER})
  target_link_libraries(huginbase ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
  set_target_properties(huginbase PROPERTIES VERSION ${HUGIN_LIB_VERSION})
  IF(WIN32)
    install(TARGETS huginbase RUNTIME DESTINATION ${BINDIR})
  ELSEIF(${HUGIN_LIBS_PRIVATE_DIR})
    install(TARGETS huginbase LIBRARY DESTINATION ${LIBDIR}/hugin NAMELINK_SKIP)
  ELSE(WIN32)
    install(TARGETS huginbase LIBRARY DESTINATION ${LIBDIR} NAMELINK_SKIP)
  ENDIF(WIN32)
ELSE (${HUGIN_SHARED_LIBS})
  add_library(huginbase STATIC ${HUGIN_BASE_SRC} ${HUGIN_BASE_HEADER})
ENDIF (${HUGIN_SHARED_LIBS})

TARGET_LINK_LIBRARIES(huginbase huginlevmar huginvigraimpex makefilelib
        ${Boost_LIBRARIES} ${EXIV2_LIBRARIES} ${PANO_LIBRARIES}
        ${TIFF_LIBRARIES} ${LAPACK_LIBRARIES} ${GLEW_LIBRARIES}
        ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT}
        ${SQLITE3_LIBRARIES} )

INSTALL(FILES algorithms/panorama_makefile/hugin_exiftool_copy.arg DESTINATION ${INSTALL_DATA_DIR})
        
add_subdirectory(makefilelib)
add_subdirectory(lines)
