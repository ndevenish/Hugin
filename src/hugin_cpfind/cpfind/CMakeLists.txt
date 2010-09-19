add_executable(cpfind PanoDetector.cpp PanoDetectorLogic.cpp TestCode.cpp Utils.cpp main.cpp ImageImport.h
                         KDTree.h KDTreeImpl.h PanoDetector.h PanoDetectorDefs.h TestCode.h Tracer.h Utils.h
)

IF(WIN32)
	target_link_libraries(cpfind localfeatures ZThread ${image_libs} ${common_libs} )
ELSE(WIN32)
	target_link_libraries(cpfind localfeatures ZThread pthread ${image_libs} ${common_libs} )
ENDIF(WIN32)

install(TARGETS cpfind DESTINATION ${BINDIR})

