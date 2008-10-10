# Windows needs an #include "direct.h
# Linux and Apple are both OK without
# Set this up here
IF(WIN32)
	ADD_CUSTOM_COMMAND(
	OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/Config.h
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/Config.h.win32 ${CMAKE_CURRENT_SOURCE_DIR}/Config.h
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Config.h.win32
	)
ELSE (WIN32)
	ADD_CUSTOM_COMMAND(
	OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/Config.h
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/Config.h.linux ${CMAKE_CURRENT_SOURCE_DIR}/Config.h
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Config.h.linux
	)
ENDIF(WIN32)

# a list of all files belonging to celeste library

SET(CELESTE_SRC
CelesteGlobals.cpp
CelesteGlobals.h
Celeste.cpp
Celeste.h
Config.h
ContrastFilter.cpp
ContrastFilter.h
Gabor.cpp
Gabor.h
GaborGlobal.h
GaborFilter.cpp
GaborFilter.h
GaborJet.cpp
GaborJet.h
ImageFile.cpp
ImageFile.h
LogPolar.cpp
LogPolar.h
PGMImage.cpp
PGMImage.h
svm.cpp
svm.h
Utilities.cpp
Utilities.h
)

IF (${HUGIN_SHARED_LIBS})
	ADD_LIBRARY(celeste SHARED ${CELESTE_SRC})
	set_target_properties(celeste PROPERTIES VERSION ${HUGIN_LIB_VERSION})
	install(TARGETS celeste DESTINATION ${LIBDIR})
ELSE (${HUGIN_SHARED_LIBS})
	ADD_LIBRARY(celeste STATIC ${CELESTE_SRC})
ENDIF (${HUGIN_SHARED_LIBS})

# standalone
ADD_EXECUTABLE(celeste_standalone ../hugin_config.h  Main.cpp )
target_link_libraries( celeste_standalone celeste ${image_libs} )

SET (CELESTE_MODEL
	data/celeste.model
)

INSTALL(TARGETS celeste_standalone DESTINATION ${BINDIR}) 
INSTALL(FILES ${CELESTE_MODEL} DESTINATION ${HUGINDATADIR}/xrc/data)

add_subdirectory(training)
