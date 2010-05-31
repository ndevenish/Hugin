#enblend_LDADD = vigra_impex/libvigra_impex.a -lpng -ljpeg -ltiff

include_directories(../zthread/include)

add_executable(panomatic PanoDetector.cpp PanoDetectorLogic.cpp TestCode.cpp Utils.cpp main.cpp ImageImport.h
                         KDTree.h KDTreeImpl.h PanoDetector.h PanoDetectorDefs.h TestCode.h Tracer.h Utils.h
)

#target_link_libraries(panomatic localfeatures vigraimpex ZThread pthread)
IF(WIN32)
#  target_link_libraries(panomatic localfeatures ${VIGRA_LIBRARIES} ZThread)
  target_link_libraries(panomatic localfeatures ZThread ${image_libs})
ELSE(WIN32)
#  target_link_libraries(panomatic localfeatures ${VIGRA_LIBRARIES} ZThread pthread)
  target_link_libraries(panomatic localfeatures ZThread pthread ${image_libs})
ENDIF(WIN32)

install(TARGETS panomatic DESTINATION ${BINDIR})

