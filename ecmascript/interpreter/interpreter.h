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

#ifndef PANDA_RUNTIME_ECMASCRIPT_INTERPRETER_H
#define PANDA_RUNTIME_ECMASCRIPT_INTERPRETER_H

#include "ecmascript/js_method.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_thread.h"

namespace panda::ecmascript {
class ConstantPool;
class ECMAObject;
class GeneratorContext;

// align with 8
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
struct FrameState {
    const uint8_t *pc;
    JSTaggedType *sp;
    uint64_t *prev;
    JSMethod *method;
    // aligned with 8 bits
    alignas(sizeof(uint64_t)) ConstantPool *constpool;
    JSTaggedValue profileTypeInfo;
    JSTaggedValue acc;
    JSTaggedValue env;
    uint64_t numActualArgs;
};

// NOLINTNEXTLINE(bugprone-sizeof-expression)
static const uint32_t FRAME_STATE_SIZE = sizeof(FrameState) / sizeof(uint64_t);

static constexpr uint32_t RESERVED_CALL_ARGCOUNT = 3;
static constexpr uint32_t RESERVED_INDEX_CALL_TARGET = 0;
static constexpr uint32_t RESERVED_INDEX_NEW_TARGET = 1;
static constexpr uint32_t RESERVED_INDEX_THIS = 2;

struct CallParams {
    ECMAObject *callTarget;
    TaggedType newTarget;
    TaggedType thisArg;
    const TaggedType *argv;
    uint32_t argc;
};

class EcmaInterpreter {
public:
    static const uint32_t METHOD_HOTNESS_THRESHOLD = 100 * 1024;
    enum ActualNumArgsOfCall : uint8_t { CALLARG0 = 3, CALLARG1, CALLARGS2, CALLARGS3 };

    static inline JSTaggedValue Execute(JSThread *thread, const CallParams& params);
    static inline JSTaggedValue ExecuteNative(JSThread *thread, const CallParams& params);
    static inline JSTaggedValue GeneratorReEnterInterpreter(JSThread *thread, JSHandle<GeneratorContext> context);
    static inline void ChangeGenContext(JSThread *thread, JSHandle<GeneratorContext> context);
    static inline void ResumeContext(JSThread *thread);
    static inline void RunInternal(JSThread *thread, ConstantPool *constpool, const uint8_t *pc, JSTaggedType *sp);
    static inline uint8_t ReadU8(const uint8_t *pc, uint32_t offset);
    static inline void InitStackFrame(JSThread *thread);
    static inline uint32_t FindCatchBlock(JSMethod *caller, uint32_t pc);
    static inline size_t GetJumpSizeAfterCall(const uint8_t *prevPc);

