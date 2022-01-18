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

#include "ecmascript/class_linker/bytecode_circuit_builder.h"

namespace panda::ecmascript {
void ByteCodeCircuitBuilder::BytecodeToCircuit(std::vector<uint8_t *> pcArr, const panda_file::File &pf,
                                               const JSMethod *method)
{
    auto curPc = pcArr.front();
    auto prePc = curPc;
    std::map<uint8_t *, uint8_t *> byteCodeCurPrePc;
    std::vector<std::tuple<uint8_t *, SplitPoint, std::vector<uint8_t *>>> bytecodeBlockInfo;
    auto startPc = curPc;
    bytecodeBlockInfo.emplace_back(std::make_tuple(startPc, SplitPoint::START, std::vector<uint8_t *>(1, startPc)));
    byteCodeCurPrePc.insert(std::pair<uint8_t *, uint8_t *>(curPc, prePc));
    for (size_t i = 1; i < pcArr.size() - 1; i++) {
        curPc = pcArr[i];
        byteCodeCurPrePc.insert(std::pair<uint8_t *, uint8_t *>(curPc, prePc));
        prePc = curPc;
        CollectBytecodeBlockInfo(curPc, bytecodeBlockInfo);
    }
    // handle empty
    byteCodeCurPrePc.insert(std::pair<uint8_t *, uint8_t *>(pcArr[pcArr.size() - 1], prePc));

    // collect try catch block info
    auto expectionInfo = CollectTryCatchBlockInfo(pf, method, byteCodeCurPrePc, bytecodeBlockInfo);

    // Complete bytecode blcok Infomation
    CompleteBytecodeBlockInfo(byteCodeCurPrePc, bytecodeBlockInfo);

    // Building the basic block diagram of bytecode
    BuildBasicBlocks(method, expectionInfo, bytecodeBlockInfo, byteCodeCurPrePc);
}

std::string ByteCodeCircuitBuilder::ByteCodeStr(uint8_t *pc) const
{
    const std::map<EcmaOpcode, const char *> strMap = {
            {LDNAN_PREF, "LDNAN"},
            {LDINFINITY_PREF, "LDINFINITY"},
            {LDGLOBALTHIS_PREF, "LDGLOBALTHIS"},
            {LDUNDEFINED_PREF, "LDUNDEFINED"},
            {LDNULL_PREF, "LDNULL"},
            {LDSYMBOL_PREF, "LDSYMBOL"},
            {LDGLOBAL_PREF, "LDGLOBAL"},
            {LDTRUE_PREF, "LDTRUE"},
            {LDFALSE_PREF, "LDFALSE"},
            {THROWDYN_PREF, "THROWDYN"},
            {TYPEOFDYN_PREF, "TYPEOFDYN"},
            {LDLEXENVDYN_PREF, "LDLEXENVDYN"},
            {POPLEXENVDYN_PREF, "POPLEXENVDYN"},
            {GETUNMAPPEDARGS_PREF, "GETUNMAPPEDARGS"},
            {GETPROPITERATOR_PREF, "GETPROPITERATOR"},
            {ASYNCFUNCTIONENTER_PREF, "ASYNCFUNCTIONENTER"},
            {LDHOLE_PREF, "LDHOLE"},
            {RETURNUNDEFINED_PREF, "RETURNUNDEFINED"},
            {CREATEEMPTYOBJECT_PREF, "CREATEEMPTYOBJECT"},
            {CREATEEMPTYARRAY_PREF, "CREATEEMPTYARRAY"},
            {GETITERATOR_PREF, "GETITERATOR"},
            {THROWTHROWNOTEXISTS_PREF, "THROWTHROWNOTEXISTS"},
            {THROWPATTERNNONCOERCIBLE_PREF, "THROWPATTERNNONCOERCIBLE"},
            {LDHOMEOBJECT_PREF, "LDHOMEOBJECT"},
            {THROWDELETESUPERPROPERTY_PREF, "THROWDELETESUPERPROPERTY"},
            {DEBUGGER_PREF, "DEBUGGER"},
            {ADD2DYN_PREF_V8, "ADD2DYN"},
            {SUB2DYN_PREF_V8, "SUB2DYN"},
            {MUL2DYN_PREF_V8, "MUL2DYN"},
            {DIV2DYN_PREF_V8, "DIV2DYN"},
            {MOD2DYN_PREF_V8, "MOD2DYN"},
            {EQDYN_PREF_V8, "EQDYN"},
            {NOTEQDYN_PREF_V8, "NOTEQDYN"},
            {LESSDYN_PREF_V8, "LESSDYN"},
            {LESSEQDYN_PREF_V8, "LESSEQDYN"},
            {GREATERDYN_PREF_V8, "GREATERDYN"},
            {GREATEREQDYN_PREF_V8, "GREATEREQDYN"},
            {SHL2DYN_PREF_V8, "SHL2DYN"},
            {SHR2DYN_PREF_V8, "SHR2DYN"},
            {ASHR2DYN_PREF_V8, "ASHR2DYN"},
            {AND2DYN_PREF_V8, "AND2DYN"},
            {OR2DYN_PREF_V8, "OR2DYN"},
            {XOR2DYN_PREF_V8, "XOR2DYN"},
            {TONUMBER_PREF_V8, "TONUMBER"},
            {NEGDYN_PREF_V8, "NEGDYN"},
            {NOTDYN_PREF_V8, "NOTDYN"},
            {INCDYN_PREF_V8, "INCDYN"},
            {DECDYN_PREF_V8, "DECDYN"},
            {EXPDYN_PREF_V8, "EXPDYN"},
            {ISINDYN_PREF_V8, "ISINDYN"},
            {INSTANCEOFDYN_PREF_V8, "INSTANCEOFDYN"},
            {STRICTNOTEQDYN_PREF_V8, "STRICTNOTEQDYN"},
            {STRICTEQDYN_PREF_V8, "STRICTEQDYN"},
            {RESUMEGENERATOR_PREF_V8, "RESUMEGENERATOR"},
            {GETRESUMEMODE_PREF_V8, "GETRESUMEMODE"},
            {CREATEGENERATOROBJ_PREF_V8, "CREATEGENERATOROBJ"},
            {THROWCONSTASSIGNMENT_PREF_V8, "THROWCONSTASSIGNMENT"},
            {GETTEMPLATEOBJECT_PREF_V8, "GETTEMPLATEOBJECT"},
            {GETNEXTPROPNAME_PREF_V8, "GETNEXTPROPNAME"},
            {CALLARG0DYN_PREF_V8, "CALLARG0DYN"},
            {THROWIFNOTOBJECT_PREF_V8, "THROWIFNOTOBJECT"},
            {ITERNEXT_PREF_V8, "ITERNEXT"},
            {CLOSEITERATOR_PREF_V8, "CLOSEITERATOR"},
            {COPYMODULE_PREF_V8, "COPYMODULE"},
            {SUPERCALLSPREAD_PREF_V8, "SUPERCALLSPREAD"},
            {DELOBJPROP_PREF_V8_V8, "DELOBJPROP"},
            {NEWOBJSPREADDYN_PREF_V8_V8, "NEWOBJSPREADDYN"},
            {CREATEITERRESULTOBJ_PREF_V8_V8, "CREATEITERRESULTOBJ"},
            {SUSPENDGENERATOR_PREF_V8_V8, "SUSPENDGENERATOR"},
            {ASYNCFUNCTIONAWAITUNCAUGHT_PREF_V8_V8, "ASYNCFUNCTIONAWAITUNCAUGHT"},
            {THROWUNDEFINEDIFHOLE_PREF_V8_V8, "THROWUNDEFINEDIFHOLE"},
            {CALLARG1DYN_PREF_V8_V8, "CALLARG1DYN"},
            {COPYDATAPROPERTIES_PREF_V8_V8, "COPYDATAPROPERTIES"},
            {STARRAYSPREAD_PREF_V8_V8, "STARRAYSPREAD"},
            {GETITERATORNEXT_PREF_V8_V8, "GETITERATORNEXT"},
            {SETOBJECTWITHPROTO_PREF_V8_V8, "SETOBJECTWITHPROTO"},
            {LDOBJBYVALUE_PREF_V8_V8, "LDOBJBYVALUE"},
            {STOBJBYVALUE_PREF_V8_V8, "STOBJBYVALUE"},
            {STOWNBYVALUE_PREF_V8_V8, "STOWNBYVALUE"},
            {LDSUPERBYVALUE_PREF_V8_V8, "LDSUPERBYVALUE"},
            {STSUPERBYVALUE_PREF_V8_V8, "STSUPERBYVALUE"},
            {LDOBJBYINDEX_PREF_V8_IMM32, "LDOBJBYINDEX"},
            {STOBJBYINDEX_PREF_V8_IMM32, "STOBJBYINDEX"},
            {STOWNBYINDEX_PREF_V8_IMM32, "STOWNBYINDEX"},
            {CALLSPREADDYN_PREF_V8_V8_V8, "CALLSPREADDYN"},
            {ASYNCFUNCTIONRESOLVE_PREF_V8_V8_V8, "ASYNCFUNCTIONRESOLVE"},
            {ASYNCFUNCTIONREJECT_PREF_V8_V8_V8, "ASYNCFUNCTIONREJECT"},
            {CALLARGS2DYN_PREF_V8_V8_V8, "CALLARGS2DYN"},
            {CALLARGS3DYN_PREF_V8_V8_V8_V8, "CALLARGS3DYN"},
            {DEFINEGETTERSETTERBYVALUE_PREF_V8_V8_V8_V8, "DEFINEGETTERSETTERBYVALUE"},
            {NEWOBJDYNRANGE_PREF_IMM16_V8, "NEWOBJDYNRANGE"},
            {CALLIRANGEDYN_PREF_IMM16_V8, "CALLIRANGEDYN"},
            {CALLITHISRANGEDYN_PREF_IMM16_V8, "CALLITHISRANGEDYN"},
            {SUPERCALL_PREF_IMM16_V8, "SUPERCALL"},
            {CREATEOBJECTWITHEXCLUDEDKEYS_PREF_IMM16_V8_V8, "CREATEOBJECTWITHEXCLUDEDKEYS"},
            {DEFINEFUNCDYN_PREF_ID16_IMM16_V8, "DEFINEFUNCDYN"},
            {DEFINENCFUNCDYN_PREF_ID16_IMM16_V8, "DEFINENCFUNCDYN"},
            {DEFINEGENERATORFUNC_PREF_ID16_IMM16_V8, "DEFINEGENERATORFUNC"},
            {DEFINEASYNCFUNC_PREF_ID16_IMM16_V8, "DEFINEASYNCFUNC"},
            {DEFINEMETHOD_PREF_ID16_IMM16_V8, "DEFINEMETHOD"},
            {NEWLEXENVDYN_PREF_IMM16, "NEWLEXENVDYN"},
            {COPYRESTARGS_PREF_IMM16, "COPYRESTARGS"},
            {CREATEARRAYWITHBUFFER_PREF_IMM16, "CREATEARRAYWITHBUFFER"},
            {CREATEOBJECTHAVINGMETHOD_PREF_IMM16, "CREATEOBJECTHAVINGMETHOD"},
            {THROWIFSUPERNOTCORRECTCALL_PREF_IMM16, "THROWIFSUPERNOTCORRECTCALL"},
            {CREATEOBJECTWITHBUFFER_PREF_IMM16, "CREATEOBJECTWITHBUFFER"},
            {LDLEXVARDYN_PREF_IMM4_IMM4, "LDLEXVARDYN"},
            {LDLEXVARDYN_PREF_IMM8_IMM8, "LDLEXVARDYN"},
            {LDLEXVARDYN_PREF_IMM16_IMM16, "LDLEXVARDYN"},
            {STLEXVARDYN_PREF_IMM4_IMM4_V8, "STLEXVARDYN"},
            {STLEXVARDYN_PREF_IMM8_IMM8_V8, "STLEXVARDYN"},
            {STLEXVARDYN_PREF_IMM16_IMM16_V8, "STLEXVARDYN"},
            {DEFINECLASSWITHBUFFER_PREF_ID16_IMM16_IMM16_V8_V8, "DEFINECLASSWITHBUFFER"},
            {IMPORTMODULE_PREF_ID32, "IMPORTMODULE"},
            {STMODULEVAR_PREF_ID32, "STMODULEVAR"},
            {TRYLDGLOBALBYNAME_PREF_ID32, "TRYLDGLOBALBYNAME"},
            {TRYSTGLOBALBYNAME_PREF_ID32, "TRYSTGLOBALBYNAME"},
            {LDGLOBALVAR_PREF_ID32, "LDGLOBALVAR"},
            {STGLOBALVAR_PREF_ID32, "STGLOBALVAR"},
            {LDOBJBYNAME_PREF_ID32_V8, "LDOBJBYNAME"},
            {STOBJBYNAME_PREF_ID32_V8, "STOBJBYNAME"},
            {STOWNBYNAME_PREF_ID32_V8, "STOWNBYNAME"},
            {LDSUPERBYNAME_PREF_ID32_V8, "LDSUPERBYNAME"},
            {STSUPERBYNAME_PREF_ID32_V8, "STSUPERBYNAME"},
            {LDMODVARBYNAME_PREF_ID32_V8, "LDMODVARBYNAME"},
            {CREATEREGEXPWITHLITERAL_PREF_ID32_IMM8, "CREATEREGEXPWITHLITERAL"},
            {ISTRUE_PREF, "ISTRUE"},
            {ISFALSE_PREF, "ISFALSE"},
            {STCONSTTOGLOBALRECORD_PREF_ID32, "STCONSTTOGLOBALRECORD"},
            {STLETTOGLOBALRECORD_PREF_ID32, "STLETTOGLOBALRECORD"},
            {STCLASSTOGLOBALRECORD_PREF_ID32, "STCLASSTOGLOBALRECORD"},
            {STOWNBYVALUEWITHNAMESET_PREF_V8_V8, "STOWNBYVALUEWITHNAMESET"},
            {STOWNBYNAMEWITHNAMESET_PREF_ID32_V8, "STOWNBYNAMEWITHNAMESET"},
            {LDFUNCTION_PREF, "LDFUNCTION"},
            {MOV_DYN_V8_V8, "MOV_DYN"},
            {MOV_DYN_V16_V16, "MOV_DYN"},
            {LDA_STR_ID32, "LDA_STR"},
            {LDAI_DYN_IMM32, "LDAI_DYN"},
            {FLDAI_DYN_IMM64, "FLDAI_DYN"},
            {JMP_IMM8, "JMP"},
            {JMP_IMM16, "JMP"},
            {JMP_IMM32, "JMP"},
            {JEQZ_IMM8, "JEQZ"},
            {JEQZ_IMM16, "JEQZ"},
            {LDA_DYN_V8, "LDA_DYN"},
            {STA_DYN_V8, "STA_DYN"},
            {RETURN_DYN, "RETURN_DYN"},
            {MOV_V4_V4, "MOV"},
            {JNEZ_IMM8, "JNEZ"},
            {JNEZ_IMM16, "JNEZ"},
            {LAST_OPCODE, "LAST_OPCODE"},
    };
    auto opcode = static_cast<EcmaOpcode>(*pc);
    if (strMap.count(opcode) > 0) {
        return strMap.at(opcode);
    }
    return "bytecode-" + std::to_string(opcode);
}

void ByteCodeCircuitBuilder::CollectBytecodeBlockInfo(uint8_t *pc,
    std::vector<std::tuple<uint8_t *, SplitPoint, std::vector<uint8_t *>>> &bytecodeBlockInfo)
{
    auto opcode = static_cast<EcmaOpcode>(*pc);
    switch (opcode) {
        case EcmaOpcode::JMP_IMM8: {
            int8_t offset = READ_INST_8_0();
            std::vector<uint8_t *> temp;
            temp.emplace_back(pc + offset);
            // current basic block end
            bytecodeBlockInfo.emplace_back(std::make_tuple(pc, SplitPoint::END, temp));
            bytecodeBlockInfo.emplace_back(
                std::make_tuple(pc + 2, SplitPoint::START, std::vector<uint8_t *>(1, pc + 2)));
            // jump basic block start
            bytecodeBlockInfo.emplace_back(
                std::make_tuple(pc + offset, SplitPoint::START, std::vector<uint8_t *>(1, pc + offset)));
        }
            break;
        case EcmaOpcode::JMP_IMM16: {
            int16_t offset = READ_INST_16_0();
            std::vector<uint8_t *> temp;
            temp.emplace_back(pc + offset);
            bytecodeBlockInfo.emplace_back(std::make_tuple(pc, SplitPoint::END, temp));
            bytecodeBlockInfo.emplace_back(
                std::make_tuple(pc + 3, SplitPoint::START, std::vector<uint8_t *>(1, pc + 3)));
            bytecodeBlockInfo.emplace_back(
                std::make_tuple(pc + offset, SplitPoint::START, std::vector<uint8_t *>(1, pc + offset)));
        }
            break;
        case EcmaOpcode::JMP_IMM32: {
            int32_t offset = READ_INST_32_0();
            std::vector<uint8_t *> temp;
            temp.emplace_back(pc + offset);
            bytecodeBlockInfo.emplace_back(std::make_tuple(pc, SplitPoint::END, temp));
            bytecodeBlockInfo.emplace_back(
                std::make_tuple(pc + 5, SplitPoint::START, std::vector<uint8_t *>(1, pc + 5)));
            bytecodeBlockInfo.emplace_back(
                std::make_tuple(pc + offset, SplitPoint::START, std::vector<uint8_t *>(1, pc + offset)));
        }
            break;
        case EcmaOpcode::JEQZ_IMM8: {
            std::vector<uint8_t *> temp;
            temp.emplace_back(pc + 2);   // first successor
            int8_t offset = READ_INST_8_0();
            temp.emplace_back(pc + offset);  // second successor
            // condition branch current basic block end
            bytecodeBlockInfo.emplace_back(std::make_tuple(pc, SplitPoint::END, temp));
            // first branch basic block start
            bytecodeBlockInfo.emplace_back(
                std::make_tuple(pc + 2, SplitPoint::START, std::vector<uint8_t *>(1, pc + 2)));
            // second branch basic block start
            bytecodeBlockInfo.emplace_back(
                std::make_tuple(pc + offset, SplitPoint::START, std::vector<uint8_t *>(1, pc + offset)));
        }
            break;
        case EcmaOpcode::JEQZ_IMM16: {
            std::vector<uint8_t *> temp;
            temp.emplace_back(pc + 3);   // first successor
            int16_t offset = READ_INST_16_0();
            temp.emplace_back(pc + offset);  // second successor
            bytecodeBlockInfo.emplace_back(std::make_tuple(pc, SplitPoint::END, temp)); // end
            bytecodeBlockInfo.emplace_back(
                std::make_tuple(pc + 3, SplitPoint::START, std::vector<uint8_t *>(1, pc + 3)));
            bytecodeBlockInfo.emplace_back(
                std::make_tuple(pc + offset, SplitPoint::START, std::vector<uint8_t *>(1, pc + offset)));
        }
            break;
        case EcmaOpcode::JNEZ_IMM8: {
            std::vector<uint8_t *> temp;
            temp.emplace_back(pc + 2);   // first successor
            int8_t offset = READ_INST_8_0();
            temp.emplace_back(pc + offset);  // second successor
            bytecodeBlockInfo.emplace_back(std::make_tuple(pc, SplitPoint::END, temp));
            bytecodeBlockInfo.emplace_back(
                std::make_tuple(pc + 2, SplitPoint::START, std::vector<uint8_t *>(1, pc + 2)));
            bytecodeBlockInfo.emplace_back(
                std::make_tuple(pc + offset, SplitPoint::START, std::vector<uint8_t *>(1, pc + offset)));
        }
            break;
        case EcmaOpcode::JNEZ_IMM16: {
            std::vector<uint8_t *> temp;
            temp.emplace_back(pc + 3);   // first successor
            int8_t offset = READ_INST_16_0();
            temp.emplace_back(pc + offset);  // second successor
            bytecodeBlockInfo.emplace_back(std::make_tuple(pc, SplitPoint::END, temp));
            bytecodeBlockInfo.emplace_back(
                std::make_tuple(pc + 3, SplitPoint::START, std::vector<uint8_t *>(1, pc + 3)));
            bytecodeBlockInfo.emplace_back(
                std::make_tuple(pc + offset, SplitPoint::START, std::vector<uint8_t *>(1, pc + offset)));
        }
            break;
        case EcmaOpcode::RETURN_DYN:
        case EcmaOpcode::RETURNUNDEFINED_PREF: {
            bytecodeBlockInfo.emplace_back(std::make_tuple(pc, SplitPoint::END, std::vector<uint8_t *>(1, pc)));
            break;
        }
        case EcmaOpcode::THROWDYN_PREF:
        case EcmaOpcode::THROWCONSTASSIGNMENT_PREF_V8:
        case EcmaOpcode::THROWTHROWNOTEXISTS_PREF:
        case EcmaOpcode::THROWPATTERNNONCOERCIBLE_PREF:
        case EcmaOpcode::THROWDELETESUPERPROPERTY_PREF: {
            bytecodeBlockInfo.emplace_back(std::make_tuple(pc, SplitPoint::END, std::vector<uint8_t *>(1, pc)));
        }
            break;
        default:
            break;
    }
}

std::map<std::pair<uint8_t *, uint8_t *>, std::vector<uint8_t *>> ByteCodeCircuitBuilder::CollectTryCatchBlockInfo(
    const panda_file::File &file, const JSMethod *method, std::map<uint8_t *, uint8_t *> &byteCodeCurPrePc,
    std::vector<std::tuple<uint8_t *, SplitPoint, std::vector<uint8_t *>>> &bytecodeBlockInfo)
{
    // try contains many catch
    std::map<std::pair<uint8_t *, uint8_t *>, std::vector<uint8_t *>> byteCodeException;
    panda_file::MethodDataAccessor mda(file, method->GetFileId());
    panda_file::CodeDataAccessor cda(file, mda.GetCodeId().value());
    cda.EnumerateTryBlocks([method, &byteCodeCurPrePc, &bytecodeBlockInfo, &byteCodeException](
            panda_file::CodeDataAccessor::TryBlock &try_block) {
        auto tryStartoffset = try_block.GetStartPc();
        auto tryEndoffset = try_block.GetStartPc() + try_block.GetLength();
        auto tryStartPc = const_cast<uint8_t *>(method->GetBytecodeArray() + tryStartoffset);
        auto tryEndPc = const_cast<uint8_t *>(method->GetBytecodeArray() + tryEndoffset);
        byteCodeException[std::make_pair(tryStartPc, tryEndPc)] = {};
        uint32_t pcOffset = panda_file::INVALID_OFFSET;
        try_block.EnumerateCatchBlocks([&](panda_file::CodeDataAccessor::CatchBlock &catch_block) {
            pcOffset = catch_block.GetHandlerPc();
            auto catchBlockPc = const_cast<uint8_t *>(method->GetBytecodeArray() + pcOffset);
            // try block associate catch block
            byteCodeException[std::make_pair(tryStartPc, tryEndPc)].emplace_back(catchBlockPc);
            return true;
        });
        // Check whether the previous block of the try block exists.
        // If yes, add the current block; otherwise, create a new block.
        bool flag = false;
        for (size_t i = 0; i < bytecodeBlockInfo.size(); i++) {
            if (std::get<1>(bytecodeBlockInfo[i]) == SplitPoint::START) {
                continue;
            }
            if (std::get<0>(bytecodeBlockInfo[i]) == byteCodeCurPrePc[tryStartPc]) {
                flag = true;
                break;
            }
        }
        if (!flag) {
            // pre block
            bytecodeBlockInfo.emplace_back(byteCodeCurPrePc[tryStartPc], SplitPoint::END,
                                           std::vector<uint8_t *>(1, tryStartPc));
        }
        bytecodeBlockInfo.emplace_back(tryStartPc, SplitPoint::START,
                                       std::vector<uint8_t *>(1, tryStartPc)); // try block
        flag = false;
        for (size_t i = 0; i < bytecodeBlockInfo.size(); i++) {
            if (std::get<1>(bytecodeBlockInfo[i]) == SplitPoint::START) {
                continue;
            }
            if (std::get<0>(bytecodeBlockInfo[i]) == byteCodeCurPrePc[tryEndPc]) {
                auto &succs = std::get<2>(bytecodeBlockInfo[i]);
                auto iter = std::find(succs.begin(), succs.end(), std::get<0>(bytecodeBlockInfo[i]));
                if (iter == succs.end()) {
                    auto opcode = static_cast<EcmaOpcode>(*(std::get<0>(bytecodeBlockInfo[i])));
                    switch (opcode) {
                        case EcmaOpcode::JMP_IMM8:
                        case EcmaOpcode::JMP_IMM16:
                        case EcmaOpcode::JMP_IMM32:
                        case EcmaOpcode::JEQZ_IMM8:
                        case EcmaOpcode::JEQZ_IMM16:
                        case EcmaOpcode::JNEZ_IMM8:
                        case EcmaOpcode::JNEZ_IMM16:
                        case EcmaOpcode::RETURN_DYN:
                        case EcmaOpcode::RETURNUNDEFINED_PREF:
                        case EcmaOpcode::THROWDYN_PREF: {
                            break;
                        }
                        default: {
                            succs.emplace_back(tryEndPc);
                            break;
                        }
                    }
                }
                flag = true;
                break;
            }
        }
        if (!flag) {
            bytecodeBlockInfo.emplace_back(
                    byteCodeCurPrePc[tryEndPc], SplitPoint::END, std::vector<uint8_t *>(1, tryEndPc));
        }
        bytecodeBlockInfo.emplace_back(tryEndPc, SplitPoint::START,
                                       std::vector<uint8_t *>(1, tryEndPc));  // next block
        return true;
    });
    return byteCodeException;
}

void ByteCodeCircuitBuilder::CompleteBytecodeBlockInfo(std::map<uint8_t *, uint8_t *> &byteCodeCurPrePc,
    std::vector<std::tuple<uint8_t *, SplitPoint, std::vector<uint8_t *>>> &bytecodeBlockInfo)
{
    // sort bytecodeBlockInfo
    Sort(bytecodeBlockInfo);

#if ECMASCRIPT_ENABLE_TS_AOT_PRINT
    PrintCollectBlockInfo(bytecodeBlockInfo);
#endif

    // Deduplicate
    auto deduplicateIndex = std::unique(bytecodeBlockInfo.begin(), bytecodeBlockInfo.end());
    bytecodeBlockInfo.erase(deduplicateIndex, bytecodeBlockInfo.end());

    // Supplementary block information
    std::vector<uint8_t *> endBlockPc;
    std::vector<uint8_t *> startBlockPc;
    for (size_t i = 0; i < bytecodeBlockInfo.size() - 1; i++) {
        if (std::get<1>(bytecodeBlockInfo[i]) == std::get<1>(bytecodeBlockInfo[i + 1]) &&
            std::get<1>(bytecodeBlockInfo[i]) == SplitPoint::START) {
            auto prePc = byteCodeCurPrePc[std::get<0>(bytecodeBlockInfo[i + 1])];
            endBlockPc.emplace_back(prePc); // Previous instruction of current instruction
            endBlockPc.emplace_back(std::get<0>(bytecodeBlockInfo[i + 1])); // current instruction
            continue;
        }
        if (std::get<1>(bytecodeBlockInfo[i]) == std::get<1>(bytecodeBlockInfo[i + 1]) &&
            std::get<1>(bytecodeBlockInfo[i]) == SplitPoint::END) {
            auto tempPc = std::get<0>(bytecodeBlockInfo[i]);
            auto findItem = std::find_if(byteCodeCurPrePc.begin(), byteCodeCurPrePc.end(),
                                         [tempPc](const std::map<uint8_t *, uint8_t *>::value_type item) {
                                             return item.second == tempPc;
                                         });
            if (findItem != byteCodeCurPrePc.end()) {
                startBlockPc.emplace_back((*findItem).first);
            }
        }
    }

    // Supplementary end block info
    for (auto iter = endBlockPc.begin(); iter != endBlockPc.end(); iter += 2) {
        bytecodeBlockInfo.emplace_back(
                std::make_tuple(*iter, SplitPoint::END, std::vector<uint8_t *>(1, *(iter + 1))));
    }
    // Supplementary start block info
    for (auto iter = startBlockPc.begin(); iter != startBlockPc.end(); iter++) {
        bytecodeBlockInfo.emplace_back(std::make_tuple(*iter, SplitPoint::START, std::vector<uint8_t *>(1, *iter)));
    }

    // Deduplicate successor
    for (size_t i = 0; i < bytecodeBlockInfo.size(); i++) {
        if (std::get<1>(bytecodeBlockInfo[i]) == SplitPoint::END) {
            std::set<uint8_t *> tempSet(std::get<2>(bytecodeBlockInfo[i]).begin(),
                                        std::get<2>(bytecodeBlockInfo[i]).end());
            std::get<2>(bytecodeBlockInfo[i]).assign(tempSet.begin(), tempSet.end());
        }
    }

    Sort(bytecodeBlockInfo);

    // handling jumps to an empty block
    auto endPc = std::get<0>(bytecodeBlockInfo[bytecodeBlockInfo.size() - 1]);
    auto iter = --byteCodeCurPrePc.end();
    if (endPc == iter->first) {
        bytecodeBlockInfo.emplace_back(std::make_tuple(endPc, SplitPoint::END, std::vector<uint8_t *>(1, endPc)));
    }
    // Deduplicate
    deduplicateIndex = std::unique(bytecodeBlockInfo.begin(), bytecodeBlockInfo.end());
    bytecodeBlockInfo.erase(deduplicateIndex, bytecodeBlockInfo.end());

#if ECMASCRIPT_ENABLE_TS_AOT_PRINT
    PrintCollectBlockInfo(bytecodeBlockInfo);
#endif
}

void ByteCodeCircuitBuilder::BuildBasicBlocks(const JSMethod *method,
    std::map<std::pair<uint8_t *, uint8_t *>, std::vector<uint8_t *>> &exception,
    std::vector<std::tuple<uint8_t *, SplitPoint, std::vector<uint8_t *>>> &bytecodeBlockInfo,
    std::map<uint8_t *, uint8_t *> &byteCodeCurPrePc)
{

    std::map<uint8_t *, ByteCodeBasicBlock *> map1; // [start, bb]
    std::map<uint8_t *, ByteCodeBasicBlock *> map2; // [end, bb]
    ByteCodeGraph byteCodeGraph;
    auto &blocks = byteCodeGraph.graph;
    byteCodeGraph.method = method;
    blocks.resize(bytecodeBlockInfo.size() / 2);
    // build basic block
    int blockId = 0;
    int index = 0;
    for (size_t i = 0; i < bytecodeBlockInfo.size() - 1; i += 2) {
        auto startPc = std::get<0>(bytecodeBlockInfo[i]);
        auto endPc = std::get<0>(bytecodeBlockInfo[i + 1]);
        auto block = &blocks[index++];
        block->id = blockId++;
        block->start = startPc;
        block->end = endPc;
        block->preds = {};
        block->succs = {};
        map1[startPc] = block;
        map2[endPc] = block;
    }

    // add block associate
    for (size_t i = 0; i < bytecodeBlockInfo.size(); i++) {
        if (std::get<1>(bytecodeBlockInfo[i]) == SplitPoint::START) {
            continue;
        }
        auto curPc = std::get<0>(bytecodeBlockInfo[i]);
        auto successors = std::get<2>(bytecodeBlockInfo[i]);
        for (size_t j = 0; j < successors.size(); j++) {
            if (successors[j] == curPc) {
                continue;
            }
            auto curBlock = map2[curPc];
            auto succsBlock = map1[successors[j]];
            curBlock->succs.emplace_back(succsBlock);
            succsBlock->preds.emplace_back(curBlock);
        }
    }

    // try catch block associate
    for (size_t i = 0; i < blocks.size(); i++) {
        auto pc = blocks[i].start;
        auto it = exception.begin();
        for (; it != exception.end(); it++) {
            if (pc < it->first.first || pc >= it->first.second) { // try block interval
                continue;
            }
            auto catchs = exception[it->first]; // catchs start pc
            for (size_t j = i + 1; j < blocks.size(); j++) {
                if (std::find(catchs.begin(), catchs.end(), blocks[j].start) != catchs.end()) {
                    blocks[i].catchs.insert(blocks[i].catchs.begin(), &blocks[j]);
                    blocks[i].succs.emplace_back(&blocks[j]);
                    blocks[j].preds.emplace_back(&blocks[i]);
                }
            }
        }
    }

#if ECMASCRIPT_ENABLE_TS_AOT_PRINT
    PrintGraph(byteCodeGraph.graph);
#endif
    ComputeDominatorTree(byteCodeGraph);
}

void ByteCodeCircuitBuilder::ComputeDominatorTree(ByteCodeGraph &byteCodeGraph)
{
    auto &graph = byteCodeGraph.graph;
    // Construct graph backward order
    std::map<size_t, size_t> dfsTimestamp; // (basicblock id, dfs order)
    size_t timestamp = 0;
    std::deque<size_t> pendingList;
    std::vector<size_t> visited(graph.size(), 0);
    auto basicBlockId = graph[0].id;
    pendingList.push_back(basicBlockId);
    while (!pendingList.empty()) {
        auto &curBlockId = pendingList.back();
        pendingList.pop_back();
        dfsTimestamp[curBlockId] = timestamp++;
        for (auto &succBlock: graph[curBlockId].succs) {
            if (visited[succBlock->id] == 0) {
                visited[succBlock->id] = 1;
                pendingList.push_back(succBlock->id);
            }
        }
    }

    DeadCodeRemove(dfsTimestamp, byteCodeGraph);

#if ECMASCRIPT_ENABLE_TS_AOT_PRINT
    // print cfg order
    for (auto iter: dfsTimestamp) {
        std::cout << "BB_" << iter.first << " depth is : " << iter.second << std::endl;
    }
#endif
    std::vector<size_t> immDom(graph.size()); // immediate dominator
    std::vector<std::vector<size_t>> dom(graph.size()); // dominators set
    dom[0] = {0};
    for (size_t i = 1; i < dom.size(); i++) {
        dom[i].resize(dom.size());
        std::iota(dom[i].begin(), dom[i].end(), 0);
    }
    bool changed = true;
    while (changed) {
        changed = false;
        for (size_t i = 1; i < dom.size(); i++) {
            if (graph[i].isDead) {
                continue;
            }
            auto &curDom = dom[i];
            size_t curDomSize = curDom.size();
            curDom.resize(dom.size());
            std::iota(curDom.begin(), curDom.end(), 0);
            // traverse the predecessor nodes of the current node, Computing Dominators
            for (auto &preBlock: graph[i].preds) {
                std::vector<size_t> tmp(curDom.size());
                auto preDom = dom[preBlock->id];
                auto it = std::set_intersection(
                        curDom.begin(), curDom.end(), preDom.begin(), preDom.end(), tmp.begin());
                tmp.resize(it - tmp.begin());
                curDom = tmp;
            }
            auto it = std::find(curDom.begin(), curDom.end(), i);
            if (it == curDom.end()) {
                curDom.push_back(i);
                std::sort(curDom.begin(), curDom.end());
            }

            if (dom[i].size() != curDomSize) {
                changed = true;
            }
        }
    }

#if ECMASCRIPT_ENABLE_TS_AOT_PRINT
    // print dominators set
    for (size_t i = 0; i < dom.size(); i++) {
        std::cout << "block " << i << " dominator blocks has: ";
        for (auto j: dom[i]) {
            std::cout << j << " , ";
        }
        std::cout << std::endl;
    }
#endif

    // compute immediate dominator
    immDom[0] = dom[0].front();
    for (size_t i = 1; i < dom.size(); i++) {
        if (graph[i].isDead) {
            continue;
        }
        auto it = std::remove(dom[i].begin(), dom[i].end(), i);
        dom[i].resize(it - dom[i].begin());
        immDom[i] = *std::max_element(dom[i].begin(), dom[i].end(),
                                      [graph, dfsTimestamp](size_t lhs, size_t rhs) -> bool {
                                          return dfsTimestamp.at(graph[lhs].id) < dfsTimestamp.at(graph[rhs].id);
                                      });
    }

#if ECMASCRIPT_ENABLE_TS_AOT_PRINT
    // print immediate dominator
    for (size_t i = 0; i < immDom.size(); i++) {
        std::cout << i << " immediate dominator: " << immDom[i] << std::endl;
    }
    PrintGraph(graph);
#endif
    BuildImmediateDominator(immDom, byteCodeGraph);
}

void ByteCodeCircuitBuilder::BuildImmediateDominator(std::vector<size_t> &immDom, ByteCodeGraph &byteCodeGraph)
{
    auto &graph = byteCodeGraph.graph;
    std::map<size_t, ByteCodeBasicBlock *> map;
    for (size_t i = 0; i < graph.size(); i++) {
        map[graph.at(i).id] = &graph.at(i);
    }

    graph[0].iDominator = &graph[0];
    for (size_t i = 1; i < immDom.size(); i++) {
        auto dominatedBlock = map.at(i);
        if (dominatedBlock->isDead) {
            continue;
        }
        auto immDomBlock = map.at(immDom[i]);
        dominatedBlock->iDominator = immDomBlock;
    }

#if ECMASCRIPT_ENABLE_TS_AOT_PRINT
    for (auto block: graph) {
        if (block.isDead) {
            continue;
        }
        std::cout << "current block " << block.id
                  << " immediate dominator block id: " << block.iDominator->id << std::endl;
    }
#endif

    for (auto &block: graph) {
        if (block.isDead) {
            continue;
        }
        if (block.iDominator->id != block.id) {
            block.iDominator->immDomBlocks.emplace_back(&block);
        }
    }

#if ECMASCRIPT_ENABLE_TS_AOT_PRINT
    for (auto &block: graph) {
        if (block.isDead) {
            continue;
        }
        std::cout << "block " << block.id << " dominat block has: ";
        for (size_t i = 0; i < block.immDomBlocks.size(); i++) {
            std::cout << block.immDomBlocks[i]->id << ",";
        }
        std::cout << std::endl;
    }
#endif
    ComputeDomFrontiers(immDom, byteCodeGraph);
    InsertPhi(byteCodeGraph);
    BuildCircuit(byteCodeGraph);
}

void ByteCodeCircuitBuilder::ComputeDomFrontiers(std::vector<size_t> &immDom, ByteCodeGraph &byteCodeGraph)
{
    auto &graph = byteCodeGraph.graph;
    std::map<size_t, ByteCodeBasicBlock *> map;
    for (size_t i = 0; i < graph.size(); i++) {
        map[graph.at(i).id] = &graph.at(i);
    }
    std::vector<std::set<ByteCodeBasicBlock *>> domFrontiers(immDom.size());
    for (auto &bb: graph) {
        if (bb.isDead) {
            continue;
        }
        if (bb.preds.size() < 2) {
            continue;
        }
        for (size_t i = 0; i < bb.preds.size(); i++) {
            auto runner = bb.preds[i];
            while (runner->id != immDom[bb.id]) {
                domFrontiers[runner->id].insert(&bb);
                runner = map.at(immDom[runner->id]);
            }
        }
    }

    for (size_t i = 0; i < domFrontiers.size(); i++) {
        for (auto iter = domFrontiers[i].begin(); iter != domFrontiers[i].end(); iter++) {
            graph[i].domFrontiers.emplace_back(*iter);
        }
    }

#if ECMASCRIPT_ENABLE_TS_AOT_PRINT
    for (size_t i = 0; i < domFrontiers.size(); i++) {
        std::cout << "basic block " << i << " dominate Frontiers is: ";
        for (auto iter = domFrontiers[i].begin(); iter != domFrontiers[i].end(); iter++) {
            std::cout << (*iter)->id << " , ";
        }
        std::cout << std::endl;
    }
#endif
}

void ByteCodeCircuitBuilder::DeadCodeRemove(const std::map<size_t, size_t> &dfsTimestamp, ByteCodeGraph &byteCodeGraph)
{
    auto &graph = byteCodeGraph.graph;
    for (auto &block: graph) {
        std::vector<ByteCodeBasicBlock *> newPreds;
        for (auto &bb: block.preds) {
            if (dfsTimestamp.count(bb->id)) {
                newPreds.emplace_back(bb);
            }
        }
        block.preds = newPreds;
    }

    for (auto &block: graph) {
        block.isDead = !dfsTimestamp.count(block.id);
        if (block.isDead) {
            block.succs.clear();
        }
    }
}

ByteCodeInfo ByteCodeCircuitBuilder::GetByteCodeInfo(uint8_t *pc)
{
    ByteCodeInfo info;
    auto opcode = static_cast<EcmaOpcode>(*pc);
    info.opcode = opcode;
    switch (opcode) {
        case EcmaOpcode::MOV_V4_V4: {
            uint16_t vdst = READ_INST_4_0();
            uint16_t vsrc = READ_INST_4_1();
            info.vregIn.emplace_back(vsrc);
            info.vregOut.emplace_back(vdst);
            info.offset = 2;
            break;
        }
        case EcmaOpcode::MOV_DYN_V8_V8: {
            uint16_t vdst = READ_INST_8_0();
            uint16_t vsrc = READ_INST_8_1();
            info.vregIn.emplace_back(vsrc);
            info.vregOut.emplace_back(vdst);
            info.offset = 3;
            break;
        }
        case EcmaOpcode::MOV_DYN_V16_V16: {
            uint16_t vdst = READ_INST_16_0();
            uint16_t vsrc = READ_INST_16_2();
            info.vregIn.emplace_back(vsrc);
            info.vregOut.emplace_back(vdst);
            info.offset = 5;
            break;
        }
        case EcmaOpcode::LDA_STR_ID32: {
            info.accOut = true;
            info.offset = 5;
            break;
        }
        case EcmaOpcode::JMP_IMM8: {
            info.offset = 2;
            break;
        }
        case EcmaOpcode::JMP_IMM16: {
            info.offset = 3;
            break;
        }
        case EcmaOpcode::JMP_IMM32: {
            info.offset = 5;
            break;
        }
        case EcmaOpcode::JEQZ_IMM8: {
            info.accIn = true;
            info.offset = 2;
            break;
        }
        case EcmaOpcode::JEQZ_IMM16: {
            info.accIn = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::JNEZ_IMM8: {
            info.accIn = true;
            info.offset = 2;
            break;
        }
        case EcmaOpcode::JNEZ_IMM16: {
            info.accIn = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::LDA_DYN_V8: {
            uint16_t vsrc = READ_INST_8_0();
            info.vregIn.emplace_back(vsrc);
            info.accOut = true;
            info.offset = 2;
            break;
        }
        case EcmaOpcode::STA_DYN_V8: {
            uint16_t vdst = READ_INST_8_0();
            info.vregOut.emplace_back(vdst);
            info.accIn = true;
            info.offset = 2;
            break;
        }
        case EcmaOpcode::LDAI_DYN_IMM32: {
            info.accOut = true;
            info.offset = 5;
            break;
        }
        case EcmaOpcode::FLDAI_DYN_IMM64: {
            info.accOut = true;
            info.offset = 9;
            break;
        }
        case EcmaOpcode::CALLARG0DYN_PREF_V8: {
            uint32_t funcReg = READ_INST_8_1();
            info.vregIn.emplace_back(funcReg);
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::CALLARG1DYN_PREF_V8_V8: {
            uint32_t funcReg = READ_INST_8_1();
            uint32_t reg = READ_INST_8_2();
            info.vregIn.emplace_back(funcReg);
            info.vregIn.emplace_back(reg);
            info.accOut = true;
            info.offset = 4;
            break;
        }
        case EcmaOpcode::CALLARGS2DYN_PREF_V8_V8_V8: {
            uint32_t funcReg = READ_INST_8_1();
            uint32_t reg = READ_INST_8_3();
            info.vregIn.emplace_back(funcReg);
            info.vregIn.emplace_back(reg);
            info.accOut = true;
            info.offset = 5;
            break;
        }
        case EcmaOpcode::CALLARGS3DYN_PREF_V8_V8_V8_V8: {
            uint32_t funcReg = READ_INST_8_1();
            uint32_t reg = READ_INST_8_4();
            info.vregIn.emplace_back(funcReg);
            info.vregIn.emplace_back(reg);
            info.accOut = true;
            info.offset = 6;
            break;
        }
        case EcmaOpcode::CALLITHISRANGEDYN_PREF_IMM16_V8: {
            uint32_t funcReg = READ_INST_8_3();
            uint32_t actualNumArgs = READ_INST_16_1() - 1;
            size_t copyArgs = actualNumArgs + NUM_MANDATORY_JSFUNC_ARGS - 2;
            info.vregIn.emplace_back(funcReg);
            for (size_t i = 1; i <= copyArgs; i++) {
                info.vregIn.emplace_back(funcReg + i);
            }
            info.accOut = true;
            info.offset = 5;
            break;
        }
        case EcmaOpcode::CALLSPREADDYN_PREF_V8_V8_V8: {
            uint16_t v0 = READ_INST_8_1();
            uint16_t v1 = READ_INST_8_2();
            uint16_t v2 = READ_INST_8_3();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v1);
            info.vregIn.emplace_back(v2);
            info.accOut = true;
            info.offset = 5;
            break;
        }
        case EcmaOpcode::CALLIRANGEDYN_PREF_IMM16_V8: {
            uint32_t funcReg = READ_INST_8_3();
            uint32_t actualNumArgs = READ_INST_16_1();
            size_t copyArgs = actualNumArgs + NUM_MANDATORY_JSFUNC_ARGS - 2;
            info.vregIn.emplace_back(funcReg);
            for (size_t i = 1; i <= copyArgs; i++) {
                info.vregIn.emplace_back(funcReg + i);
            }
            info.accOut = true;
            info.offset = 5;
            break;
        }
        case EcmaOpcode::RETURN_DYN: {
            info.accIn = true;
            info.accOut = true;
            info.offset = 1;
            break;
        }
        case EcmaOpcode::RETURNUNDEFINED_PREF: {
            info.accIn = true;
            info.accOut = true;
            info.offset = 2;
            break;
        }
        case EcmaOpcode::LDNAN_PREF: {
            info.accOut = true;
            info.offset = 2;
            break;
        }
        case EcmaOpcode::LDINFINITY_PREF: {
            info.accOut = true;
            info.offset = 2;
            break;
        }
        case EcmaOpcode::LDGLOBALTHIS_PREF: {
            info.accOut = true;
            info.offset = 2;
            break;
        }
        case EcmaOpcode::LDUNDEFINED_PREF: {
            info.accOut = true;
            info.offset = 2;
            break;
        }
        case EcmaOpcode::LDNULL_PREF: {
            info.accOut = true;
            info.offset = 2;
            break;
        }
        case EcmaOpcode::LDSYMBOL_PREF: {
            info.accOut = true;
            info.offset = 2;
            break;
        }
        case EcmaOpcode::LDGLOBAL_PREF: {
            info.accOut = true;
            info.offset = 2;
            break;
        }
        case EcmaOpcode::LDTRUE_PREF: {
            info.accOut = true;
            info.offset = 2;
            break;
        }
        case EcmaOpcode::LDFALSE_PREF: {
            info.accOut = true;
            info.offset = 2;
            break;
        }
        case EcmaOpcode::LDLEXENVDYN_PREF: {
            info.accOut = true;
            info.offset = 2;
            break;
        }
        case EcmaOpcode::GETUNMAPPEDARGS_PREF: {
            info.accOut = true;
            info.offset = 2;
            break;
        }
        case EcmaOpcode::ASYNCFUNCTIONENTER_PREF: {
            info.accOut = true;
            info.offset = 2;
            break;
        }
        case EcmaOpcode::TONUMBER_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::NEGDYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::NOTDYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::INCDYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::DECDYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::THROWDYN_PREF: {
            info.accIn = true;
            info.offset = 2;
            break;
        }
        case EcmaOpcode::TYPEOFDYN_PREF: {
            info.accIn = true;
            info.accOut = true;
            info.offset = 2;
            break;
        }
        case EcmaOpcode::GETPROPITERATOR_PREF: {
            info.accIn = true;
            info.accOut = true;
            info.offset = 2;
            break;
        }
        case EcmaOpcode::RESUMEGENERATOR_PREF_V8: {
            uint16_t vs = READ_INST_8_1();
            info.vregIn.emplace_back(vs);
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::GETRESUMEMODE_PREF_V8: {
            uint16_t vs = READ_INST_8_1();
            info.vregIn.emplace_back(vs);
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::GETITERATOR_PREF: {
            info.accIn = true;
            info.accOut = true;
            info.offset = 2;
            break;
        }
        case EcmaOpcode::THROWCONSTASSIGNMENT_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.offset = 3;
            break;
        }
        case EcmaOpcode::THROWTHROWNOTEXISTS_PREF: {
            info.offset = 2;
            break;
        }
        case EcmaOpcode::THROWPATTERNNONCOERCIBLE_PREF: {
            info.offset = 2;
            break;
        }
        case EcmaOpcode::THROWIFNOTOBJECT_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.offset = 3;
            break;
        }
        case EcmaOpcode::ITERNEXT_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::CLOSEITERATOR_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::ADD2DYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::SUB2DYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::MUL2DYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::DIV2DYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::MOD2DYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::EQDYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::NOTEQDYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::LESSDYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::LESSEQDYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::GREATERDYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::GREATEREQDYN_PREF_V8: {
            uint16_t vs = READ_INST_8_1();
            info.vregIn.emplace_back(vs);
            info.accIn = true;
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::SHL2DYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::SHR2DYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::ASHR2DYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::AND2DYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::OR2DYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::XOR2DYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::DELOBJPROP_PREF_V8_V8: {
            uint16_t v0 = READ_INST_8_1();
            uint16_t v1 = READ_INST_8_2();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v1);
            info.accOut = true;
            info.offset = 4;
            break;
        }
        case EcmaOpcode::DEFINEFUNCDYN_PREF_ID16_IMM16_V8: {
            uint16_t v0 = READ_INST_8_5();
            info.vregIn.emplace_back(v0);
            info.accOut = true;
            info.offset = 7;
            break;
        }
        case EcmaOpcode::DEFINENCFUNCDYN_PREF_ID16_IMM16_V8: {
            uint16_t v0 = READ_INST_8_5();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = 7;
            break;
        }
        case EcmaOpcode::DEFINEMETHOD_PREF_ID16_IMM16_V8: {
            uint16_t v0 = READ_INST_8_5();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = 7;
            break;
        }
        case EcmaOpcode::NEWOBJDYNRANGE_PREF_IMM16_V8: {
            uint16_t firstArgRegIdx = READ_INST_8_3();
            info.vregIn.emplace_back(firstArgRegIdx);
            info.vregIn.emplace_back(firstArgRegIdx + 1);
            info.accOut = true;
            info.offset = 5;
            break;
        }
        case EcmaOpcode::EXPDYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::ISINDYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::INSTANCEOFDYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::STRICTNOTEQDYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::STRICTEQDYN_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::LDLEXVARDYN_PREF_IMM16_IMM16: {
            info.accOut = true;
            info.offset = 6;
            break;
        }
        case EcmaOpcode::LDLEXVARDYN_PREF_IMM8_IMM8: {
            info.accOut = true;
            info.offset = 4;
            break;
        }
        case EcmaOpcode::LDLEXVARDYN_PREF_IMM4_IMM4: {
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::STLEXVARDYN_PREF_IMM16_IMM16_V8: {
            uint16_t v0 = READ_INST_8_5();
            info.vregIn.emplace_back(v0);
            info.offset = 7;
            break;
        }
        case EcmaOpcode::STLEXVARDYN_PREF_IMM8_IMM8_V8: {
            uint16_t v0 = READ_INST_8_5();
            info.vregIn.emplace_back(v0);
            info.offset = 5;
            break;
        }
        case EcmaOpcode::STLEXVARDYN_PREF_IMM4_IMM4_V8: {
            uint16_t v0 = READ_INST_8_5();
            info.vregIn.emplace_back(v0);
            info.offset = 4;
            break;
        }
        case EcmaOpcode::NEWLEXENVDYN_PREF_IMM16: {
            uint16_t v0 = READ_INST_8_5();
            info.vregIn.emplace_back(v0);
            info.offset = 4;
            break;
        }
        case EcmaOpcode::POPLEXENVDYN_PREF: {
            info.offset = 2;
            break;
        }
        case EcmaOpcode::CREATEITERRESULTOBJ_PREF_V8_V8: {
            uint16_t v0 = READ_INST_8_1();
            uint16_t v1 = READ_INST_8_2();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v1);
            info.accOut = true;
            info.offset = 4;
            break;
        }
        case EcmaOpcode::SUSPENDGENERATOR_PREF_V8_V8: {
            uint16_t v0 = READ_INST_8_1();
            uint16_t v1 = READ_INST_8_2();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v1);
            info.accIn = true;
            info.accOut = true;
            info.offset = 4;
            break;
        }
        case EcmaOpcode::ASYNCFUNCTIONAWAITUNCAUGHT_PREF_V8_V8: {
            uint16_t v0 = READ_INST_8_1();
            uint16_t v1 = READ_INST_8_2();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v1);
            info.accOut = true;
            info.offset = 4;
            break;
        }
        case EcmaOpcode::ASYNCFUNCTIONRESOLVE_PREF_V8_V8_V8: {
            uint16_t v0 = READ_INST_8_1();
            uint16_t v2 = READ_INST_8_3();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v2);
            info.accOut = true;
            info.offset = 5;
            break;
        }
        case EcmaOpcode::ASYNCFUNCTIONREJECT_PREF_V8_V8_V8: {
            uint16_t v0 = READ_INST_8_1();
            uint16_t v2 = READ_INST_8_3();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v2);
            info.offset = 5;
            break;
        }
        case EcmaOpcode::NEWOBJSPREADDYN_PREF_V8_V8: {
            uint16_t v0 = READ_INST_8_1();
            uint16_t v1 = READ_INST_8_2();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v1);
            info.accIn = true;
            info.accOut = true;
            info.offset = 4;
            break;
        }
        case EcmaOpcode::THROWUNDEFINEDIFHOLE_PREF_V8_V8: {
            uint16_t v0 = READ_INST_8_1();
            uint16_t v1 = READ_INST_8_2();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v1);
            info.offset = 4;
            break;
        }
        case EcmaOpcode::STOWNBYNAME_PREF_ID32_V8: {
            uint32_t v0 = READ_INST_8_5();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.offset = 7;
            break;
        }
        case EcmaOpcode::CREATEEMPTYARRAY_PREF: {
            info.accOut = true;
            info.offset = 2;
            break;
        }
        case EcmaOpcode::CREATEEMPTYOBJECT_PREF: {
            info.accOut = true;
            info.offset = 2;
            break;
        }
        case EcmaOpcode::CREATEOBJECTWITHBUFFER_PREF_IMM16: {
            info.accOut = true;
            info.offset = 4;
            break;
        }
        case EcmaOpcode::SETOBJECTWITHPROTO_PREF_V8_V8: {
            uint16_t v0 = READ_INST_8_1();
            uint16_t v1 = READ_INST_8_2();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v1);
            info.offset = 4;
            break;
        }
        case EcmaOpcode::CREATEARRAYWITHBUFFER_PREF_IMM16: {
            info.accOut = true;
            info.offset = 4;
            break;
        }
        case EcmaOpcode::IMPORTMODULE_PREF_ID32: {
            info.accOut = true;
            info.offset = 6;
            break;
        }
        case EcmaOpcode::STMODULEVAR_PREF_ID32: {
            info.accIn = true;
            info.offset = 6;
            break;
        }
        case EcmaOpcode::COPYMODULE_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.offset = 3;
            break;
        }
        case EcmaOpcode::LDMODVARBYNAME_PREF_ID32_V8: {
            uint32_t v0 = READ_INST_8_5();
            info.vregIn.emplace_back(v0);
            info.accOut = true;
            info.offset = 7;
            break;
        }
        case EcmaOpcode::CREATEREGEXPWITHLITERAL_PREF_ID32_IMM8: {
            info.accOut = true;
            info.offset = 7;
            break;
        }
        case EcmaOpcode::GETTEMPLATEOBJECT_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::GETNEXTPROPNAME_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::COPYDATAPROPERTIES_PREF_V8_V8: {
            uint16_t v0 = READ_INST_8_1();
            uint16_t v1 = READ_INST_8_2();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v1);
            info.accOut = true;
            info.offset = 4;
            break;
        }
        case EcmaOpcode::STOWNBYINDEX_PREF_V8_IMM32: {
            uint32_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.offset = 7;
            break;
        }
        case EcmaOpcode::STOWNBYVALUE_PREF_V8_V8: {
            uint32_t v0 = READ_INST_8_1();
            uint32_t v1 = READ_INST_8_2();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v1);
            info.accIn = true;
            info.offset = 4;
            break;
        }
        case EcmaOpcode::CREATEOBJECTWITHEXCLUDEDKEYS_PREF_IMM16_V8_V8: {
            uint16_t v0 = READ_INST_8_3();
            info.vregIn.emplace_back(v0);
            info.accOut = true;
            info.offset = 6;
            break;
        }
        case EcmaOpcode::DEFINEGENERATORFUNC_PREF_ID16_IMM16_V8: {
            uint16_t v0 = READ_INST_8_5();
            info.vregIn.emplace_back(v0);
            info.accOut = true;
            info.offset = 7;
            break;
        }
        case EcmaOpcode::DEFINEASYNCFUNC_PREF_ID16_IMM16_V8: {
            uint16_t v0 = READ_INST_8_5();
            info.vregIn.emplace_back(v0);
            info.accOut = true;
            info.offset = 7;
            break;
        }
        case EcmaOpcode::LDHOLE_PREF: {
            info.accOut = true;
            info.offset = 2;
            break;
        }
        case EcmaOpcode::COPYRESTARGS_PREF_IMM16: {
            info.offset = 4;
            break;
        }
        case EcmaOpcode::DEFINEGETTERSETTERBYVALUE_PREF_V8_V8_V8_V8: {
            uint16_t v0 = READ_INST_8_1();
            uint16_t v1 = READ_INST_8_2();
            uint16_t v2 = READ_INST_8_3();
            uint16_t v3 = READ_INST_8_4();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v1);
            info.vregIn.emplace_back(v2);
            info.vregIn.emplace_back(v3);
            info.accIn = true;
            info.accOut = true;
            info.offset = 6;
            break;
        }
        case EcmaOpcode::LDOBJBYINDEX_PREF_V8_IMM32: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accOut = true;
            info.offset = 7;
            break;
        }
        case EcmaOpcode::STOBJBYINDEX_PREF_V8_IMM32: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.offset = 7;
            break;
        }
        case EcmaOpcode::LDOBJBYVALUE_PREF_V8_V8: {
            uint32_t v0 = READ_INST_8_1();
            uint32_t v1 = READ_INST_8_2();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v1);
            info.accOut = true;
            info.offset = 4;
            break;
        }
        case EcmaOpcode::STOBJBYVALUE_PREF_V8_V8: {
            uint32_t v0 = READ_INST_8_1();
            uint32_t v1 = READ_INST_8_2();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v1);
            info.accIn = true;
            info.offset = 4;
            break;
        }
        case EcmaOpcode::LDSUPERBYVALUE_PREF_V8_V8: {
            uint32_t v0 = READ_INST_8_1();
            uint32_t v1 = READ_INST_8_2();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v1);
            info.accOut = true;
            info.offset = 4;
            break;
        }
        case EcmaOpcode::STSUPERBYVALUE_PREF_V8_V8: {
            uint32_t v0 = READ_INST_8_1();
            uint32_t v1 = READ_INST_8_2();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v1);
            info.accIn = true;
            info.offset = 4;
            break;
        }
        case EcmaOpcode::TRYLDGLOBALBYNAME_PREF_ID32: {
            info.accOut = true;
            info.offset = 6;
            break;
        }
        case EcmaOpcode::TRYSTGLOBALBYNAME_PREF_ID32: {
            info.accIn = true;
            info.offset = 6;
            break;
        }
        case EcmaOpcode::STCONSTTOGLOBALRECORD_PREF_ID32: {
            info.accIn = true;
            info.offset = 6;
            break;
        }
        case EcmaOpcode::STLETTOGLOBALRECORD_PREF_ID32: {
            info.accIn = true;
            info.offset = 6;
            break;
        }
        case EcmaOpcode::STCLASSTOGLOBALRECORD_PREF_ID32: {
            info.accIn = true;
            info.offset = 6;
            break;
        }
        case EcmaOpcode::STOWNBYVALUEWITHNAMESET_PREF_V8_V8: {
            uint32_t v0 = READ_INST_8_1();
            uint32_t v1 = READ_INST_8_2();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v1);
            info.accIn = true;
            info.offset = 4;
            break;
        }
        case EcmaOpcode::STOWNBYNAMEWITHNAMESET_PREF_ID32_V8: {
            uint32_t v0 = READ_INST_8_5();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.offset = 7;
            break;
        }
        case EcmaOpcode::LDGLOBALVAR_PREF_ID32: {
            info.accOut = true;
            info.offset = 6;
            break;
        }
        case EcmaOpcode::LDOBJBYNAME_PREF_ID32_V8: {
            uint32_t v0 = READ_INST_8_5();
            info.vregIn.emplace_back(v0);
            info.accOut = true;
            info.offset = 7;
            break;
        }
        case EcmaOpcode::STOBJBYNAME_PREF_ID32_V8: {
            uint32_t v0 = READ_INST_8_5();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.offset = 7;
            break;
        }
        case EcmaOpcode::LDSUPERBYNAME_PREF_ID32_V8: {
            uint32_t v0 = READ_INST_8_5();
            info.vregIn.emplace_back(v0);
            info.accOut = true;
            info.offset = 7;
            break;
        }
        case EcmaOpcode::STSUPERBYNAME_PREF_ID32_V8: {
            uint32_t v0 = READ_INST_8_5();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.offset = 7;
            break;
        }
        case EcmaOpcode::STGLOBALVAR_PREF_ID32: {
            info.accIn = true;
            info.offset = 6;
            break;
        }
        case EcmaOpcode::CREATEGENERATOROBJ_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::STARRAYSPREAD_PREF_V8_V8: {
            uint16_t v0 = READ_INST_8_1();
            uint16_t v1 = READ_INST_8_2();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v1);
            info.accIn = true;
            info.accOut = true;
            info.offset = 4;
            break;
        }
        case EcmaOpcode::GETITERATORNEXT_PREF_V8_V8: {
            uint16_t v0 = READ_INST_8_1();
            uint16_t v1 = READ_INST_8_2();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v1);
            info.accOut = true;
            info.offset = 4;
            break;
        }
        case EcmaOpcode::DEFINECLASSWITHBUFFER_PREF_ID16_IMM16_IMM16_V8_V8: {
            uint16_t v0 = READ_INST_8_7();
            uint16_t v1 = READ_INST_8_8();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v1);
            info.accOut = true;
            info.offset = 10;
            break;
        }
        case EcmaOpcode::LDFUNCTION_PREF: {
            info.accOut = true;
            info.offset = 2;
            break;
        }
        case EcmaOpcode::SUPERCALL_PREF_IMM16_V8: {
            info.accIn = true;
            info.accOut = true;
            info.offset = 5;
            break;
        }
        case EcmaOpcode::SUPERCALLSPREAD_PREF_V8: {
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::CREATEOBJECTHAVINGMETHOD_PREF_IMM16: {
            info.accIn = true;
            info.accOut = true;
            info.offset = 4;
            break;
        }
        case EcmaOpcode::THROWIFSUPERNOTCORRECTCALL_PREF_IMM16: {
            info.accIn = true;
            info.offset = 4;
            break;
        }
        case EcmaOpcode::LDHOMEOBJECT_PREF: {
            info.accOut = true;
            info.offset = 2;
            break;
        }
        case EcmaOpcode::THROWDELETESUPERPROPERTY_PREF: {
            info.offset = 2;
            break;
        }
        case EcmaOpcode::DEBUGGER_PREF: {
            info.offset = 2;
            break;
        }
        case EcmaOpcode::ISTRUE_PREF: {
            info.accIn = true;
            info.accOut = true;
            info.offset = 2;
            break;
        }
        case EcmaOpcode::ISFALSE_PREF: {
            info.accIn = true;
            info.accOut = true;
            info.offset = 2;
            break;
        }
        default: {
            std::cout << opcode << std::endl;
            abort();
            break;
        }
    }
    return info;
}

