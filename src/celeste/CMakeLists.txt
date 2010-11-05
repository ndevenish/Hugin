#    Copyright (C) 2008 by Tim Nugent
#    timnugent@gmail.com
# 
#    This file is part of hugin.
# 
#    Hugin is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 2 of the License, or
#    (at your option) any later version.
#  
#    Hugin is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#  
#    You should have received a copy of the GNU General Public License
#    along with Hugin  If not, see <http://www.gnu.org/licenses/>.
 
IF(NOT WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
ENDIF(NOT WIN32)

# a list of all files belonging to celeste library

SET(CELESTE_SRC
CelesteGlobals.cpp
CelesteGlobals.h
Celeste.cpp
Celeste.h
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
    # messages are quite obtrusive in windows cmakesetup.exe
    IF (NOT WIN32)
	#message("Using these CMAKE_CXX_FLAGS : ${CMAKE_CXX_FLAGS}")
	ENDIF (NOT WIN32)
	ADD_LIBRARY(celeste SHARED ${CELESTE_SRC})
	TARGET_LINK_LIBRARIES(celeste huginbase ${common_libs})
	set_target_properties(celeste PROPERTIES VERSION ${HUGIN_LIB_VERSION})
    IF(WIN32)
      install(TARGETS celeste RUNTIME DESTINATION ${BINDIR})
    ELSEIF(${HUGIN_LIBS_PRIVATE_DIR})
      install(TARGETS celeste LIBRARY DESTINATION ${LIBDIR}/hugin NAMELINK_SKIP)
    ELSE(WIN32)
      install(TARGETS celeste LIBRARY DESTINATION ${LIBDIR} NAMELINK_SKIP)
    ENDIF(WIN32)
ELSE (${HUGIN_SHARED_LIBS})
    # messages are quite obtrusive in windows cmakesetup.exe
    IF (NOT WIN32)
    	#message("Using these CMAKE_CXX_FLAGS : ${CMAKE_CXX_FLAGS}")
	ENDIF (NOT WIN32)	
	ADD_LIBRARY(celeste STATIC ${CELESTE_SRC})
	TARGET_LINK_LIBRARIES(celeste huginbase ${common_libs})
	set_target_properties(celeste PROPERTIES VERSION ${HUGIN_LIB_VERSION})
ENDIF (${HUGIN_SHARED_LIBS})

# standalone
ADD_EXECUTABLE(celeste_standalone ../hugin_config.h  Main.cpp )
target_link_libraries( celeste_standalone celeste ${image_libs} ${common_libs})

SET (CELESTE_MODEL
	data/celeste.model
)

INSTALL(TARGETS celeste_standalone DESTINATION ${BINDIR}) 
INSTALL(FILES ${CELESTE_MODEL} DESTINATION ${INSTALL_DATA_DIR})

#add_subdirectory(training)
