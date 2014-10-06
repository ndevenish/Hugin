add_executable(cpfind PanoDetector.cpp PanoDetectorLogic.cpp TestCode.cpp Utils.cpp main.cpp ImageImport.h
                         KDTree.h KDTreeImpl.h PanoDetector.h PanoDetectorDefs.h TestCode.h Tracer.h Utils.h
)

IF(FLANN_FOUND)
    target_link_libraries(cpfind localfeatures ${image_libs} ${common_libs} celeste ${FLANN_LIBRARIES})
ELSE(FLANN_FOUND)
	target_link_libraries(cpfind localfeatures ${image_libs} ${common_libs} celeste)
ENDIF(FLANN_FOUND)

install(TARGETS cpfind DESTINATION ${BINDIR})