void ByteCodeCircuitBuilder::InsertPhi(ByteCodeGraph &byteCodeGraph)
{
    auto &graph = byteCodeGraph.graph;
    std::map<uint16_t, std::set<size_t>> defsitesInfo; // <vreg, bbs>
    for (auto &bb: graph) {
        if (bb.isDead) {
            continue;
        }
        auto pc = bb.start;
        while (pc <= bb.end) {
            auto bytecodeInfo = GetByteCodeInfo(pc);
            pc = pc + bytecodeInfo.offset; // next inst start pc
            for (const auto &vreg: bytecodeInfo.vregOut) {
                defsitesInfo[vreg].insert(bb.id);
            }
        }
    }

#if ECMASCRIPT_ENABLE_TS_AOT_PRINT
    for (const auto&[variable, defsites]: defsitesInfo) {
        std::cout << "variable: " << variable << " locate block have: ";
        for (auto id: defsites) {
            std::cout << id << " , ";
        }
        std::cout << std::endl;
    }
#endif

    for (const auto&[variable, defsites]: defsitesInfo) {
        std::queue<uint16_t> workList;
        for (auto blockId: defsites) {
            workList.push(blockId);
        }
        while (!workList.empty()) {
            auto currentId = workList.front();
            workList.pop();
            for (auto &block: graph[currentId].domFrontiers) {
                if (!block->phi.count(variable)) {
                    block->phi.insert(variable);
                    if (!defsitesInfo[variable].count(block->id)) {
                        workList.push(block->id);
                    }
                }
            }
        }
    }
#if ECMASCRIPT_ENABLE_TS_AOT_PRINT
    PrintGraph(graph);
#endif
}

