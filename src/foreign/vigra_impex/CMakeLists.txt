
#build a shared library
add_library(huginvigraimpex STATIC bmp.cxx byteorder.cxx codecmanager.cxx gif.cxx imageinfo.cxx jpeg.cxx png.cxx pnm.cxx sun.cxx tiff.cxx viff.cxx void_vector.cxx iccjpeg.c rgbe.c hdr.cxx exr.cxx)

#install(TARGETS huginjhead DESTINATION lib)