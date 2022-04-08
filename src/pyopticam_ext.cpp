#include <nanobind/nanobind.h>
#include <nanobind/tensor.h>
#include <cameralibrary.h>

using namespace CameraLibrary;

namespace nb = nanobind;

using namespace nb::literals;

NB_MODULE(pyopticam_ext, m) {
    m.def("add", [](int a, int b) { return a + b; }, "a"_a, "b"_a);

    m.def("inspect", [](nb::tensor<> tensor) {
        printf("Tensor data pointer : %p\n", tensor.data());
        printf("Tensor dimension : %zu\n", tensor.ndim());
        for (size_t i = 0; i < tensor.ndim(); ++i) {
            printf("Tensor dimension [%zu] : %zu\n", i, tensor.shape(i));
            printf("Tensor stride    [%zu] : %zd\n", i, tensor.stride(i));
        }
        printf("Device ID = %u (cpu=%i, cuda=%i)\n", tensor.device_id(),
            int(tensor.device_type() == nb::device::cpu::value),
            int(tensor.device_type() == nb::device::cuda::value)
        );
        printf("Tensor dtype: int16=%i, uint32=%i, float32=%i\n",
            tensor.dtype() == nb::dtype<int16_t>(),
            tensor.dtype() == nb::dtype<uint32_t>(),
            tensor.dtype() == nb::dtype<float>()
        );
    });

    m.def("process", [](nb::tensor<uint8_t, nb::shape<nb::any, nb::any, 3>, nb::c_contig, nb::device::cpu> tensor) {
        // Double brightness of the MxNx3 RGB image
        for (size_t y = 0; y < tensor.shape(0); ++y){
            for (size_t x = 0; y < tensor.shape(1); ++x){
                for (size_t ch = 0; ch < 3; ++ch){
                    tensor(y, x, ch) = (uint8_t) std::min(255, tensor(y, x, ch) * 2);
                }
            }
        }
    });

    m.def("ret_numpy", []() {
        float *data = new float[8] { 1, 2, 3, 4, 
                                     5, 6, 7, 8 };
        size_t shape[2] = { 2, 4 };

        /// Delete 'data' when the 'deleter' capsule expires
        nb::capsule deleter(data, [](void *p) {
            delete[] (float *) p;
        });

        return nb::tensor<nb::numpy, float>(data, 2, shape, /* owner = */ deleter);
    });
}