bool ByteCodeCircuitBuilder::IsJump(EcmaOpcode opcode)
{
    switch (opcode) {
        case EcmaOpcode::JMP_IMM8:
        case EcmaOpcode::JMP_IMM16:
        case EcmaOpcode::JMP_IMM32:
        case EcmaOpcode::JEQZ_IMM8:
        case EcmaOpcode::JEQZ_IMM16:
        case EcmaOpcode::JNEZ_IMM8:
        case EcmaOpcode::JNEZ_IMM16:
            return true;
        default:
            return false;
    }
}

bool ByteCodeCircuitBuilder::IsCondJump(EcmaOpcode opcode)
{
    switch (opcode) {
        case EcmaOpcode::JEQZ_IMM8:
        case EcmaOpcode::JEQZ_IMM16:
        case EcmaOpcode::JNEZ_IMM8:
        case EcmaOpcode::JNEZ_IMM16:
            return true;
        default:
            return false;
    }
}

bool ByteCodeCircuitBuilder::IsMov(EcmaOpcode opcode)
{
    switch (opcode) {
        case EcmaOpcode::MOV_V4_V4:
        case EcmaOpcode::MOV_DYN_V8_V8:
        case EcmaOpcode::MOV_DYN_V16_V16:
        case EcmaOpcode::LDA_DYN_V8:
        case EcmaOpcode::STA_DYN_V8:
            return true;
        default:
            return false;
    }
}

