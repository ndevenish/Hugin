
SET(VIGRA_IMPEX_SRC bmp.cxx byteorder.cxx codecmanager.cxx gif.cxx imageinfo.cxx
                    jpeg.cxx png.cxx pnm.cxx sun.cxx tiff.cxx viff.cxx
                    void_vector.cxx iccjpeg.c rgbe.c hdr.cxx exr.cxx)
SET(VIGRA_IMPEX_HEADER bmp.hxx byteorder.hxx codecmanager.hxx gif.hxx 
                    jpeg.hxx png.hxx pnm.hxx sun.hxx tiff.hxx viff.hxx
                    void_vector.hxx iccjpeg.h rgbe.h hdr.hxx exr.hxx)
IF (${HUGIN_SHARED_LIBS})
  add_library(huginvigraimpex SHARED ${VIGRA_IMPEX_SRC} ${VIGRA_IMPEX_HEADER})
  set_target_properties(huginvigraimpex PROPERTIES VERSION ${HUGIN_LIB_VERSION})
  install(TARGETS huginvigraimpex DESTINATION ${LIBDIR})
ELSE (${HUGIN_SHARED_LIBS})
  add_library(huginvigraimpex STATIC ${VIGRA_IMPEX_SRC} ${VIGRA_IMPEX_HEADER})
ENDIF (${HUGIN_SHARED_LIBS})

TARGET_LINK_LIBRARIES(huginvigraimpex ${JPEG_LIBRARIES} ${OPENEXR_LIBRARIES} ${PNG_LIBRARIES} ${TIFF_LIBRARIES})
