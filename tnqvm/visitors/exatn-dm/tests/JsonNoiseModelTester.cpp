#include <gtest/gtest.h>
#include "xacc.hpp"
#include "xacc_service.hpp"
#include "NoiseModel.hpp"

namespace {
// A sample Json for testing
// Single-qubit depolarizing:
const std::string depol_json =
    R"({"gate_noise": [{"gate_name": "X", "register_location": ["0"], "noise_channels": [{"matrix": [[[[0.99498743710662, 0.0], [0.0, 0.0]], [[0.0, 0.0], [0.99498743710662, 0.0]]], [[[0.0, 0.0], [0.05773502691896258, 0.0]], [[0.05773502691896258, 0.0], [0.0, 0.0]]], [[[0.0, 0.0], [0.0, -0.05773502691896258]], [[0.0, 0.05773502691896258], [0.0, 0.0]]], [[[0.05773502691896258, 0.0], [0.0, 0.0]], [[0.0, 0.0], [-0.05773502691896258, 0.0]]]]}]}], "bit_order": "MSB"})";
// Single-qubit amplitude damping (25% rate):
const std::string ad_json =
    R"({"gate_noise": [{"gate_name": "X", "register_location": ["0"], "noise_channels": [{"matrix": [[[[1.0, 0.0], [0.0, 0.0]], [[0.0, 0.0], [0.8660254037844386, 0.0]]], [[[0.0, 0.0], [0.5, 0.0]], [[0.0, 0.0], [0.0, 0.0]]]]}]}], "bit_order": "MSB"})";
// Two-qubit noise channel (on a CNOT gate) in MSB and LSB order convention
// (matrix representation)
const std::string msb_noise_model =
    R"({"gate_noise": [{"gate_name": "CNOT", "register_location": ["0", "1"], "noise_channels": [{"matrix": [[[[0.99498743710662, 0.0], [0.0, 0.0], [0.0, 0.0], [0.0, 0.0]], [[0.0, 0.0], [0.99498743710662, 0.0], [0.0, 0.0], [0.0, 0.0]], [[0.0, 0.0], [0.0, 0.0], [0.99498743710662, 0.0], [0.0, 0.0]], [[0.0, 0.0], [0.0, 0.0], [0.0, 0.0], [0.99498743710662, 0.0]]], [[[0.0, 0.0], [0.05773502691896258, 0.0], [0.0, 0.0], [0.0, 0.0]], [[0.05773502691896258, 0.0], [0.0, 0.0], [0.0, 0.0], [0.0, 0.0]], [[0.0, 0.0], [0.0, 0.0], [0.0, 0.0], [0.05773502691896258, 0.0]], [[0.0, 0.0], [0.0, 0.0], [0.05773502691896258, 0.0], [0.0, 0.0]]], [[[0.0, 0.0], [0.0, -0.05773502691896258], [0.0, 0.0], [0.0, 0.0]], [[0.0, 0.05773502691896258], [0.0, 0.0], [0.0, 0.0], [0.0, 0.0]], [[0.0, 0.0], [0.0, 0.0], [0.0, 0.0], [0.0, -0.05773502691896258]], [[0.0, 0.0], [0.0, 0.0], [0.0, 0.05773502691896258], [0.0, 0.0]]], [[[0.05773502691896258, 0.0], [0.0, 0.0], [0.0, 0.0], [0.0, 0.0]], [[0.0, 0.0], [-0.05773502691896258, 0.0], [0.0, 0.0], [-0.0, 0.0]], [[0.0, 0.0], [0.0, 0.0], [0.05773502691896258, 0.0], [0.0, 0.0]], [[0.0, 0.0], [-0.0, 0.0], [0.0, 0.0], [-0.05773502691896258, 0.0]]]]}]}], "bit_order": "MSB"})";
const std::string lsb_noise_model =
    R"({"gate_noise": [{"gate_name": "CNOT", "register_location": ["0", "1"], "noise_channels": [{"matrix": [[[[0.99498743710662, 0.0], [0.0, 0.0], [0.0, 0.0], [0.0, 0.0]], [[0.0, 0.0], [0.99498743710662, 0.0], [0.0, 0.0], [0.0, 0.0]], [[0.0, 0.0], [0.0, 0.0], [0.99498743710662, 0.0], [0.0, 0.0]], [[0.0, 0.0], [0.0, 0.0], [0.0, 0.0], [0.99498743710662, 0.0]]], [[[0.0, 0.0], [0.0, 0.0], [0.05773502691896258, 0.0], [0.0, 0.0]], [[0.0, 0.0], [0.0, 0.0], [0.0, 0.0], [0.05773502691896258, 0.0]], [[0.05773502691896258, 0.0], [0.0, 0.0], [0.0, 0.0], [0.0, 0.0]], [[0.0, 0.0], [0.05773502691896258, 0.0], [0.0, 0.0], [0.0, 0.0]]], [[[0.0, 0.0], [0.0, 0.0], [0.0, -0.05773502691896258], [0.0, 0.0]], [[0.0, 0.0], [0.0, 0.0], [0.0, 0.0], [0.0, -0.05773502691896258]], [[0.0, 0.05773502691896258], [0.0, 0.0], [0.0, 0.0], [0.0, 0.0]], [[0.0, 0.0], [0.0, 0.05773502691896258], [0.0, 0.0], [0.0, 0.0]]], [[[0.05773502691896258, 0.0], [0.0, 0.0], [0.0, 0.0], [0.0, 0.0]], [[0.0, 0.0], [0.05773502691896258, 0.0], [0.0, 0.0], [0.0, 0.0]], [[0.0, 0.0], [0.0, 0.0], [-0.05773502691896258, 0.0], [-0.0, 0.0]], [[0.0, 0.0], [0.0, 0.0], [-0.0, 0.0], [-0.05773502691896258, 0.0]]]]}]}], "bit_order": "LSB"})";
// Noise model that only has readout errors for validation:
// P(1|0) = 0.1; P(0|1) = 0.2
const std::string ro_error_noise_model =
    R"({"gate_noise": [], "bit_order": "MSB", "readout_errors": [{"register_location": "0", "prob_meas0_prep1": 0.2, "prob_meas1_prep0": 0.1}]})";
} // namespace

