

SET(ANN_SRC ANN.cpp brute.cpp kd_tree.cpp kd_util.cpp kd_split.cpp
            kd_dump.cpp kd_search.cpp kd_pr_search.cpp kd_fix_rad_search.cpp
            bd_tree.cpp bd_search.cpp bd_pr_search.cpp bd_fix_rad_search.cpp
	    perf.cpp)

SET(ANN_HEADERS ANN.h ANNperf.h ANNx.h kd_tree.h kd_split.h kd_util.h kd_search.h
                 kd_pr_search.h kd_fix_rad_search.h pr_queue.h pr_queue_k.h)


IF (${HUGIN_SHARED_LIBS})
  add_library(huginANN SHARED ${ANN_SRC} ${ANN_HEADERS})
  set_target_properties(huginANN PROPERTIES VERSION ${HUGIN_LIB_VERSION})
  install(TARGETS huginANN DESTINATION ${LIBDIR})
ELSE (${HUGIN_SHARED_LIBS})
  add_library(huginANN STATIC ${ANN_SRC} ${ANN_HEADERS})
ENDIF (${HUGIN_SHARED_LIBS})