bool ByteCodeCircuitBuilder::IsReturn(EcmaOpcode opcode)
{
    switch (opcode) {
        case EcmaOpcode::RETURN_DYN:
        case EcmaOpcode::RETURNUNDEFINED_PREF:
            return true;
        default:
            return false;
    }
}

bool ByteCodeCircuitBuilder::IsThrow(EcmaOpcode opcode)
{
    switch (opcode) {
        case EcmaOpcode::THROWDYN_PREF:
        case EcmaOpcode::THROWCONSTASSIGNMENT_PREF_V8:
        case EcmaOpcode::THROWTHROWNOTEXISTS_PREF:
        case EcmaOpcode::THROWPATTERNNONCOERCIBLE_PREF:
        case EcmaOpcode::THROWDELETESUPERPROPERTY_PREF:
            return true;
        default:
            return false;
    }
}

bool ByteCodeCircuitBuilder::IsGeneral(EcmaOpcode opcode)
{
    return !IsMov(opcode) && !IsJump(opcode) && !IsReturn(opcode) && !IsThrow(opcode);
}

void ByteCodeCircuitBuilder::BuildCircuit(ByteCodeGraph &byteCodeGraph)
{
    auto &graph = byteCodeGraph.graph;
    // workaround, remove this when the problem in preds succs trys catchs is fixed.
    for (auto &bb: graph) {
        if (bb.isDead) {
            continue;
        }
        bb.preds.clear();
        bb.trys.clear();
        std::vector<ByteCodeBasicBlock *> newSuccs;
        for (const auto &succ: bb.succs) {
            if (std::count(bb.catchs.begin(), bb.catchs.end(), succ)) {
                continue;
            }
            newSuccs.push_back(succ);
        }
        bb.succs = newSuccs;
    }
    for (auto &bb: graph) {
        if (bb.isDead) {
            continue;
        }
        for (auto &succ: bb.succs) {
            succ->preds.push_back(&bb);
        }
        for (auto &catchBlock: bb.catchs) {
            catchBlock->trys.push_back(&bb);
        }
    }

#if ECMASCRIPT_ENABLE_TS_AOT_PRINT
    PrintBBInfo(graph);
#endif

    // create arg gates
    const size_t numArgs = byteCodeGraph.method->GetNumArgs();
    const size_t offsetArgs = byteCodeGraph.method->GetNumVregs();
    std::vector<kungfu::GateRef> argGates(numArgs);
    for (size_t argIdx = 0; argIdx < numArgs; argIdx++) {
        argGates.at(argIdx) = circuit_.NewGate(kungfu::OpCode(kungfu::OpCode::ARG), kungfu::ValueCode::INT64,
                                               argIdx,
                                               {kungfu::Circuit::GetCircuitRoot(
                                                       kungfu::OpCode(kungfu::OpCode::ARG_LIST))},
                                               kungfu::TypeCode::NOTYPE);
    }
    // get number of state predicates of each block
    for (auto &bb: graph) {
        if (bb.isDead) {
            continue;
        }
        bb.numStatePred = 0;
    }
    for (auto &bb: graph) {
        if (bb.isDead) {
            continue;
        }
        auto pc = bb.start;
        while (pc <= bb.end) {
            auto bytecodeInfo = GetByteCodeInfo(pc);
            pc = pc + bytecodeInfo.offset; // next inst start pc
            if (IsGeneral(static_cast<EcmaOpcode>(bytecodeInfo.opcode))) {
                if (!bb.catchs.empty()) {
                    bb.catchs.at(0)->numStatePred++;
                }
            }
        }
        for (auto &succ: bb.succs) {
            succ->numStatePred++;
        }
    }
    // build entry of each block
    for (auto &bb: graph) {
        if (bb.isDead) {
            continue;
        }
        if (bb.numStatePred == 0) {
            bb.stateStart = kungfu::Circuit::GetCircuitRoot(kungfu::OpCode(kungfu::OpCode::STATE_ENTRY));
            bb.dependStart = kungfu::Circuit::GetCircuitRoot(kungfu::OpCode(kungfu::OpCode::DEPEND_ENTRY));
        } else if (bb.numStatePred == 1) {
            bb.stateStart = circuit_.NewGate(kungfu::OpCode(kungfu::OpCode::ORDINARY_BLOCK), 0,
                                             {kungfu::Circuit::NullGate()}, kungfu::TypeCode::NOTYPE);
            bb.dependStart = circuit_.NewGate(kungfu::OpCode(kungfu::OpCode::DEPEND_RELAY), 0,
                                              {bb.stateStart, kungfu::Circuit::NullGate()},
                                              kungfu::TypeCode::NOTYPE);
        } else {
            bb.stateStart = circuit_.NewGate(kungfu::OpCode(kungfu::OpCode::MERGE), bb.numStatePred,
                                             std::vector<kungfu::GateRef>(bb.numStatePred,
                                                                          kungfu::Circuit::NullGate()),
                                             kungfu::TypeCode::NOTYPE);
            bb.dependStart = circuit_.NewGate(kungfu::OpCode(kungfu::OpCode::DEPEND_SELECTOR), bb.numStatePred,
                                              std::vector<kungfu::GateRef>(bb.numStatePred + 1,
                                                                           kungfu::Circuit::NullGate()),
                                              kungfu::TypeCode::NOTYPE);
            circuit_.NewIn(bb.dependStart, 0, bb.stateStart);
        }
    }
    // build states sub-circuit_ of each block
    for (auto &bb: graph) {
        if (bb.isDead) {
            continue;
        }
        auto stateCur = bb.stateStart;
        auto dependCur = bb.dependStart;
        ASSERT(stateCur != kungfu::Circuit::NullGate());
        ASSERT(dependCur != kungfu::Circuit::NullGate());
        auto pc = bb.start;
        while (pc <= bb.end) {
            auto pcPrev = pc;
            auto bytecodeInfo = GetByteCodeInfo(pc);
            pc = pc + bytecodeInfo.offset; // next inst start pc
            size_t numValueInputs = (bytecodeInfo.accIn ? 1 : 0) + bytecodeInfo.vregIn.size();
            if (IsGeneral(static_cast<EcmaOpcode>(bytecodeInfo.opcode))) {
                auto gate = circuit_.NewGate(kungfu::OpCode(kungfu::OpCode::JS_BYTECODE), numValueInputs,
                                             std::vector<kungfu::GateRef>(
                                                     2 + numValueInputs, kungfu::Circuit::NullGate()),
                                             kungfu::TypeCode::NOTYPE);
                circuit_.NewIn(gate, 0, stateCur);
                circuit_.NewIn(gate, 1, dependCur);
                auto ifSuccess = circuit_.NewGate(kungfu::OpCode(kungfu::OpCode::IF_SUCCESS), 0,
                                                  {gate},
                                                  kungfu::TypeCode::NOTYPE);
                auto ifException = circuit_.NewGate(kungfu::OpCode(kungfu::OpCode::IF_EXCEPTION), 0,
                                                    {gate},
                                                    kungfu::TypeCode::NOTYPE);
                if (!bb.catchs.empty()) {
                    auto bbNext = bb.catchs.at(0);
                    circuit_.NewIn(bbNext->stateStart, bbNext->cntStatePred, ifException);
                    circuit_.NewIn(bbNext->dependStart, bbNext->cntStatePred + 1, gate);
                    bbNext->cntStatePred++;
                    bbNext->realPreds.push_back({bb.id, pcPrev, true});
                    ASSERT(bbNext->cntStatePred <= bbNext->numStatePred);
                } else {
                    circuit_.NewGate(kungfu::OpCode(kungfu::OpCode::THROW), 0,
                                     {ifException, gate, gate,
                                      kungfu::Circuit::GetCircuitRoot(kungfu::OpCode(kungfu::OpCode::THROW_LIST))},
                                     kungfu::TypeCode::NOTYPE);
                }
                stateCur = ifSuccess;
                dependCur = gate;
                gateToByteCode_[gate] = {bb.id, pcPrev};
                if (pcPrev == bb.end) {
                    auto bbNext = &graph.at(bb.id + 1);
                    circuit_.NewIn(bbNext->stateStart, bbNext->cntStatePred, stateCur);
                    circuit_.NewIn(bbNext->dependStart, bbNext->cntStatePred + 1, dependCur);
                    bbNext->cntStatePred++;
                    bbNext->realPreds.push_back({bb.id, pcPrev, false});
                    ASSERT(bbNext->cntStatePred <= bbNext->numStatePred);
                }
            } else if (IsJump(static_cast<EcmaOpcode>(bytecodeInfo.opcode))) {
                if (IsCondJump(static_cast<EcmaOpcode>(bytecodeInfo.opcode))) {
                    auto gate = circuit_.NewGate(kungfu::OpCode(kungfu::OpCode::JS_BYTECODE), numValueInputs,
                                                 std::vector<kungfu::GateRef>(
                                                         2 + numValueInputs, kungfu::Circuit::NullGate()),
                                                 kungfu::TypeCode::NOTYPE);
                    circuit_.NewIn(gate, 0, stateCur);
                    circuit_.NewIn(gate, 1, dependCur);
                    auto ifTrue = circuit_.NewGate(kungfu::OpCode(kungfu::OpCode::IF_TRUE), 0,
                                                   {gate},
                                                   kungfu::TypeCode::NOTYPE);
                    auto ifFalse = circuit_.NewGate(kungfu::OpCode(kungfu::OpCode::IF_FALSE), 0,
                                                    {gate},
                                                    kungfu::TypeCode::NOTYPE);
                    ASSERT(bb.succs.size() == 2);
                    int bitSet = 0;
                    for (auto &bbNext: bb.succs) {
                        if (bbNext->id == bb.id + 1) {
                            circuit_.NewIn(bbNext->stateStart, bbNext->cntStatePred, ifFalse);
                            circuit_.NewIn(bbNext->dependStart, bbNext->cntStatePred + 1, gate);
                            bbNext->cntStatePred++;
                            bbNext->realPreds.push_back({bb.id, pcPrev, false});
                            ASSERT(bbNext->cntStatePred <= bbNext->numStatePred);
                            bitSet |= 1;
                        } else {
                            circuit_.NewIn(bbNext->stateStart, bbNext->cntStatePred, ifTrue);
                            circuit_.NewIn(bbNext->dependStart, bbNext->cntStatePred + 1, gate);
                            bbNext->cntStatePred++;
                            bbNext->realPreds.push_back({bb.id, pcPrev, false});
                            ASSERT(bbNext->cntStatePred <= bbNext->numStatePred);
                            bitSet |= 2;
                        }
                    }
                    ASSERT(bitSet == 3);
                    gateToByteCode_[gate] = {bb.id, pcPrev};
                    break;
                } else {
                    ASSERT(bb.succs.size() == 1);
                    auto bbNext = bb.succs.at(0);
                    circuit_.NewIn(bbNext->stateStart, bbNext->cntStatePred, stateCur);
                    circuit_.NewIn(bbNext->dependStart, bbNext->cntStatePred + 1, dependCur);
                    bbNext->cntStatePred++;
                    bbNext->realPreds.push_back({bb.id, pcPrev, false});
                    ASSERT(bbNext->cntStatePred <= bbNext->numStatePred);
                    break;
                }
            } else if (static_cast<EcmaOpcode>(bytecodeInfo.opcode) == EcmaOpcode::RETURN_DYN) {
                ASSERT(bb.succs.empty());
                auto gate = circuit_.NewGate(kungfu::OpCode(kungfu::OpCode::RETURN), 0,
                                             {stateCur, dependCur, kungfu::Circuit::NullGate(),
                                              kungfu::Circuit::GetCircuitRoot(
                                                      kungfu::OpCode(kungfu::OpCode::RETURN_LIST))},
                                             kungfu::TypeCode::NOTYPE);
                gateToByteCode_[gate] = {bb.id, pcPrev};
                break;
            } else if (static_cast<EcmaOpcode>(bytecodeInfo.opcode) == EcmaOpcode::RETURNUNDEFINED_PREF) {
                ASSERT(bb.succs.empty());
                auto constant = circuit_.NewGate(kungfu::OpCode(kungfu::OpCode::CONSTANT), kungfu::ValueCode::INT64,
                                                 TaggedValue::VALUE_UNDEFINED, {kungfu::Circuit::GetCircuitRoot(
                                kungfu::OpCode(kungfu::OpCode::CONSTANT_LIST))},
                                                 kungfu::TypeCode::NOTYPE);
                auto gate = circuit_.NewGate(kungfu::OpCode(kungfu::OpCode::RETURN), 0,
                                             {stateCur, dependCur, constant,
                                              kungfu::Circuit::GetCircuitRoot(
                                                      kungfu::OpCode(kungfu::OpCode::RETURN_LIST))},
                                             kungfu::TypeCode::NOTYPE);
                gateToByteCode_[gate] = {bb.id, pcPrev};
                break;
            } else if (IsThrow(static_cast<EcmaOpcode>(bytecodeInfo.opcode))) {
                auto gate = circuit_.NewGate(kungfu::OpCode(kungfu::OpCode::JS_BYTECODE), numValueInputs,
                                             std::vector<kungfu::GateRef>(
                                                     2 + numValueInputs, kungfu::Circuit::NullGate()),
                                             kungfu::TypeCode::NOTYPE);
                circuit_.NewIn(gate, 0, stateCur);
                circuit_.NewIn(gate, 1, dependCur);

                if (!bb.catchs.empty()) {
                    auto bbNext = bb.catchs.at(0);
                    circuit_.NewIn(bbNext->stateStart, bbNext->cntStatePred, gate);
                    circuit_.NewIn(bbNext->dependStart, bbNext->cntStatePred + 1, gate);
                    bbNext->cntStatePred++;
                    bbNext->realPreds.push_back({bb.id, pcPrev, true});
                    ASSERT(bbNext->cntStatePred <= bbNext->numStatePred);
                } else {
                    circuit_.NewGate(kungfu::OpCode(kungfu::OpCode::THROW), 0,
                                     {stateCur, gate, gate,
                                      kungfu::Circuit::GetCircuitRoot(kungfu::OpCode(kungfu::OpCode::THROW_LIST))},
                                     kungfu::TypeCode::NOTYPE);
                }
                gateToByteCode_[gate] = {bb.id, pcPrev};
                break;
            } else if (IsMov(static_cast<EcmaOpcode>(bytecodeInfo.opcode))) {
                if (pcPrev == bb.end) {
                    auto bbNext = &graph.at(bb.id + 1);
                    circuit_.NewIn(bbNext->stateStart, bbNext->cntStatePred, stateCur);
                    circuit_.NewIn(bbNext->dependStart, bbNext->cntStatePred + 1, dependCur);
                    bbNext->cntStatePred++;
                    bbNext->realPreds.push_back({bb.id, pcPrev, false});
                    ASSERT(bbNext->cntStatePred <= bbNext->numStatePred);
                }
            } else {
                abort();
            }
        }
    }
    // set all value inputs
    for (auto &bb: graph) {
        if (bb.isDead) {
            continue;
        }
        ASSERT(bb.cntStatePred == bb.numStatePred);
    }
    for (auto &bb: graph) {
        if (bb.isDead) {
            continue;
        }
        bb.phiAcc = (bb.numStatePred > 1) || (!bb.trys.empty());
    }
#if ECMASCRIPT_ENABLE_TS_AOT_PRINT
    PrintByteCodeInfo(graph);
#endif
    for (const auto &[key, value]: gateToByteCode_) {
        byteCodeToGate_[value.second] = key;
    }
    for (auto gate: circuit_.GetAllGates()) {
        auto numInsArray = circuit_.GetOpCode(gate).GetOpCodeNumInsArray(circuit_.GetBitField(gate));
        auto it = gateToByteCode_.find(gate);
        if (it == gateToByteCode_.end()) {
            continue;
        }
        const auto &[id, pc] = it->second;
        auto bytecodeInfo = GetByteCodeInfo(pc);
        [[maybe_unused]] size_t numValueInputs = (bytecodeInfo.accIn ? 1 : 0) + bytecodeInfo.vregIn.size();
        [[maybe_unused]] size_t numValueOutputs = (bytecodeInfo.accOut ? 1 : 0) + bytecodeInfo.vregOut.size();
        ASSERT(numValueInputs == numInsArray[2]);
        ASSERT(numValueOutputs <= 1);
        std::function<kungfu::GateRef(size_t, const uint8_t *, uint16_t, bool)> defSiteOfReg =
                [&](size_t bbId, const uint8_t *end, uint16_t reg, bool acc) -> kungfu::GateRef {
                    auto ans = kungfu::Circuit::NullGate();
                    auto &bb = graph.at(bbId);
                    std::vector<uint8_t *> instList;
                    {
                        auto pcIter = bb.start;
                        while (pcIter <= end) {
                            instList.push_back(pcIter);
                            auto curInfo = GetByteCodeInfo(pcIter);
                            pcIter += curInfo.offset;
                        }
                    }
                    std::reverse(instList.begin(), instList.end());
                    for (auto pcIter: instList) {
                        auto curInfo = GetByteCodeInfo(pcIter);
                        if (acc) {
                            if (curInfo.accOut) {
                                if (IsMov(static_cast<EcmaOpcode>(curInfo.opcode))) {
                                    acc = curInfo.accIn;
                                    if (!curInfo.vregIn.empty()) {
                                        ASSERT(!acc);
                                        ASSERT(curInfo.vregIn.size() == 1);
                                        reg = curInfo.vregIn.at(0);
                                    }
                                } else {
                                    ans = byteCodeToGate_.at(pcIter);
                                    break;
                                }
                            }
                        } else {
                            if (!curInfo.vregOut.empty() && curInfo.vregOut.at(0) == reg) {
                                if (IsMov(static_cast<EcmaOpcode>(curInfo.opcode))) {
                                    acc = curInfo.accIn;
                                    if (!curInfo.vregIn.empty()) {
                                        ASSERT(!acc);
                                        ASSERT(curInfo.vregIn.size() == 1);
                                        reg = curInfo.vregIn.at(0);
                                    }
                                } else {
                                    ans = byteCodeToGate_.at(pcIter);
                                    break;
                                }
                            }
                        }
                    }
                    if (ans == kungfu::Circuit::NullGate() && !acc && bb.phi.count(reg)) {
                        if (!bb.valueSelector.count(reg)) {
                            auto gate = circuit_.NewGate(kungfu::OpCode(kungfu::OpCode::VALUE_SELECTOR),
                                                         kungfu::ValueCode::INT64,
                                                         bb.numStatePred,
                                                         std::vector<kungfu::GateRef>(
                                                                 1 + bb.numStatePred, kungfu::Circuit::NullGate()),
                                                         kungfu::TypeCode::NOTYPE);
                            bb.valueSelector[reg] = gate;
                            circuit_.NewIn(gate, 0, bb.stateStart);
                            for (size_t i = 0; i < bb.numStatePred; ++i) {
                                auto &[predId, predPc, isException] = bb.realPreds.at(i);
                                circuit_.NewIn(gate, i + 1, defSiteOfReg(predId, predPc, reg, acc));
                            }
                        }
                        ans = bb.valueSelector.at(reg);
                    }
                    if (ans == kungfu::Circuit::NullGate() && acc && bb.phiAcc) {
                        if (bb.valueSelectorAcc == kungfu::Circuit::NullGate()) {
                            auto gate = circuit_.NewGate(kungfu::OpCode(kungfu::OpCode::VALUE_SELECTOR),
                                                         kungfu::ValueCode::INT64,
                                                         bb.numStatePred,
                                                         std::vector<kungfu::GateRef>(
                                                                 1 + bb.numStatePred, kungfu::Circuit::NullGate()),
                                                         kungfu::TypeCode::NOTYPE);
                            bb.valueSelectorAcc = gate;
                            circuit_.NewIn(gate, 0, bb.stateStart);
                            bool hasException = false;
                            bool hasNonException = false;
                            for (size_t i = 0; i < bb.numStatePred; ++i) {
                                auto &[predId, predPc, isException] = bb.realPreds.at(i);
                                if (isException) {
                                    hasException = true;
                                } else {
                                    hasNonException = true;
                                }
                                if (isException) {
                                    auto ifExceptionGate = circuit_.GetIn(bb.stateStart, i);
                                    ASSERT(circuit_.GetOpCode(ifExceptionGate) == kungfu::OpCode::IF_EXCEPTION);
                                    circuit_.NewIn(gate, i + 1, circuit_.GetIn(ifExceptionGate, 0));
                                } else {
                                    circuit_.NewIn(gate, i + 1, defSiteOfReg(predId, predPc, reg, acc));
                                }
                            }
                            // catch block should have only exception entries
                            // normal block should have only normal entries
                            ASSERT(!hasException || !hasNonException);
                        }
                        ans = bb.valueSelectorAcc;
                    }
                    if (ans == kungfu::Circuit::NullGate() && bbId == 0) {
                        ASSERT(!acc && reg >= offsetArgs && reg < offsetArgs + argGates.size());
                        return argGates.at(reg - offsetArgs);
                    }
                    if (ans == kungfu::Circuit::NullGate()) {
                        return defSiteOfReg(bb.iDominator->id, bb.iDominator->end, reg, acc);
                    } else {
                        return ans;
                    }
                };
        for (size_t valueIdx = 0; valueIdx < numInsArray[2]; valueIdx++) {
            auto inIdx = valueIdx + numInsArray[0] + numInsArray[1];
            if (!circuit_.IsInGateNull(gate, inIdx)) {
                continue;
            }
            if (valueIdx < bytecodeInfo.vregIn.size()) {
                circuit_.NewIn(gate, inIdx, defSiteOfReg(id, pc - 1, bytecodeInfo.vregIn.at(valueIdx), false));
            } else {
                circuit_.NewIn(gate, inIdx, defSiteOfReg(id, pc - 1, 0, true));
            }
        }
    }
#if ECMASCRIPT_ENABLE_TS_AOT_PRINT
    circuit_.PrintAllGates(*this);
#endif
}

