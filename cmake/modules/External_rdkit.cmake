# Written by Patrick Avery - 2017
# This module adds RDKit as a compilation target for the project
# The variables RDKit_INCLUDE_DIRS and RDKit_LIBRARIES will be
# set with the RDKit include directory and the RDKit libraries, respectively
# The include directory is automatically included.

set(_source "${CMAKE_CURRENT_SOURCE_DIR}/rdkit")
set(_build "${CMAKE_CURRENT_BINARY_DIR}/rdkit")

unset(_deps)

if(NOT Boost_FOUND)
  set(_deps "boost")
endif()

ExternalProject_Add(rdkit
  SOURCE_DIR ${_source}
  BINARY_DIR ${_build}
  CMAKE_CACHE_ARGS
    -DRDK_BUILD_PYTHON_WRAPPERS:BOOL=OFF
    -DRDK_BUILD_SLN_SUPPORT:BOOL=OFF
    -DRDK_TEST_MMFF_COMPLIANCE:BOOL=OFF
    -DRDK_BUILD_CPP_TESTS:BOOL=OFF
    -DBOOST_ROOT:FILEPATH=${BOOST_ROOT}
  DEPENDS
    ${_deps}
)

# Set the include dir
set(RDKit_INCLUDE_DIRS "${_source}/Code")

# If RDKit adds more libraries or we need more than just these,
# we can add them in later
set(RDKit_LIBRARIES
  RDGeneral
  RDBoost
  DataStructs
  RDGeometryLib
  Alignment
  EigenSolvers
  Optimizer
  ForceField
  DistGeometry
  Catalogs
  GraphMol
  Depictor
  SmilesParse
  FileParsers
  SubstructMatch
  ChemReactions
  ChemTransforms
  Subgraphs
  FilterCatalog
  FragCatalog
  Descriptors
  Fingerprints
  PartialCharges
  MolTransforms
  ForceFieldHelpers
  DistGeomHelpers
  MolAlign
  MolChemicalFeatures
  ShapeHelpers
  MolCatalog
  FMCS
  MolHash
  MMPA
  StructChecker
  ReducedGraphs
  Trajectory
  SLNParse
  SimDivPickers
  hc
  InfoTheory
  ChemicalFeatures
)

# Go ahead and add the include directories and the link directory
include_directories(${RDKit_INCLUDE_DIRS})
link_directories("${_source}/lib")
