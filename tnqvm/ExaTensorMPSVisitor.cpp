/***********************************************************************************
 * Copyright (c) 2017, UT-Battelle
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Contributors:
 *   Initial sketch - Mengsu Chen 2017/07/17;
 *   Implementation - Dmitry Lyakh 2017/10/05 - active;
 *
 **********************************************************************************/
#ifdef TNQVM_HAS_EXATENSOR

#include "ExaTensorMPSVisitor.hpp"

#define _DEBUG_DIL

namespace xacc {
namespace quantum {

//Life cycle:

ExaTensorMPSVisitor::ExaTensorMPSVisitor(std::shared_ptr<TNQVMBuffer> buffer, const std::size_t initialValence):
 Buffer(buffer), EagerEval(false)
{
 assert(initialValence > 0);
 const auto numQubits = buffer->size();
#ifdef _DEBUG_DIL
 std::cout << "[ExaTensorMPSVisitor]: Constructing an MPS wavefunction for " << numQubits << " qubits ... "; //debug
#endif
 //Construct initial MPS tensors for all qubits:
 const unsigned int rankMPS = 3; //MPS tensor rank
 const std::size_t dimExts[] = {initialValence,initialValence,BASE_SPACE_DIM}; //initial MPS tensor shape
 for(unsigned int i = 0; i < numQubits; ++i){
  StateMPS.emplace_back(Tensor(rankMPS,dimExts)); //construct a bodyless MPS tensor
  StateMPS[i].allocateBody(); //allocates MPS tensor body
  StateMPS[i].nullifyBody(); //sets all MPS tensor elements to zero
  this->initMPSTensor(i); //initializes the MPS tensor body to a pure state
 }
#ifdef _DEBUG_DIL
 std::cout << "Done" << std::endl; //debug
#endif
}

ExaTensorMPSVisitor::~ExaTensorMPSVisitor()
{
}

//Private member functions:

/** Initializes an MPS tensor to a disentangled pure |0> state. **/
void ExaTensorMPSVisitor::initMPSTensor(const unsigned int tensNum) //in: qubit id
{
 assert(tensNum < StateMPS.size());
 assert(StateMPS[tensNum].getRank() == 3);
 StateMPS[tensNum][{0,0,0}] = TensDataType(1.0,0.0);
 return;
}

/** Builds a tensor network for the wavefunction representation. **/
void ExaTensorMPSVisitor::buildWaveFunctionNetwork()
{
 assert(TensNet.isEmpty());
 //Construct the output tensor:
 unsigned int numQubits = StateMPS.size(); //number of MPS tensors = number of qubits
 const std::size_t outDims[numQubits] = {BASE_SPACE_DIM};
 std::vector<TensorLeg> legs;
 for(unsigned int i = 1; i <= numQubits; ++i) legs.emplace_back(TensorLeg(i,2)); //leg #2 is the open leg of each MPS tensor
 TensNet.appendTensor(Tensor(numQubits,outDims),legs);
 //Construct the input tensors:
 for(unsigned int i = 1; i <= numQubits; ++i){
  legs.clear();
  unsigned int prevTensId = i - 1; if(prevTensId == 0) prevTensId = numQubits; //previous MPS tensor id: [1..numQubits]
  unsigned int nextTensId = i + 1; if(nextTensId > numQubits) nextTensId = 1; //next MPS tensor id: [1..numQubits]
  legs.emplace_back(TensorLeg(prevTensId,1)); //connection to the previous MPS tensor
  legs.emplace_back(TensorLeg(nextTensId,0)); //connection to the next MPS tensor
  legs.emplace_back(TensorLeg(0,i-1)); //connection to the output tensor
  TensNet.appendTensor(StateMPS[i-1],legs); //append a wavefunction MPS tensor to the tensor network
 }
 return;
}

/** Closes the circuit tensor network with output tensors, those to be optimized.**/
void ExaTensorMPSVisitor::closeCircuitNetwork()
{
 assert(!(TensNet.isEmpty()));
 //`Implement
 return;
}

int ExaTensorMPSVisitor::apply1BodyGate(const Tensor & gate, const unsigned int q0)
{
 int error_code = 0;
 assert(gate.getRank() == 2);
 if(TensNet.isEmpty()) buildWaveFunctionNetwork();
 const auto numOutLegs = TensNet.getTensor(0).getRank();
 assert(q0 < numOutLegs);
 std::initializer_list<unsigned int> qubits {q0};
 std::vector<unsigned int> legIds(qubits);
 TensNet.appendTensor(gate,legIds);
 if(EagerEval) error_code = this->evaluate();
 return error_code;
}

int ExaTensorMPSVisitor::apply2BodyGate(const Tensor & gate, const unsigned int q0, const unsigned int q1)
{
 int error_code = 0;
 assert(gate.getRank() == 4);
 if(TensNet.isEmpty()) buildWaveFunctionNetwork();
 const auto numOutLegs = TensNet.getTensor(0).getRank();
 assert(q0 < numOutLegs && q1 < numOutLegs);
 std::initializer_list<unsigned int> qubits {q0,q1};
 std::vector<unsigned int> legIds(qubits);
 TensNet.appendTensor(gate,legIds);
 if(EagerEval) error_code = this->evaluate();
 return error_code;
}

int ExaTensorMPSVisitor::applyNBodyGate(const Tensor & gate, const unsigned int q[])
{
 if(TensNet.isEmpty()) buildWaveFunctionNetwork();
 //`Implement
 return 0;
}

//Public visitor methods:

void ExaTensorMPSVisitor::visit(Hadamard & gate)
{
 auto qbit0 = gate.bits()[0];
 std::cout << "Applying " << gate.getName() << " @ {" << qbit0 << "}" << std::endl;
 const Tensor & gateTensor = GateTensors.getTensor(gate);
 int error_code = this->apply1BodyGate(gateTensor,qbit0); assert(error_code == 0);
 return;
}

void ExaTensorMPSVisitor::visit(X & gate)
{
 auto qbit0 = gate.bits()[0];
 std::cout << "Applying " << gate.getName() << " @ {" << qbit0 << "}" << std::endl;
 const Tensor & gateTensor = GateTensors.getTensor(gate);
 int error_code = this->apply1BodyGate(gateTensor,qbit0); assert(error_code == 0);
 return;
}

void ExaTensorMPSVisitor::visit(Y & gate)
{
 auto qbit0 = gate.bits()[0];
 std::cout << "Applying " << gate.getName() << " @ {" << qbit0 << "}" << std::endl;
 const Tensor & gateTensor = GateTensors.getTensor(gate);
 int error_code = this->apply1BodyGate(gateTensor,qbit0); assert(error_code == 0);
 return;
}

void ExaTensorMPSVisitor::visit(Z & gate)
{
 auto qbit0 = gate.bits()[0];
 std::cout << "Applying " << gate.getName() << " @ {" << qbit0 << "}" << std::endl;
 const Tensor & gateTensor = GateTensors.getTensor(gate);
 int error_code = this->apply1BodyGate(gateTensor,qbit0); assert(error_code == 0);
 return;
}

void ExaTensorMPSVisitor::visit(Rx & gate)
{
 auto qbit0 = gate.bits()[0];
 std::cout << "Applying " << gate.getName() << " @ {" << qbit0 << "}" << std::endl;
 const Tensor & gateTensor = GateTensors.getTensor(gate);
 int error_code = this->apply1BodyGate(gateTensor,qbit0); assert(error_code == 0);
 return;
}

void ExaTensorMPSVisitor::visit(Ry & gate)
{
 auto qbit0 = gate.bits()[0];
 std::cout << "Applying " << gate.getName() << " @ {" << qbit0 << "}" << std::endl;
 const Tensor & gateTensor = GateTensors.getTensor(gate);
 int error_code = this->apply1BodyGate(gateTensor,qbit0); assert(error_code == 0);
 return;
}

void ExaTensorMPSVisitor::visit(Rz & gate)
{
 auto qbit0 = gate.bits()[0];
 std::cout << "Applying " << gate.getName() << " @ {" << qbit0 << "}" << std::endl;
 const Tensor & gateTensor = GateTensors.getTensor(gate);
 int error_code = this->apply1BodyGate(gateTensor,qbit0); assert(error_code == 0);
 return;
}

void ExaTensorMPSVisitor::visit(CPhase & gate)
{
 auto qbit0 = gate.bits()[0];
 auto qbit1 = gate.bits()[1];
 std::cout << "Applying " << gate.getName() << " @ {" << qbit0 << "," << qbit1 << "}" << std::endl;
 const Tensor & gateTensor = GateTensors.getTensor(gate);
 int error_code = this->apply2BodyGate(gateTensor,qbit0,qbit1); assert(error_code == 0);
 return;
}

void ExaTensorMPSVisitor::visit(CNOT & gate)
{
 auto qbit0 = gate.bits()[0];
 auto qbit1 = gate.bits()[1];
 std::cout << "Applying " << gate.getName() << " @ {" << qbit0 << "," << qbit1 << "}" << std::endl;
 const Tensor & gateTensor = GateTensors.getTensor(gate);
 int error_code = this->apply2BodyGate(gateTensor,qbit0,qbit1); assert(error_code == 0);
 return;
}

void ExaTensorMPSVisitor::visit(Swap & gate)
{
 auto qbit0 = gate.bits()[0];
 auto qbit1 = gate.bits()[1];
 std::cout << "Applying " << gate.getName() << " @ {" << qbit0 << "," << qbit1 << "}" << std::endl;
 const Tensor & gateTensor = GateTensors.getTensor(gate);
 int error_code = this->apply2BodyGate(gateTensor,qbit0,qbit1); assert(error_code == 0);
 return;
}

void ExaTensorMPSVisitor::visit(Measure & gate)
{
 auto qbit0 = gate.bits()[0];
 std::cout << "Applying " << gate.getName() << " @ {" << qbit0 << "}" << std::endl;
 //`Implement
 return;
}

void ExaTensorMPSVisitor::visit(ConditionalFunction & condFunc)
{
 //`Implement
 return;
}

void ExaTensorMPSVisitor::visit(GateFunction & gateFunc)
{
 //`Implement
 return;
}

//Numerical evaluation:

void ExaTensorMPSVisitor::setEvaluationStrategy(const bool eagerEval)
{
 assert(TensNet.isEmpty());
 EagerEval = eagerEval;
 return;
}

int ExaTensorMPSVisitor::evaluate()
{
 int error_code = 0;
 assert(!(TensNet.isEmpty()));
 closeCircuitNetwork(); //close the circuit tensor network with output tensors (those to be optimized)
 //`Send the tensor network to the solver, specifying which tensors to optimize
 //`Update the WaveFunction tensors with the optimized output tensors and destroy old wavefunction tensors
 //`Destroy the tensor network object
 return error_code;
}

}  // end namespace quantum
}  // end namespace xacc

#endif //TNQVM_HAS_EXATENSOR
