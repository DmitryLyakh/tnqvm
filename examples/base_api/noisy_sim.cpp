/***********************************************************************************
 * Copyright (c) 2020, UT-Battelle
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of the xacc nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Contributors:
 *   Initial implementation - Thien Nguyen
 *
 **********************************************************************************/

// This example demonstrates the basic usage of TNQVM's exatn visitor for QASM
// circuit simulation.
#include "xacc.hpp"

int main(int argc, char **argv) {

  // Initialize the XACC Framework
  xacc::Initialize(argc, argv);

  // Using the purified-mps backend with noise model from
  // "ibmq_5_yorktown - ibmqx2" device.
  // **IMPORTANT** make sure you have a valid IBMQ credential stored in
  // ~/.ibm_config file.
  auto qpu = xacc::getAccelerator(
      "tnqvm", {{"tnqvm-visitor", "exatn-pmps"}, {"backend", "ibmqx2"}});

  // Allocate a register of 2 qubits
  auto qubitReg = xacc::qalloc(2);

  // Create a Program: simple Bell test
  auto xasmCompiler = xacc::getCompiler("xasm");
  auto ir = xasmCompiler->compile(R"(__qpu__ void test(qbit q, double theta) {
    H(q[0]);
    CX(q[0],q[1]);
    Measure(q[0]);
    Measure(q[1]);
    })",
                                  qpu);

  // Request the quantum kernel representing
  // the above source code
  auto program = ir->getComposite("test");

  // Execute!
  qpu->execute(qubitReg, program);
  // Print the result (measurement count distribution) in the buffer.
  qubitReg->print();

  // Finalize the XACC Framework
  xacc::Finalize();

  return 0;
}