void ByteCodeCircuitBuilder::Sort(std::vector<std::tuple<uint8_t *, SplitPoint, std::vector<uint8_t *>>> &markOffset)
{
    std::sort(markOffset.begin(), markOffset.end(),
              [](std::tuple<uint8_t *, SplitPoint, std::vector<uint8_t *>> left,
                 std::tuple<uint8_t *, SplitPoint, std::vector<uint8_t *>> right) {
                  if (std::get<0>(left) != std::get<0>(right)) {
                      return std::get<0>(left) < std::get<0>(right);
                  } else {
                      return std::get<1>(left) < std::get<1>(right);
                  }
              });
}

void ByteCodeCircuitBuilder::PrintCollectBlockInfo(
    std::vector<std::tuple<uint8_t *, SplitPoint, std::vector<uint8_t *>>> &bytecodeBlockInfo)
{
    for (auto iter = bytecodeBlockInfo.begin(); iter != bytecodeBlockInfo.end(); iter++) {
        std::cout << "offset: " << static_cast<const void *>(std::get<0>(*iter)) << " position: " <<
                  static_cast<int32_t>(std::get<1>(*iter)) << " successor are: ";
        auto vec = std::get<2>(*iter);
        for (size_t i = 0; i < vec.size(); i++) {
            std::cout << static_cast<const void *>(vec[i]) << " , ";
        }
        std::cout << "" << std::endl;
    }
    std::cout << "=============================================================================" << std::endl;
}

