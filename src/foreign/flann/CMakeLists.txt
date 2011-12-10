
add_definitions(-D_FLANN_VERSION=1.6.11)

file(GLOB_RECURSE C_SOURCES *.cpp)
file(GLOB_RECURSE CPP_SOURCES *.cpp)

file(GLOB_RECURSE C_FLANN flann.cpp)
file(GLOB_RECURSE CPP_FLANN flann_cpp.cpp)

list(REMOVE_ITEM CPP_SOURCES ${C_FLANN})
list(REMOVE_ITEM C_SOURCES ${CPP_FLANN})

IF (${HUGIN_SHARED_LIBS})

IF(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_COMPILER_IS_GNUCC)
    add_library(flann_cpp_s STATIC ${CPP_SOURCES})
    set_target_properties(flann_cpp_s PROPERTIES COMPILE_FLAGS -fPIC)

    add_library(flann_cpp SHARED "")
    target_link_libraries(flann_cpp -Wl,-whole-archive flann_cpp_s -Wl,-no-whole-archive)
    set_target_properties(flann_cpp PROPERTIES VERSION ${HUGIN_LIB_VERSION}) 

    IF(${HUGIN_LIBS_PRIVATE_DIR})
        install(TARGETS flann_cpp LIBRARY DESTINATION ${LIBDIR}/hugin NAMELINK_SKIP ARCHIVE DESTINATION ${LIBDIR}/hugin  )
    ELSE()
        install(TARGETS flann_cpp LIBRARY DESTINATION ${LIBDIR} NAMELINK_SKIP ARCHIVE DESTINATION ${LIBDIR} )
    ENDIF()
ELSE()
    add_library(flann_cpp SHARED ${CPP_SOURCES})
    IF(WIN32)
        install(TARGETS flann_cpp RUNTIME DESTINATION ${BINDIR} ARCHIVE DESTINATION ${BINDIR} )
    ELSEIF(${HUGIN_LIBS_PRIVATE_DIR})
        install(TARGETS flann_cpp LIBRARY DESTINATION ${LIBDIR}/hugin NAMELINK_SKIP ARCHIVE DESTINATION ${LIBDIR}/hugin  )
    ELSE(WIN32)
        install(TARGETS flann_cpp LIBRARY DESTINATION ${LIBDIR} NAMELINK_SKIP ARCHIVE DESTINATION ${LIBDIR} )
    ENDIF(WIN32)

ENDIF()

ELSE(${HUGIN_SHARED_LIBS})
    add_library(flann_cpp STATIC ${CPP_SOURCES})
ENDIF(${HUGIN_SHARED_LIBS})

