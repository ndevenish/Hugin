
add_executable(fulla fulla.cpp)
target_link_libraries(fulla ${common_libs} huginlensdb ${image_libs})

add_executable(nona nona.cpp)
target_link_libraries(nona ${common_libs} ${image_libs} ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES} ${GLEW_LIBRARIES})

#add_executable(autocreate autocreate.cpp)
#target_link_libraries(autocreate ${common_libs} ${image_libs})

add_executable(autooptimiser autooptimiser.cpp ExtractPoints.h)
target_link_libraries(autooptimiser ${common_libs} ${image_libs})

add_executable(vig_optimize vig_optimize.cpp ExtractPoints.h)
target_link_libraries(vig_optimize ${common_libs} ${image_libs})

add_executable(align_image_stack align_image_stack.cpp)
target_link_libraries(align_image_stack ${common_libs} ${image_libs} ${GLEW_LIBRARIES} ${GLUT_LIBRARIES})

add_executable(linefind linefind.cpp)
target_link_libraries(linefind ${common_libs} ${image_libs} huginlines)

add_executable(geocpset geocpset.cpp)
target_link_libraries(geocpset ${common_libs})

add_executable(tca_correct tca_correct.cpp)
target_link_libraries(tca_correct ${common_libs} ${image_libs})

add_executable(pto2mk pto2mk.cpp)
target_link_libraries(pto2mk ${common_libs} ${image_libs} )

add_executable(cpclean cpclean.cpp)
target_link_libraries(cpclean ${common_libs} )

add_executable(checkpto checkpto.cpp)
target_link_libraries(checkpto ${common_libs} )

add_executable(pto_merge pto_merge.cpp)
target_link_libraries(pto_merge ${common_libs} )

add_executable(pto_template pto_template.cpp)
target_link_libraries(pto_template ${common_libs} )

add_executable(pto_mask pto_mask.cpp)
target_link_libraries(pto_mask ${common_libs} )

add_executable(pto_gen pto_gen.cpp)
target_link_libraries(pto_gen ${common_libs} )

add_executable(pano_trafo pano_trafo.cpp)
target_link_libraries(pano_trafo ${common_libs} )

add_executable(pano_modify pano_modify.cpp)
target_link_libraries(pano_modify ${common_libs} )

add_executable(pto_var pto_var.cpp ParseExp.cpp ParseExp.h)
target_link_libraries(pto_var ${common_libs})

add_executable(pto_lensstack pto_lensstack.cpp)
target_link_libraries(pto_lensstack ${common_libs})

add_executable(hugin_hdrmerge hugin_hdrmerge.cpp ../deghosting/deghosting.cpp ../deghosting/deghosting.h ../deghosting/khan.h ../deghosting/support.h ../deghosting/algtinyvector.h)
target_link_libraries(hugin_hdrmerge ${common_libs} ${image_libs} )

install(TARGETS nona vig_optimize autooptimiser fulla align_image_stack linefind geocpset
        tca_correct pto2mk cpclean checkpto hugin_hdrmerge pano_trafo pano_modify pto_merge 
        pto_gen pto_var pto_lensstack pto_template pto_mask
        DESTINATION ${BINDIR})

IF(UNIX)
  INSTALL(FILES pto_gen.desktop DESTINATION ${DATADIR}/applications)
ENDIF(UNIX)

