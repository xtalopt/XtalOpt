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

Detailed instructions for downloading the latest stable version and installation can be found here:

https://xtalopt.github.io/download.html

# License

XtalOpt is published under the "New" BSD License. See the [LICENSE](LICENSE) file.

# Contributors

All contributors, in alphabetical order:

- Patrick Avery <psavery@buffalo.edu>
- Zackary Falls <zmfalls@buffalo.edu>
- Samad Hajinazar <samadh@buffalo.edu>
- Allison Vacanti <allison.vacanti@kitware.com>

# External Sources

The sources from the following projects and libraries are used in XtalOpt, and can be found under 'external/' folder:

- [xtalcomp](http://xtalopt.openmolecules.net/xtalcomp/xtalcomp.html)
- [randspg](http://xtalopt.openmolecules.net/randSpg/randSpg.html)
- [spglib](https://github.com/spglib/spglib) (2.5.0)
- [qhull](http://www.qhull.org) (8.0.2)
- [xraylib](https://github.com/tschoonj/xraylib) (4.2.1)
- [libmsym](https://github.com/mcodev31/libmsym) (0.2.2)
- [args](https://github.com/Taywee/args) (3.3.0)

These sources, other than modifications for interoperability
with XtalOpt, are the works of the copyright holders
specified in the corresponding folder.
