[![License](https://img.shields.io/badge/License-BSD%203--Clause-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)

XtalOpt
=========

XtalOpt is an evolutionary multi-objective global optimization
algorithm, designed for computational prediction of functional materials
with fixed or variable composition.

With an on-the-fly convex hull evaluation, the code can explore the
composition space of a desired chemical system, 
and supports both generalized scalar fitness function and Pareto
optimization schemes for global optimization.

XtalOpt is developed and maintained in the
[Eva Zurek's group](https://www.acsu.buffalo.edu/~ezurek/)
in the University at Buffalo.

More information can be found at https://xtalopt.github.io

# User Manual and Installation

A brief introduction to the code's features and its user manual are available at:

https://xtalopt.github.io/xtalopt.html

For downloading the latest stable version and installation instructions see:

https://xtalopt.github.io/download.html

# License

XtalOpt is published under the "New" BSD License. See LICENSE file.

# Contributors

All contributors, in alphabetical order:

- Patrick Avery <psavery@buffalo.edu>
- Zackary Falls <zmfalls@buffalo.edu>
- Samad Hajinazar <samadh@buffalo.edu>
- Allison Vacanti <allison.vacanti@kitware.com>

# External Sources

Included sources from various projects are found under:

- external/pugixml  https://pugixml.org/
- external/randSpg  http://xtalopt.openmolecules.net/randSpg/randSpg.html
- external/spglib   https://github.com/spglib/spglib
- external/xtalcomp http://xtalopt.openmolecules.net/xtalcomp/xtalcomp.html
- external/qhull    http://www.qhull.org

These files, other than some minor modifications for interoperability
with XtalOpt, are the works of the copyright holders
specified in the source files.