    static inline JSTaggedValue GetRuntimeProfileTypeInfo(TaggedType *sp);
    static inline void UpdateHotnessCounter(JSThread* thread, TaggedType *sp, int32_t offset);
    static inline void InterpreterFrameCopyArgs(JSTaggedType *newSp, uint32_t numVregs, uint32_t numActualArgs,
                                                uint32_t numDeclaredArgs);
    static inline void NotifyBytecodePcChanged(JSThread *thread);
    static inline JSTaggedValue GetThisFunction(JSTaggedType *sp);
};

enum EcmaOpcode {
    MOV_DYN_V8_V8,
    MOV_DYN_V16_V16,
    LDA_STR_ID32,
    LDAI_DYN_IMM32,
    FLDAI_DYN_IMM64,
    LDNAN_IMM8,
    LDINFINITY_IMM8,
    LDGLOBALTHIS_IMM8,
    LDUNDEFINED_IMM8,
    LDNULL_IMM8,
    LDSYMBOL_IMM8,
    LDGLOBAL_IMM8,
    LDTRUE_IMM8,
    LDFALSE_IMM8,
    THROWDYN_IMM8,
    TYPEOFDYN_IMM8,
    LDLEXENVDYN_IMM8,
    POPLEXENVDYN_IMM8,
    GETUNMAPPEDARGS_IMM8,
    TOBOOLEAN_IMM8,
    GETPROPITERATOR_IMM8,
    ASYNCFUNCTIONENTER_IMM8,
    LDHOLE_IMM8,
    RETURNUNDEFINED_IMM8,
    CREATEEMPTYOBJECT_IMM8,
    CREATEEMPTYARRAY_IMM8,
    GETITERATOR_IMM8,
    THROWTHROWNOTEXISTS_IMM8,
    THROWPATTERNNONCOERCIBLE_IMM8,
    LDHOMEOBJECT_IMM8,
    THROWDELETESUPERPROPERTY_IMM8,
    DEBUGGER_IMM8,
    JMP_IMM8,
    JMP_IMM16,
    JMP_IMM32,
    JEQZ_IMM8,
    JEQZ_IMM16,
    LDA_DYN_V8,
    STA_DYN_V8,
    LDBOOLEAN_IMM8_V8,
    LDNUMBER_IMM8_V8,
    LDSTRING_IMM8_V8,
    LDBIGINT_IMM8_V8,
    ADD2DYN_IMM8_V8,
    SUB2DYN_IMM8_V8,
    MUL2DYN_IMM8_V8,
    DIV2DYN_IMM8_V8,
    MOD2DYN_IMM8_V8,
    EQDYN_IMM8_V8,
    NOTEQDYN_IMM8_V8,
    LESSDYN_IMM8_V8,
    LESSEQDYN_IMM8_V8,
    GREATERDYN_IMM8_V8,
    GREATEREQDYN_IMM8_V8,
    SHL2DYN_IMM8_V8,
    SHR2DYN_IMM8_V8,
    ASHR2DYN_IMM8_V8,
    AND2DYN_IMM8_V8,
    OR2DYN_IMM8_V8,
    XOR2DYN_IMM8_V8,
    TONUMBER_IMM8_V8,
    NEGDYN_IMM8_V8,
    NOTDYN_IMM8_V8,
    INCDYN_IMM8_V8,
    DECDYN_IMM8_V8,
    EXPDYN_IMM8_V8,
    ISINDYN_IMM8_V8,
    INSTANCEOFDYN_IMM8_V8,
    STRICTNOTEQDYN_IMM8_V8,
    STRICTEQDYN_IMM8_V8,
    RESUMEGENERATOR_IMM8_V8,
    GETRESUMEMODE_IMM8_V8,
    CREATEGENERATOROBJ_IMM8_V8,
    THROWUNDEFINED_IMM8_V8,
    THROWCONSTASSIGNMENT_IMM8_V8,
    GETTEMPLATEOBJECT_IMM8_V8,
    GETNEXTPROPNAME_IMM8_V8,
    CALLARG0DYN_IMM8_V8,
    THROWIFNOTOBJECT_IMM8_V8,
    ITERNEXT_IMM8_V8,
    CLOSEITERATOR_IMM8_V8,
    COPYMODULE_IMM8_V8,
    SUPERCALLSPREAD_IMM8_V8,
    LDOBJECT_IMM8_V8_V8,
    LDFUNCTION_IMM8_V8_V8,
    DELOBJPROP_IMM8_V8_V8,
    DEFINEGLOBALVAR_IMM8_V8_V8,
    DEFINELOCALVAR_IMM8_V8_V8,
    DEFINEFUNCEXPR_IMM8_V8_V8,
    REFEQDYN_IMM8_V8_V8,
    CALLRUNTIMERANGE_IMM8_V8_V8,
    NEWOBJSPREADDYN_IMM8_V8_V8,
    CREATEITERRESULTOBJ_IMM8_V8_V8,
    SUSPENDGENERATOR_IMM8_V8_V8,
    ASYNCFUNCTIONAWAITUNCAUGHT_IMM8_V8_V8,
    THROWUNDEFINEDIFHOLE_IMM8_V8_V8,
    CALLARG1DYN_IMM8_V8_V8,
    COPYDATAPROPERTIES_IMM8_V8_V8,
    STARRAYSPREAD_IMM8_V8_V8,
    GETITERATORNEXT_IMM8_V8_V8,
    SETOBJECTWITHPROTO_IMM8_V8_V8,
    CALLSPREADDYN_IMM8_V8_V8_V8,
    ASYNCFUNCTIONRESOLVE_IMM8_V8_V8_V8,
    ASYNCFUNCTIONREJECT_IMM8_V8_V8_V8,
    CALLARGS2DYN_IMM8_V8_V8_V8,
    CALLARGS3DYN_IMM8_V8_V8_V8_V8,
    DEFINEGETTERSETTERBYVALUE_IMM8_V8_V8_V8_V8,
    TRYLDGLOBALBYVALUE_IMM8_IMM16_V8,
    NEWOBJDYNRANGE_IMM8_IMM16_V8,
    TRYSTGLOBALBYVALUE_IMM8_IMM16_V8,
    CALLIRANGEDYN_IMM8_IMM16_V8,
    CALLITHISRANGEDYN_IMM8_IMM16_V8,
    SUPERCALL_IMM8_IMM16_V8,
    LDOBJBYVALUE_IMM8_IMM16_V8_V8,
    STOBJBYVALUE_IMM8_IMM16_V8_V8,
    LDOBJBYINDEX_IMM8_IMM16_V8_V8,
    STOBJBYINDEX_IMM8_IMM16_V8_V8,
    STOWNBYINDEX_IMM8_IMM16_V8_V8,
    STOWNBYVALUE_IMM8_IMM16_V8_V8,
    CREATEOBJECTWITHEXCLUDEDKEYS_IMM8_IMM16_V8_V8,
    STSUPERBYVALUE_IMM8_IMM16_V8_V8,
    LDSUPERBYVALUE_IMM8_IMM16_V8_V8,
    IMPORTMODULE_IMM8_ID32,
    STMODULEVAR_IMM8_ID32,
    DEFINEFUNCDYN_IMM8_ID16_V8,
    DEFINENCFUNCDYN_IMM8_ID16_V8,
    DEFINEGENERATORFUNC_IMM8_ID16_V8,
    DEFINEASYNCFUNC_IMM8_ID16_V8,
    DEFINEMETHOD_IMM8_ID16_V8,
    TRYLDGLOBALBYNAME_IMM8_ID32_IMM16,
    TRYSTGLOBALBYNAME_IMM8_ID32_IMM16,
    LDGLOBALVAR_IMM8_ID32_IMM16,
    STGLOBALVAR_IMM8_ID32_IMM16,
    LDOBJBYNAME_IMM8_ID32_IMM16_V8,
    STOBJBYNAME_IMM8_ID32_IMM16_V8,
    STOWNBYNAME_IMM8_ID32_IMM16_V8,
    LDMODVARBYNAME_IMM8_ID32_IMM16_V8,
    LDSUPERBYNAME_IMM8_ID32_IMM16_V8,
    STSUPERBYNAME_IMM8_ID32_IMM16_V8,
    STLEXVARDYN_IMM8_IMM16_IMM16_V8,
    LDLEXVARDYN_IMM8_IMM16_IMM16,
    NEWLEXENVDYN_IMM8_IMM16,
    COPYRESTARGS_IMM8_IMM16,
    CREATEOBJECTWITHBUFFER_IMM8_IMM16,
    CREATEARRAYWITHBUFFER_IMM8_IMM16,
    CREATEOBJECTHAVINGMETHOD_IMM8_IMM16,
    THROWIFSUPERNOTCORRECTCALL_IMM8_IMM16,
    DEFINECLASSWITHBUFFER_IMM8_ID16_IMM16_V8_V8,
    RETURN_DYN,
    MOV_V4_V4,
    JNEZ_IMM8,
    JNEZ_IMM16,
    LAST_OPCODE,
};
}  // namespace panda::ecmascript
#endif  // PANDA_RUNTIME_ECMASCRIPT_INTERPRETER_H
