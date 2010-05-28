set( generic_SRCS 
     optbase.cpp
     queuemanager.cpp
     structure.cpp
     tracker.cpp
     xtal.cpp
     optimizer.cpp
     bt.cpp)

string(REPLACE ".cpp" ".h" hdr_list "${generic_SRCS}")
qt4_wrap_cpp(moc_files ${hdr_list})
add_library(globalsearch STATIC ${generic_SRCS} ${moc_files})
target_link_libraries( globalsearch
		       ${LINK_LIBS} 
		       ${QT_QTCORE_LIBRARY} 
		       ${QT_QTGUI_LIBRARY} 
		       ${OPENBABEL2_LIBRARIES})
