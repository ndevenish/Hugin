SET(MAKEFILELIB_SRC Comment.cpp MakefileItem.cpp  Variable.cpp VariableRef.cpp 
 Filename.cpp  Makefile.cpp  Rule.cpp VariableDef.cpp AutoVariable.cpp Conditional.cpp) 
SET(MAKEFILELIB_HEADER Comment.h Makefile.h Rule.h Variable.h Filename.h 
 MakefileItem.h VariableDef.h VariableRef.h AutoVariable.h Newline.h Conditional.h char_type.h StringAdapter.h)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

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



# tester executable
add_executable(test_simple main.cpp)
target_link_libraries(test_simple makefilelib) 

add_executable(test_filenames test_filenames.cpp)
target_link_libraries(test_filenames makefilelib ${Boost_LIBRARIES})

add_executable(test_makefilelib test_makefilelib.cpp)
target_link_libraries(test_makefilelib makefilelib ${Boost_LIBRARIES})