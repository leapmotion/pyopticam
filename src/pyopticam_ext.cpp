#include <nanobind/nanobind.h>
#include <cameralibrary.h>

using namespace CameraLibrary;

namespace nb = nanobind;

using namespace nb::literals;

NB_MODULE(pyopticam_ext, m) {
    m.def("add", [](int a, int b) { return a + b; }, "a"_a, "b"_a);
}
