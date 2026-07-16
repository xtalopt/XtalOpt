XtalOpt test sample data
========================

This directory holds the fixture and reference data used by the test suite.
It is organized by purpose, not by test. Each subfolder has a single, distinct
job and a single update rule; please keep new data in the folder that matches
its purpose.


legacy/
-------
Frozen old-format files, kept so the conversion / back-compatibility layer can
be exercised against real historical data.

  legacy/xo-duplicateXtals-v4/
      A v4 xtalopt.state used to check settings conversion.

Rule: keep the v4 xtalopt.state unchanged.

Used by (tests/legacycompattest.cpp):
  v4SessionRestoreIsStable          - v4 settings and a generated v14 structure load
  v4ConvertedSettingsAreStable       - converted settings match outputs/
  schemeIsVersionedAndRoutesThroughLegacyLayer
  convertProducesCurrentFormatPerType     - the -c/--convert path per file type


inputs/
-------
Current-format input fixtures that the engine reads.

  inputs/xtalopt.in        a current xtalopt.in (gulp, direct-run)
  inputs/templates/        the template files it references

Rule: keep at the CURRENT input format; update when the format changes.

Used by:
  inputReadAndVerifyLoadsAssets (tests/xtaloptunittest.cpp)
  convertMechanicalReadIgnoresMissingAssets (tests/legacycompattest.cpp)


formats/
--------
Sample output files from external optimizers / quantum codes, consumed by the
format-parser tests. These mirror third-party tools, not XtalOpt's own format.

  formats/optimizerSamples/   per-code optimizer outputs (OUTCAR, xtal.got, ...)
  formats/*.POSCAR, *.cml     structure inputs for the parsers

Rule: changes only when a parser is added or a sample is refreshed.

Used by:
  tests/formatstest.cpp, tests/spglibtest.cpp, tests/genxrdtest.cpp,
  tests/genetictest.cpp, tests/structuretest.cpp, and the structuretool_*
  CTest cases (src/xtalopt/CMakeLists.txt).


outputs/
--------
Saved "expected" outputs, compared byte-for-byte by consistency tests.

  outputs/structure.state             current structure.state expected output
  outputs/results.txt, hull.txt       search output expected files
  outputs/v4_converted_settings.txt    dump of the v4->v5 converted settings

Rule: MACHINE-GENERATED. Never hand-edit. Regenerate deliberately by running
the relevant test once with XTALOPT_UPDATE_EXPECTED=1, then review the diff.

Used by:
  structureStateFileIsStable (tests/xtaloptunittest.cpp)
  resultsOutputsAreStable (tests/xtaloptunittest.cpp)
  v4ConvertedSettingsAreStable (tests/legacycompattest.cpp)
