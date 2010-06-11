set( globalsearch_SRCS 
     optbase.cpp
     queuemanager.cpp
     structure.cpp
     tracker.cpp
     optimizer.cpp
     bt.cpp)

string(REPLACE ".cpp" ".h" hdr_list "${globalsearch_SRCS}")
qt4_wrap_cpp(moc_files ${hdr_list})
add_library(globalsearch STATIC ${globalsearch_SRCS} ${moc_files})
target_link_libraries( globalsearch
		       ${LINK_LIBS} 
		       ${QT_QTCORE_LIBRARY} 
		       ${QT_QTGUI_LIBRARY} 
		       ${OPENBABEL2_LIBRARIES})