void ByteCodeCircuitBuilder::PrintGraph(std::vector<ByteCodeBasicBlock> &blocks)
{
    for (size_t i = 0; i < blocks.size(); i++) {
        if (blocks[i].isDead) {
            std::cout << "BB_" << blocks[i].id << ":                               ;predsId= ";
            for (size_t k = 0; k < blocks[i].preds.size(); ++k) {
                std::cout << blocks[i].preds[k]->id << ", ";
            }
            std::cout << "" << std::endl;
            std::cout << "curStartPc: " << static_cast<const void *>(blocks[i].start) <<
                      " curEndPc: " << static_cast<const void *>(blocks[i].end) << std::endl;

            for (size_t j = 0; j < blocks[i].preds.size(); j++) {
                std::cout << "predsStartPc: " << static_cast<const void *>(blocks[i].preds[j]->start) <<
                          " predsEndPc: " << static_cast<const void *>(blocks[i].preds[j]->end) << std::endl;
            }

            for (size_t j = 0; j < blocks[i].succs.size(); j++) {
                std::cout << "succesStartPc: " << static_cast<const void *>(blocks[i].succs[j]->start) <<
                          " succesEndPc: " << static_cast<const void *>(blocks[i].succs[j]->end) << std::endl;
            }
            continue;
        }
        std::cout << "BB_" << blocks[i].id << ":                               ;predsId= ";
        for (size_t k = 0; k < blocks[i].preds.size(); ++k) {
            std::cout << blocks[i].preds[k]->id << ", ";
        }
        std::cout << "" << std::endl;
        std::cout << "curStartPc: " << static_cast<const void *>(blocks[i].start) <<
                  " curEndPc: " << static_cast<const void *>(blocks[i].end) << std::endl;

        for (size_t j = 0; j < blocks[i].preds.size(); j++) {
            std::cout << "predsStartPc: " << static_cast<const void *>(blocks[i].preds[j]->start) <<
                      " predsEndPc: " << static_cast<const void *>(blocks[i].preds[j]->end) << std::endl;
        }

        for (size_t j = 0; j < blocks[i].succs.size(); j++) {
            std::cout << "succesStartPc: " << static_cast<const void *>(blocks[i].succs[j]->start) <<
                      " succesEndPc: " << static_cast<const void *>(blocks[i].succs[j]->end) << std::endl;
        }

        std::cout << "succesId: ";
        for (size_t j = 0; j < blocks[i].succs.size(); j++) {
            std::cout << blocks[i].succs[j]->id << ", ";
        }
        std::cout << "" << std::endl;


        for (size_t j = 0; j < blocks[i].catchs.size(); j++) {
            std::cout << "catchStartPc: " << static_cast<const void *>(blocks[i].catchs[j]->start) <<
                      " catchEndPc: " << static_cast<const void *>(blocks[i].catchs[j]->end) << std::endl;
        }

        for (size_t j = 0; j < blocks[i].immDomBlocks.size(); j++) {
            std::cout << "dominate block id: " << blocks[i].immDomBlocks[j]->id << " startPc: " <<
                      static_cast<const void *>(blocks[i].immDomBlocks[j]->start) << " endPc: " <<
                      static_cast<const void *>(blocks[i].immDomBlocks[j]->end) << std::endl;
        }

        if (blocks[i].iDominator) {
            std::cout << "current block " << blocks[i].id <<
                      " immediate dominator is " << blocks[i].iDominator->id << std::endl;
        }
        std::cout << std::endl;

        std::cout << "current block " << blocks[i].id << " dominace Frontiers: ";
        for (const auto &frontier: blocks[i].domFrontiers) {
            std::cout << frontier->id << " , ";
        }
        std::cout << std::endl;

        std::cout << "current block " << blocks[i].id << " phi variable: ";
        for (auto variable: blocks[i].phi) {
            std::cout << variable << " , ";
        }
        std::cout << std::endl;
    }
}