TEST(JsonNoiseModelTester, checkNoNoise) {
  {
    auto accelerator =
        xacc::getAccelerator("tnqvm", {{"tnqvm-visitor", "exatn-dm"}});
    auto xasmCompiler = xacc::getCompiler("xasm");
    auto program = xasmCompiler
                       ->compile(R"(__qpu__ void testX(qbit q) {
        X(q[0]);
        Measure(q[0]);
      })",
                                 accelerator)
                       ->getComposite("testX");
    auto buffer = xacc::qalloc(1);
    accelerator->execute(buffer, program);
    auto exeInfo = accelerator->getExecutionInfo();
    auto dm = accelerator
                  ->getExecutionInfo<xacc::ExecutionInfo::DensityMatrixPtrType>(
                      xacc::ExecutionInfo::DmKey);
    std::cout << "Density matrix\n";
    for (const auto &row : *dm) {
      for (const auto &elem : row) {
        std::cout << elem << " ";
      }
      std::cout << "\n";
    }
    EXPECT_EQ(dm->size(), 2);
  }

  {
    auto accelerator =
        xacc::getAccelerator("tnqvm", {{"tnqvm-visitor", "exatn-dm"}});
    auto xasmCompiler = xacc::getCompiler("xasm");
    auto program = xasmCompiler
                       ->compile(R"(__qpu__ void testBell(qbit q) {
        H(q[0]);
        CX(q[0], q[1]);
        Measure(q[0]);
      })",
                                 accelerator)
                       ->getComposite("testBell");
    auto buffer = xacc::qalloc(2);
    accelerator->execute(buffer, program);
    buffer->print();
    auto exeInfo = accelerator->getExecutionInfo();
    auto dm = accelerator
                  ->getExecutionInfo<xacc::ExecutionInfo::DensityMatrixPtrType>(
                      xacc::ExecutionInfo::DmKey);
    std::cout << "Density matrix\n";
    for (const auto &row : *dm) {
      for (const auto &elem : row) {
        std::cout << elem << " ";
      }
      std::cout << "\n";
    }
    EXPECT_EQ(dm->size(), 4);
  }
}

