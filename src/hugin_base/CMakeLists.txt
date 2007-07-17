
# a list of all files belonging to hugin base library

SET(HUGIN_BASE_SRC
algorithms/basic/CalculateCPStatistics.cpp
algorithms/basic/CalculateMeanExposure.cpp
algorithms/basic/CalculateOptimalScale.cpp
algorithms/basic/RotatePanorama.cpp
algorithms/basic/StraightenPanorama.cpp
algorithms/external/PTmenderStitcher.cpp
algorithms/external/PTStitcherStitcher.cpp
algorithms/nona/CalculateFOV.cpp
algorithms/nona/CenterHorizontally.cpp
algorithms/nona/FitPanorama.cpp
algorithms/nona/NonaFileStitcher.cpp
algorithms/nona/NonaImageStitcher.cpp
algorithms/optimizer/ImageGraph.cpp
algorithms/optimizer/PhotometricOptimizer.cpp
algorithms/optimizer/PTOptimizer.cpp
algorithms/panorama_makefile/PanoramaMakefileExport.cpp
algorithms/point_sampler/PointSampler.cpp
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
panotools/PanoToolsInterface.cpp
panotools/PanoToolsOptimizerWrapper.cpp
vigra_ext/emor.cpp
vigra_ext/MultiThreadOperations.cpp
)

add_library(huginbase SHARED ${HUGIN_BASE_SRC})
install(TARGETS huginbase DESTINATION lib)

add_executable(fulla test/fulla.cpp)
target_link_libraries(fulla ${common_libs} huginlensdb ${image_libs})

add_executable(nona test/nona.cpp)
target_link_libraries(nona ${common_libs} ${image_libs})

add_executable(autooptimiser test/autooptimiser.cpp)
target_link_libraries(autooptimiser ${common_libs} ${image_libs})

add_executable(vig_optimize test/vig_optimize.cpp)
target_link_libraries(vig_optimize ${common_libs} ${image_libs})

add_executable(open_file test/open_file.cpp)
target_link_libraries(open_file ${common_libs} ${image_libs})

