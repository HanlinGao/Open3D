// ----------------------------------------------------------------------------
// -                        Open3D: www.open3d.org                            -
// ----------------------------------------------------------------------------
// The MIT License (MIT)
//
// Copyright (c) 2018 www.open3d.org
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
// ----------------------------------------------------------------------------

#include "open3d/core/linalg/Solve.h"
#include <unordered_map>

namespace open3d {
namespace core {

void Solve(const Tensor &A, const Tensor &B, Tensor &X) {
    // Check devices
    Device device = A.GetDevice();
    if (device != B.GetDevice()) {
        utility::LogError("Tensor A device {} and Tensor B device {} mismatch",
                          A.GetDevice().ToString(), B.GetDevice().ToString());
    }

    // Check dtypes
    Dtype dtype = A.GetDtype();
    if (dtype != B.GetDtype()) {
        utility::LogError("Tensor A dtype {} and Tensor B dtype {} mismatch",
                          DtypeUtil::ToString(A.GetDtype()),
                          DtypeUtil::ToString(B.GetDtype()));
    }

    if (dtype != Dtype::Float32 && dtype != Dtype::Float64) {
        utility::LogError(
                "Only tensors with Float32 or Float64 are supported, but "
                "received {}",
                DtypeUtil::ToString(dtype));
    }

    // Check dimensions
    SizeVector A_shape = A.GetShape();
    SizeVector B_shape = B.GetShape();
    if (A_shape.size() != 2) {
        utility::LogError("Tensor A must be 2D, but got {}D", A_shape.size());
    }
    if (B_shape.size() != 1 && B_shape.size() != 2) {
        utility::LogError(
                "Tensor B must be 1D (vector) or 2D (matrix), but got {}D",
                B_shape.size());
    }
    int m = A_shape[0], n = A_shape[1],
        k = B_shape.size() == 2 ? B_shape[1] : 1;
    if (m < n) {
        utility::LogError("Tensor A shape must satisfy rows ({}) > cols({})", m,
                          n);
    }

    if (device.GetType() == Device::DeviceType::CUDA) {
#ifdef BUILD_CUDA_MODULE
        // cuSolver uses column-wise storage
        Tensor A_copy = A.T().Copy(device);
        void *A_data = A_copy.GetDataPtr();

        Tensor B_copy = B.T().Copy(device);
        void *B_data = B_copy.GetDataPtr();

        SolveCUDA(A_data, B_data, m, n, k, dtype, device);
        X = B_copy.T().Slice(0, 0, n);
#else
        utility::LogError("Unimplemented device.");
#endif
    } else {
        // LAPACKE changes solves X by modifying B in-place
        Tensor A_copy = A.Copy(device);
        void *A_data = A_copy.GetDataPtr();

        Tensor B_copy = B.Copy(device);
        void *B_data = B_copy.GetDataPtr();

        SolveCPU(A_data, B_data, m, n, k, dtype, device);
        X = B_copy.Slice(0, 0, n);
    }
}
}  // namespace core
}  // namespace open3d