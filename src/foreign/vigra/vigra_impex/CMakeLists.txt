
SET(VIGRA_IMPEX_SRC bmp.cxx byteorder.cxx codecmanager.cxx gif.cxx imageinfo.cxx
                    jpeg.cxx png.cxx pnm.cxx sun.cxx tiff.cxx viff.cxx
                    void_vector.cxx iccjpeg.c rgbe.c hdr.cxx exr.cxx)
IF (${HUGIN_SHARED_LIBS})
  add_library(huginvigraimpex SHARED ${VIGRA_IMPEX_SRC})
  install(TARGETS huginvigraimpex DESTINATION ${LIBDIR})
ELSE (${HUGIN_SHARED_LIBS})
  add_library(huginvigraimpex STATIC ${VIGRA_IMPEX_SRC})
ENDIF (${HUGIN_SHARED_LIBS})
