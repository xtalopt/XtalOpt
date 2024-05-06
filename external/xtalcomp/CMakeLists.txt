#cmake_minimum_required (VERSION 2.6...3.27)
#project (XtalComp)

add_library (XtalComp xtalcomp.cpp xctransform.cpp xcvector.h xcmatrix.h
  stablecomparison.h)

#add_executable (test test.cpp)

#target_link_libraries (test XtalComp)

