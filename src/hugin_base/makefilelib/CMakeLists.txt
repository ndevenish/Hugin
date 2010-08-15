SET(MAKEFILELIB_SRC Comment.cpp MakefileItem.cpp  Variable.cpp VariableRef.cpp 
 Makefile.cpp  Rule.cpp VariableDef.cpp AutoVariable.cpp Conditional.cpp) 
SET(MAKEFILELIB_HEADER Comment.h Makefile.h Rule.h Variable.h 
 MakefileItem.h VariableDef.h VariableRef.h AutoVariable.h Newline.h Conditional.h 
 char_type.h StringAdapter.h Manager.h Anything.h)

# use -Wall not on Windows, there are thousands of pointless padding warnings.
IF(NOT WIN32)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
ENDIF(NOT WIN32)

IF (${HUGIN_SHARED_LIBS})
  add_library(makefilelib SHARED ${MAKEFILELIB_SRC} ${MAKEFILELIB_HEADER})
  set_target_properties(makefilelib PROPERTIES VERSION ${HUGIN_LIB_VERSION})
  IF(WIN32)
    install(TARGETS makefilelib RUNTIME DESTINATION ${BINDIR})
  ELSE(WIN32)
    install(TARGETS makefilelib DESTINATION ${LIBDIR})
  ENDIF(WIN32)
ELSE (${HUGIN_SHARED_LIBS})
  add_library(makefilelib STATIC ${MAKEFILELIB_SRC} ${MAKEFILELIB_HEADER})
ENDIF (${HUGIN_SHARED_LIBS})

# additional libs  
target_link_libraries(makefilelib ${Boost_LIBRARIES})

# test_util
IF(WIN32)
add_library(test_util STATIC test_util_win32.cpp test_util.h)
ELSE(WIN32)
add_library(test_util STATIC test_util.cpp test_util.h)
ENDIF(WIN32)

# tester executable
add_executable(test_simple main.cpp)
target_link_libraries(test_simple makefilelib) 

add_executable(test_filenames test_filenames.cpp)
target_link_libraries(test_filenames test_util makefilelib ${Boost_LIBRARIES})

add_executable(test_makefilelib test_makefilelib.cpp)
target_link_libraries(test_makefilelib test_util makefilelib ${Boost_LIBRARIES})

add_executable(example example.cpp)
target_link_libraries(example makefilelib)