TEST(JsonNoiseModelTester, checkSimple) {
  // Check depolarizing channels
  {
    auto noiseModel = xacc::getService<xacc::NoiseModel>("json");
    noiseModel->initialize({{"noise-model", depol_json}});
    auto accelerator = xacc::getAccelerator(
        "tnqvm", {{"tnqvm-visitor", "exatn-dm"}, {"noise-model", noiseModel}});
    auto xasmCompiler = xacc::getCompiler("xasm");
    auto program = xasmCompiler
                       ->compile(R"(__qpu__ void testX(qbit q) {
        X(q[0]);
        Measure(q[0]);
      })",
                                 accelerator)
                       ->getComposite("testX");
    auto buffer = xacc::qalloc(1);
    accelerator->execute(buffer, program);
    buffer->print();
    auto dmPtr =
        accelerator
            ->getExecutionInfo<xacc::ExecutionInfo::DensityMatrixPtrType>(
                xacc::ExecutionInfo::DmKey);
    auto densityMatrix = *dmPtr;
    EXPECT_EQ(densityMatrix.size(), 2);
    // Check trace
    EXPECT_NEAR(densityMatrix[0][0].real() + densityMatrix[1][1].real(), 1.0, 1e-6);
    // Expected result:
    // 0.00666667+0.j 0.        +0.j
    // 0.        +0.j 0.99333333+0.j
    // Check real part
    EXPECT_NEAR(densityMatrix[0][0].real(), 0.00666667, 1e-6);
    EXPECT_NEAR(densityMatrix[1][0].real(), 0.0, 1e-6);
    EXPECT_NEAR(densityMatrix[0][1].real(), 0.0, 1e-6);
    EXPECT_NEAR(densityMatrix[1][1].real(), 0.99333333, 1e-6);
  }

  // Check amplitude damping channels
  {
    auto noiseModel = xacc::getService<xacc::NoiseModel>("json");
    noiseModel->initialize({{"noise-model", ad_json}});
    auto accelerator = xacc::getAccelerator(
        "tnqvm", {{"tnqvm-visitor", "exatn-dm"}, {"noise-model", noiseModel}});
    auto xasmCompiler = xacc::getCompiler("xasm");
    auto program = xasmCompiler
                       ->compile(R"(__qpu__ void testX_ad(qbit q) {
        X(q[0]);
        Measure(q[0]);
      })",
                                 accelerator)
                       ->getComposites()[0];
    auto buffer = xacc::qalloc(1);
    accelerator->execute(buffer, program);
    auto dmPtr = accelerator
                  ->getExecutionInfo<xacc::ExecutionInfo::DensityMatrixPtrType>(
                      xacc::ExecutionInfo::DmKey);
    auto densityMatrix = *dmPtr;
    // Verify the distribution (25% amplitude damping)
    EXPECT_NEAR(densityMatrix[0][0].real(), 0.25, 1e-6);
    EXPECT_NEAR(densityMatrix[1][1].real(), 0.75, 1e-6);
  }
}

TEST(JsonNoiseModelTester, checkBitOrdering) {
  auto xasmCompiler = xacc::getCompiler("xasm");
  auto program = xasmCompiler
                     ->compile(R"(__qpu__ void testCX(qbit q) {
        CX(q[0], q[1]);
        Measure(q[0]);
        Measure(q[1]);
      })",
                               nullptr)
                     ->getComposites()[0];

  std::vector<std::vector<std::complex<double>>> densityMatrix_msb, densityMatrix_lsb;
  // Check MSB:
  {
    auto noiseModel = xacc::getService<xacc::NoiseModel>("json");
    noiseModel->initialize({{"noise-model", msb_noise_model}});
    auto accelerator = xacc::getAccelerator(
        "tnqvm", {{"tnqvm-visitor", "exatn-dm"}, {"noise-model", noiseModel}});

    auto buffer = xacc::qalloc(2);
    accelerator->execute(buffer, program);
    auto dmPtr =
        accelerator
            ->getExecutionInfo<xacc::ExecutionInfo::DensityMatrixPtrType>(
                xacc::ExecutionInfo::DmKey);
    densityMatrix_msb = *dmPtr;
  }

  // Check LSB:
  {
    auto noiseModel = xacc::getService<xacc::NoiseModel>("json");
    noiseModel->initialize({{"noise-model", lsb_noise_model}});
    auto accelerator = xacc::getAccelerator(
        "tnqvm", {{"tnqvm-visitor", "exatn-dm"}, {"noise-model", noiseModel}});
    auto buffer = xacc::qalloc(2);
    accelerator->execute(buffer, program);
    auto dmPtr =
        accelerator
            ->getExecutionInfo<xacc::ExecutionInfo::DensityMatrixPtrType>(
                xacc::ExecutionInfo::DmKey);
    densityMatrix_lsb = *dmPtr;
  }
}

int main(int argc, char **argv) {
  xacc::Initialize(argc, argv);
  ::testing::InitGoogleTest(&argc, argv);
  auto ret = RUN_ALL_TESTS();
  xacc::Finalize();
  return ret;
}