void ByteCodeCircuitBuilder::PrintByteCodeInfo(std::vector<ByteCodeBasicBlock> &graph)
{
    for (auto &bb: graph) {
        if (bb.isDead) {
            continue;
        }
        auto pc = bb.start;
        std::cout << "BB_" << bb.id << ": " << std::endl;
        while (pc <= bb.end) {
            auto curInfo = GetByteCodeInfo(pc);
            std::cout << "Inst_" << static_cast<int>(curInfo.opcode) << ": ";
            std::cout << "In=[";
            if (curInfo.accIn) {
                std::cout << "acc" << ",";
            }
            for (const auto &in: curInfo.vregIn) {
                std::cout << in << ",";
            }
            std::cout << "] Out=[";
            if (curInfo.accOut) {
                std::cout << "acc" << ",";
            }
            for (const auto &out: curInfo.vregOut) {
                std::cout << out << ",";
            }
            std::cout << "]";
            std::cout << std::endl;
            pc += curInfo.offset;
        }
    }
}

void ByteCodeCircuitBuilder::PrintBBInfo(std::vector<ByteCodeBasicBlock> &graph)
{
    for (auto &bb: graph) {
        if (bb.isDead) {
            continue;
        }
        std::cout << "------------------------" << std::endl;
        std::cout << "block: " << bb.id << std::endl;
        std::cout << "preds: ";
        for (auto pred: bb.preds) {
            std::cout << pred->id << " , ";
        }
        std::cout << std::endl;
        std::cout << "succs: ";
        for (auto succ: bb.succs) {
            std::cout << succ->id << " , ";
        }
        std::cout << std::endl;
        std::cout << "catchs: ";
        for (auto catchBlock: bb.catchs) {
            std::cout << catchBlock->id << " , ";
        }
        std::cout << std::endl;
        std::cout << "trys: ";
        for (auto tryBlock: bb.trys) {
            std::cout << tryBlock->id << " , ";
        }
        std::cout << std::endl;
    }
}
}  // namespace panda::ecmascript