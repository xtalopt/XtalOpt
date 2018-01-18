# Copyright Patrick Avery - 2017
# This module just sets RDKit_LIBRARIES to the RDKit libraries that we need
# for XtalOpt. If RDKIT_STATIC is set to ON, it will link to the static
# RDKit libraries

# If RDKit adds more libraries or we need more than just these,
# we can add them in later
set(_RDKit_LIBRARIES
  RDGeneral
#  RDBoost
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
#  SLNParse
  SimDivPickers
  hc
  InfoTheory
  ChemicalFeatures
)

if(RDKIT_STATIC)
  set(RDKIT_SUFFIX "_static")
endif(RDKIT_STATIC)

set(RDKIT_PREFIX "RDKit")

# We need to add the RDKit prefix (and possibly suffix) to each of these
foreach(_lib ${_RDKit_LIBRARIES})
  set(RDKit_LIBRARIES
      ${RDKit_LIBRARIES}
      ${RDKIT_PREFIX}${_lib}${RDKIT_SUFFIX}
  )
endforeach()

# If RDKIT_STATIC, we should add another copy of RDKit_LIBRARIES to the end of
# the list to ensure that all libraries will be found when linking
# In fact, tests show that we need 3 copies of it...
if(RDKIT_STATIC)
  set(RDKit_LIBRARIES ${RDKit_LIBRARIES};${RDKit_LIBRARIES};${RDKit_LIBRARIES})
endif(RDKIT_STATIC)
