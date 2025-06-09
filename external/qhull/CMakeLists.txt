# CMakeLists.txt -- This is based on the original file from qhull distribution.

project(qhull)

cmake_minimum_required(VERSION 3.0)
set(qhull_VERSION  "8.0.2")  # Advance every release

include(CMakeDependentOption)

# ---------------------------------------
# Define library source files and variables
# ---------------------------------------

set(
    libqhullr_HEADERS
        src/libqhull_r/libqhull_r.h
        src/libqhull_r/geom_r.h
        src/libqhull_r/io_r.h
        src/libqhull_r/mem_r.h
        src/libqhull_r/merge_r.h
        src/libqhull_r/poly_r.h
        src/libqhull_r/qhull_ra.h
        src/libqhull_r/qset_r.h
        src/libqhull_r/random_r.h
        src/libqhull_r/stat_r.h
        src/libqhull_r/user_r.h
)
set(
    libqhullr_SOURCES
        src/libqhull_r/global_r.c
        src/libqhull_r/stat_r.c
        src/libqhull_r/geom2_r.c
        src/libqhull_r/poly2_r.c
        src/libqhull_r/merge_r.c
        src/libqhull_r/libqhull_r.c
        src/libqhull_r/geom_r.c
        src/libqhull_r/poly_r.c
        src/libqhull_r/qset_r.c
        src/libqhull_r/mem_r.c
        src/libqhull_r/random_r.c
        src/libqhull_r/usermem_r.c
        src/libqhull_r/userprintf_r.c
        src/libqhull_r/io_r.c
        src/libqhull_r/user_r.c
        src/libqhull_r/rboxlib_r.c
        src/libqhull_r/userprintf_rbox_r.c
        ${libqhullr_HEADERS}
)

set(
    libqhullcpp_HEADERS
        src/libqhullcpp/Coordinates.h
        src/libqhullcpp/functionObjects.h
        src/libqhullcpp/PointCoordinates.h
        src/libqhullcpp/Qhull.h
        src/libqhullcpp/QhullError.h
        src/libqhullcpp/QhullFacet.h
        src/libqhullcpp/QhullFacetList.h
        src/libqhullcpp/QhullFacetSet.h
        src/libqhullcpp/QhullHyperplane.h
        src/libqhullcpp/QhullIterator.h
        src/libqhullcpp/QhullLinkedList.h
        src/libqhullcpp/QhullPoint.h
        src/libqhullcpp/QhullPoints.h
        src/libqhullcpp/QhullPointSet.h
        src/libqhullcpp/QhullQh.h
        src/libqhullcpp/QhullRidge.h
        src/libqhullcpp/QhullSet.h
        src/libqhullcpp/QhullSets.h
        src/libqhullcpp/QhullStat.h
        src/libqhullcpp/QhullUser.h
        src/libqhullcpp/QhullVertex.h
        src/libqhullcpp/QhullVertexSet.h
        src/libqhullcpp/RboxPoints.h
        src/libqhullcpp/RoadError.h
        src/libqhullcpp/RoadLogEvent.h
)

set(
    libqhullcpp_SOURCES
        src/libqhullcpp/Coordinates.cpp
        src/libqhullcpp/PointCoordinates.cpp
        src/libqhullcpp/Qhull.cpp
        src/libqhullcpp/QhullFacet.cpp
        src/libqhullcpp/QhullFacetList.cpp
        src/libqhullcpp/QhullFacetSet.cpp
        src/libqhullcpp/QhullHyperplane.cpp
        src/libqhullcpp/QhullPoint.cpp
        src/libqhullcpp/QhullPointSet.cpp
        src/libqhullcpp/QhullPoints.cpp
        src/libqhullcpp/QhullQh.cpp
        src/libqhullcpp/QhullRidge.cpp
        src/libqhullcpp/QhullSet.cpp
        src/libqhullcpp/QhullStat.cpp
        src/libqhullcpp/QhullUser.cpp
        src/libqhullcpp/QhullVertex.cpp
        src/libqhullcpp/QhullVertexSet.cpp
        src/libqhullcpp/RboxPoints.cpp
        src/libqhullcpp/RoadError.cpp
        src/libqhullcpp/RoadLogEvent.cpp
        ${libqhullcpp_HEADERS}
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

set(qhull_CPP qhullcpp)
set(qhull_STATICR qhullstatic_r)

set(qhull_TARGETS_STATIC ${qhull_CPP} ${qhull_STATICR})

# ---------------------------------------
# Define static libraries qhullstatic_r (reentrant)
# ---------------------------------------

add_library(${qhull_STATICR} STATIC ${libqhullr_SOURCES})
set_target_properties(${qhull_STATICR} PROPERTIES
    VERSION ${qhull_VERSION}
    OUTPUT_NAME "${qhull_STATICR}$<$<CONFIG:Debug>:d>")

if(UNIX)
    target_link_libraries(${qhull_STATICR} m)
endif(UNIX)

# ---------------------------------------
# Define C++ static library qhullcpp
# ---------------------------------------

add_library(${qhull_CPP} STATIC ${libqhullcpp_SOURCES})
set_target_properties(${qhull_CPP} PROPERTIES
    VERSION ${qhull_VERSION}
    OUTPUT_NAME "${qhull_CPP}$<$<CONFIG:Debug>:_d>"
    POSITION_INDEPENDENT_CODE "TRUE")
