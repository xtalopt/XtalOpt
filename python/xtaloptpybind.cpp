#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <string>
#include <iostream>

#include <xtalopt/ui/dialog.h>
#include <xtalopt/xtalopt.h>

namespace py = pybind11;

PYBIND11_PLUGIN(xtaloptpybind) {
  py::module m("xtaloptpybind", "XtalOpt Python bindings");

  py::class_<XtalOpt::XtalOpt>(m, "XtalOpt")
      .def(py::init<XtalOpt::XtalOptDialog*>())
      .def("printHello", &XtalOpt::XtalOpt::printHello);

  return m.ptr();
}
