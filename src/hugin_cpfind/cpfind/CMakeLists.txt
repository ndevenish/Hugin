add_executable(cpfind PanoDetector.cpp PanoDetectorLogic.cpp TestCode.cpp Utils.cpp main.cpp ImageImport.h
                         KDTree.h KDTreeImpl.h PanoDetector.h PanoDetectorDefs.h TestCode.h Tracer.h Utils.h
)

IF(WIN32)
	target_link_libraries(cpfind localfeatures ${ZTHREAD_LIBRARIES} ${image_libs} ${common_libs} celeste)
ELSE(WIN32)
IF(FLANN_FOUND)
        target_link_libraries(cpfind localfeatures ${ZTHREAD_LIBRARIES} pthread ${image_libs} ${common_libs} celeste ${FLANN_LIBRARIES})
ELSE(FLANN_FOUND)
	target_link_libraries(cpfind localfeatures ${ZTHREAD_LIBRARIES} pthread ${image_libs} ${common_libs} celeste)
ENDIF(FLANN_FOUND)
ENDIF(WIN32)

install(TARGETS cpfind DESTINATION ${BINDIR})

