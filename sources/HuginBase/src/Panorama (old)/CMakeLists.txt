
add_library(huginpanorama SHARED Panorama.cpp PanoramaMemento.cpp PanoImage.cpp
                                 PanoToolsInterface.cpp SpaceTransform.cpp
                                 PTOptimise.cpp ImageGraph.cpp Stitcher.cpp
                                 utils.cpp PhotometricOptimizer.cpp
                                 Stitcher1.cpp Stitcher2.cpp Stitcher3.cpp
                                 Stitcher4.cpp)

install(TARGETS huginpanorama
        DESTINATION lib)