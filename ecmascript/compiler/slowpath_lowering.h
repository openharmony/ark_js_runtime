/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ECMASCRIPT_COMPILER_GENERIC_LOWERING_H
#define ECMASCRIPT_COMPILER_GENERIC_LOWERING_H

#include "circuit.h"
#include "bytecode_circuit_builder.h"
#include "circuit_builder.h"
#include "circuit_builder-inl.h"
#include "gate_accessor.h"

namespace panda::ecmascript::kungfu {
// slowPath Lowering Process
// SW: state wire, DW: depend wire, VW: value wire
// Before lowering:
//                         SW        DW         VW
//                         |         |          |
//                         |         |          |
//                         v         v          v
//                     +-----------------------------+
//                     |            (HIR)            |
//                     |         JS_BYTECODE         |DW--------------------------------------
//                     |                             |                                       |
//                     +-----------------------------+                                       |
//                         SW                   SW                                           |
//                         |                     |                                           |
//                         |                     |                                           |
//                         |                     |                                           |
//                         v                     v                                           |
//                 +--------------+        +--------------+                                  |
//                 |  IF_SUCCESS  |        | IF_EXCEPTION |SW---------                       |
//                 +--------------+        +--------------+          |                       |
//                         SW                    SW                  |                       |
//                         |                     |                   |                       |
//                         v                     v                   |                       |
//     --------------------------------------------------------------|-----------------------|-------------------
//     catch processing                                              |                       |
//                                                                   |                       |
//                                                                   v                       V
//                                                            +--------------+       +-----------------+
//                                                            |    MERGE     |SW---->| DEPEND_SELECTOR |
//                                                            +--------------+       +-----------------+
//                                                                                          DW
//                                                                                          |
//                                                                                          v
//                                                                                   +-----------------+
//                                                                                   |  GET_EXCEPTION  |
//                                                                                   +-----------------+


// After lowering:
//         SW                                          DW      VW
//         |                                           |       |
//         |                                           |       |
//         |                                           v       v
//         |        +---------------------+         +------------------+
//         |        | CONSTANT(Exception) |         |       CALL       |DW---------------
//         |        +---------------------+         +------------------+                |
//         |                           VW            VW                                 |
//         |                           |             |                                  |
//         |                           |             |                                  |
//         |                           v             v                                  |
//         |                        +------------------+                                |
//         |                        |        EQ        |                                |
//         |                        +------------------+                                |
//         |                                VW                                          |
//         |                                |                                           |
//         |                                |                                           |
//         |                                v                                           |
//         |                        +------------------+                                |
//         ------------------------>|    IF_BRANCH     |                                |
//                                  +------------------+                                |
//                                   SW             SW                                  |
//                                   |              |                                   |
//                                   v              v                                   |
//                           +--------------+  +--------------+                         |
//                           |   IF_FALSE   |  |   IF_TRUE    |                         |
//                           |  (success)   |  |  (exception) |                         |
//                           +--------------+  +--------------+                         |
//                                 SW                SW   SW                            |
//                                 |                 |    |                             |
//                                 v                 v    |                             |
//     ---------------------------------------------------|-----------------------------|----------------------
//     catch processing                                   |                             |
//                                                        |                             |
//                                                        v                             v
//                                                 +--------------+             +-----------------+
//                                                 |    MERGE     |SW---------->| DEPEND_SELECTOR |
//                                                 +--------------+             +-----------------+
//                                                                                      DW
//                                                                                      |
//                                                                                      v
//                                                                              +-----------------+
//                                                                              |  GET_EXCEPTION  |
//                                                                              +-----------------+

class SlowPathLowering {
public:
    explicit SlowPathLowering(BytecodeCircuitBuilder *builder, Circuit *circuit)
        : builder_(builder), circuit_(circuit) {}
    ~SlowPathLowering() = default;
    void CallRuntimeLowering();

private:
    void LowerHirToCall(CircuitBuilder &cirBuilder, GateRef hirGate, GateRef callGate);
    void LowerHirToConditionCall(CircuitBuilder &cirBuilder, GateRef hirGate,
                                              GateRef condGate, GateRef callGate);
    void LowerHirToThrowCall(CircuitBuilder &cirBuilder, GateRef hirGate, GateRef callGate);
    void LowerExceptionHandler(GateRef hirGate);
    void Lower(GateRef gate, EcmaOpcode op);
    void LowerAdd2Dyn(GateRef gate, GateRef glue);
    void LowerCreateIterResultObj(GateRef gate, GateRef glue);
    void LowerSuspendGenerator(GateRef gate, GateRef glue);
    void LowerAsyncFunctionAwaitUncaught(GateRef gate, GateRef glue);
    void LowerAsyncFunctionResolve(GateRef gate, GateRef glue);
    void LowerAsyncFunctionReject(GateRef gate, GateRef glue);
    GateRef GetValueFromConstantStringTable(CircuitBuilder &builder, GateRef glue,
                                            GateRef gate, uint32_t inIndex);
    void LowerLoadStr(GateRef gate, GateRef glue);
    void LowerLexicalEnv(GateRef gate, GateRef glue);
    void LowerStGlobalVar(GateRef gate, GateRef glue);
    void LowerTryLdGlobalByName(GateRef gate, GateRef glue);
    void LowerGetIterator(GateRef gate, GateRef glue);
    void LowerCallArg0Dyn(GateRef gate, GateRef glue);
    void LowerCallArg1Dyn(GateRef gate, GateRef glue);
    void LowerCallArgs2Dyn(GateRef gate, GateRef glue);
    void LowerCallArgs3Dyn(GateRef gate, GateRef glue);
    void LowerCallIThisRangeDyn(GateRef gate, GateRef glue);
    void LowerCallSpreadDyn(GateRef gate, GateRef glue);
    void LowerCallIRangeDyn(GateRef gate, GateRef glue);
    void LowerNewObjSpreadDyn(GateRef gate, GateRef glue);
    void LowerThrowDyn(GateRef gate, GateRef glue);
    void LowerThrowConstAssignment(GateRef gate, GateRef glue);
    void LowerThrowThrowNotExists(GateRef gate, GateRef glue);
    void LowerThrowPatternNonCoercible(GateRef gate, GateRef glue);
    void LowerThrowIfSuperNotCorrectCall(GateRef gate, GateRef glue);
    void LowerThrowDeleteSuperProperty(GateRef gate, GateRef glue);
    void LowerLdSymbol(GateRef gate, GateRef glue);
    void LowerLdGlobal(GateRef gate, GateRef glue);
    void LowerSub2Dyn(GateRef gate, GateRef glue);
    void LowerMul2Dyn(GateRef gate, GateRef glue);
    void LowerDiv2Dyn(GateRef gate, GateRef glue);
    void LowerMod2Dyn(GateRef gate, GateRef glue);
    void LowerEqDyn(GateRef gate, GateRef glue);
    void LowerNotEqDyn(GateRef gate, GateRef glue);
    void LowerLessDyn(GateRef gate, GateRef glue);
    void LowerLessEqDyn(GateRef gate, GateRef glue);
    void LowerGreaterDyn(GateRef gate, GateRef glue);
    void LowerGreaterEqDyn(GateRef gate, GateRef glue);
    void LowerGetPropIterator(GateRef gate, GateRef glue);
    void LowerIterNext(GateRef gate, GateRef glue);
    void LowerCloseIterator(GateRef gate, GateRef glue);
    void LowerIncDyn(GateRef gate, GateRef glue);
    void LowerDecDyn(GateRef gate, GateRef glue);

    BytecodeCircuitBuilder *builder_;
    Circuit *circuit_;
};
}  // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_GENERIC_LOWERING_H