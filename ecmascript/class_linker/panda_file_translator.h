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

#ifndef ECMASCRIPT_CLASS_LINKER_PANDA_FILE_TRANSLATOR_H
#define ECMASCRIPT_CLASS_LINKER_PANDA_FILE_TRANSLATOR_H

#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_function.h"
#include "libpandafile/bytecode_instruction.h"
#include "libpandafile/code_data_accessor-inl.h"
#include "libpandafile/file-inl.h"
#include "utils/bit_field.h"
#include "ecmascript/compiler/circuit.h"

namespace panda::ecmascript {
class JSThread;
class Program;

struct ByteCodeBasicBlock{
    size_t id;
    uint8_t *start;
    uint8_t *end;
    std::vector<ByteCodeBasicBlock *> preds{}; // List of predessesor blocks
    std::vector<ByteCodeBasicBlock *> succs{}; // List of successors blocks
    std::vector<ByteCodeBasicBlock *> trys{};
    std::vector<ByteCodeBasicBlock *> catchs{}; // List of catches blocks
    std::vector<ByteCodeBasicBlock *> immDomBlocks{}; // List of dominated blocks
    ByteCodeBasicBlock *iDominator{nullptr}; // Block that dominates the current block
    std::vector<ByteCodeBasicBlock *> domFrontiers{}; // List of dominace frontiers
    bool isDead{false};
    std::set<uint16_t> phi{}; // phi node
    bool phiAcc{false};
    size_t numStatePred{0};
    size_t cntStatePred{0};
    std::vector<std::tuple<size_t, uint8_t *, bool>> realPreds{};
    kungfu::GateRef stateStart = kungfu::Circuit::NullGate();
    kungfu::GateRef dependStart = kungfu::Circuit::NullGate();
    std::map<uint16_t, kungfu::GateRef> valueSelector{};
    kungfu::GateRef valueSelectorAcc = kungfu::Circuit::NullGate();

    bool operator <(const ByteCodeBasicBlock &target) const
    {
        return id < target.id;
    }
};

struct ByteCodeInfo {
    std::vector<uint16_t> vregIn{}; // read register
    std::vector<uint16_t> vregOut{}; // write register
    bool accIn{false}; // read acc
    bool accOut{false}; // write acc
    uint8_t opcode{-1};
    uint16_t offset{-1};
};

struct ByteCodeGraph{
    std::vector<ByteCodeBasicBlock> graph{};
    const JSMethod *method;
};

class PandaFileTranslator {
public:
    enum FixInstructionIndex : uint8_t { FIX_ONE = 1, FIX_TWO = 2, FIX_FOUR = 4 };

    explicit PandaFileTranslator(EcmaVM *vm);
    ~PandaFileTranslator() = default;
    NO_COPY_SEMANTIC(PandaFileTranslator);
    NO_MOVE_SEMANTIC(PandaFileTranslator);
    static JSHandle<Program> TranslatePandaFile(EcmaVM *vm, const panda_file::File &pf,
                                                const CString &methodName);
    JSHandle<JSFunction> DefineMethodInLiteral(JSThread *thread, uint32_t methodId, FunctionKind kind,
                                               uint16_t length) const;

private:
    enum class ConstPoolType : uint8_t {
        STRING,
        BASE_FUNCTION,
        NC_FUNCTION,
        GENERATOR_FUNCTION,
        ASYNC_FUNCTION,
        CLASS_FUNCTION,
        METHOD,
        ARRAY_LITERAL,
        OBJECT_LITERAL,
        CLASS_LITERAL,
    };

    class ConstPoolValue {
    public:
        ConstPoolValue(ConstPoolType type, uint32_t index)
            : value_(ConstPoolIndexField::Encode(index) | ConstPoolTypeField::Encode(type))
        {
        }

        explicit ConstPoolValue(uint64_t v) : value_(v) {}
        ~ConstPoolValue() = default;
        NO_COPY_SEMANTIC(ConstPoolValue);
        NO_MOVE_SEMANTIC(ConstPoolValue);

        inline uint64_t GetValue() const
        {
            return value_;
        }

        inline uint32_t GetConstpoolIndex() const
        {
            return ConstPoolIndexField::Get(value_);
        }

        inline ConstPoolType GetConstpoolType() const
        {
            return ConstPoolTypeField::Get(value_);
        }

    private:
        // NOLINTNEXTLINE(readability-magic-numbers)
        using ConstPoolIndexField = BitField<uint32_t, 0, 32>;  // 32: 32 bit
        // NOLINTNEXTLINE(readability-magic-numbers)
        using ConstPoolTypeField = BitField<ConstPoolType, 32, 4>;  // 32: offset, 4: 4bit

        uint64_t value_{0};
    };
    uint32_t GetOrInsertConstantPool(ConstPoolType type, uint32_t offset);
    const JSMethod *FindMethods(uint32_t offset) const;
    Program *GenerateProgram(const panda_file::File &pf);
    void TranslateClasses(const panda_file::File &pf, const CString &methodName);
    void TranslateBytecode(uint32_t insSz, const uint8_t *insArr, const panda_file::File &pf, const JSMethod *method);
    void FixInstructionId32(const BytecodeInstruction &inst, uint32_t index, uint32_t fixOrder = 0) const;
    void FixOpcode(uint8_t *pc) const;
    void UpdateICOffset(JSMethod *method, uint8_t *pc) const;
    void DefineClassInConstPool(const JSHandle<ConstantPool> &constpool) const;

    void SetMethods(Span<JSMethod> methods, const uint32_t numMethods)
    {
        methods_ = methods.data();
        numMethods_ = numMethods;
    }

    Span<JSMethod> GetMethods() const
    {
        return {methods_, numMethods_};
    }

    void CollectBytecodeBlockInfo(uint8_t* pc,
        std::vector<std::tuple<uint8_t *, int32_t, std::vector<uint8_t *>>> &bytecodeBlockInfo);

    std::map<std::pair<uint8_t *, uint8_t *>, std::vector<uint8_t *>> CollectTryCatchBlockInfo(
        const panda_file::File &file, const JSMethod *method, std::map<uint8_t *, uint8_t*> &byteCodeCurPrePc,
        std::vector<std::tuple<uint8_t *, int32_t, std::vector<uint8_t *>>> &bytecodeBlockInfo);

    void CompleteBytecodeBlockInfo(std::map<uint8_t *, uint8_t*> &byteCodeCurPrePc,
        std::vector<std::tuple<uint8_t *, int32_t, std::vector<uint8_t *>>> &bytecodeBlockInfo);

    void BuildBasicBlocks(const JSMethod *method,
                          std::map<std::pair<uint8_t *, uint8_t *>, std::vector<uint8_t *>> &exception,
                          std::vector<std::tuple<uint8_t *, int32_t, std::vector<uint8_t *>>> &bytecodeBlockInfo,
                          std::map<uint8_t *, uint8_t*> &byteCodeCurPrePc);

    void Sort(std::vector<std::tuple<uint8_t *, int32_t, std::vector<uint8_t *>>> &markOffset)
    {
        std::sort(markOffset.begin(), markOffset.end(),
                  [](std::tuple<uint8_t *, int32_t, std::vector<uint8_t *>> left,
                     std::tuple<uint8_t *, int32_t, std::vector<uint8_t *>> right) {
                      if (std::get<0>(left) != std::get<0>(right)) {
                          return std::get<0>(left) < std::get<0>(right);
                      } else {
                          return std::get<1>(left) < std::get<1>(right);
                      }
                  });
    }

    void PrintCollectBlockInfo(std::vector<std::tuple<uint8_t *, int32_t, std::vector<uint8_t *>>> &bytecodeBlockInfo)
    {
        for(auto iter = bytecodeBlockInfo.begin(); iter != bytecodeBlockInfo.end(); iter++) {
            std::cout << "offset: " << static_cast<const void *>(std::get<0>(*iter)) << " position: " <<
                      std::get<1>(*iter) << " successor are: ";
            auto vec = std::get<2>(*iter);
            for(size_t i = 0; i < vec.size(); i++) {
                std::cout<< static_cast<const void *>(vec[i]) << " , ";
            }
            std::cout<<""<<std::endl;
        }
        std::cout << "=============================================================================" << std::endl;
    }

    void PrintGraph(std::vector<ByteCodeBasicBlock> &blocks)
    {
        for (size_t i = 0; i < blocks.size(); i++) {
            if (blocks[i].isDead) {
                std::cout<< "BB_" << blocks[i].id << ":                               ;predsId= ";
                for (size_t k = 0; k < blocks[i].preds.size(); ++k) {
                    std::cout << blocks[i].preds[k]->id << ", ";
                }
                std::cout << "" << std::endl;
                std::cout<< "curStartPc: " << static_cast<const void *>(blocks[i].start) <<
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
            std::cout<< "BB_" << blocks[i].id << ":                               ;predsId= ";
            for (size_t k = 0; k < blocks[i].preds.size(); ++k) {
                std::cout << blocks[i].preds[k]->id << ", ";
            }
            std::cout << "" << std::endl;
            std::cout<< "curStartPc: " << static_cast<const void *>(blocks[i].start) <<
                     " curEndPc: " << static_cast<const void *>(blocks[i].end) << std::endl;

            for (size_t j = 0; j < blocks[i].preds.size(); j++) {
                std::cout << "predsStartPc: " << static_cast<const void *>(blocks[i].preds[j]->start) <<
                          " predsEndPc: " << static_cast<const void *>(blocks[i].preds[j]->end) << std::endl;
            }

            for (size_t j = 0; j < blocks[i].succs.size(); j++) {
                std::cout << "succesStartPc: " << static_cast<const void *>(blocks[i].succs[j]->start) <<
                          " succesEndPc: " << static_cast<const void *>(blocks[i].succs[j]->end) << std::endl;
            }

            std::cout<< "succesId: ";
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
            for (const auto &frontier : blocks[i].domFrontiers) {
                std::cout << frontier->id << " , ";
            }
            std::cout << std::endl;

            std::cout << "current block " << blocks[i].id << " phi variable: ";
            for (auto variable : blocks[i].phi) {
                std::cout << variable << " , " ;
            }
            std::cout << std::endl;
        }
    }

    // void ComputeDominatorTree(std::vector<ByteCodeBasicBlock> &graph);
    // void BuildImmediateDominator(std::vector<size_t> &immDom, std::vector<ByteCodeBasicBlock> &graph);
    // void ComputeDomFrontiers(std::vector<size_t> &immDom, std::vector<ByteCodeBasicBlock> &graph);
    // void GetByteCodeInfo(std::vector<ByteCodeBasicBlock> &graph);
    // void DeadCodeRemove(const std::map<size_t, size_t> &dfsTimestamp, std::vector<ByteCodeBasicBlock> &graph);
    // void InsertPhi(std::vector<ByteCodeBasicBlock> &graph);
    void ComputeDominatorTree(ByteCodeGraph &byteCodeGraph);
    void BuildImmediateDominator(std::vector<size_t> &immDom, ByteCodeGraph &byteCodeGraph);
    void ComputeDomFrontiers(std::vector<size_t> &immDom, ByteCodeGraph &byteCodeGraph);
    void GetByteCodeInfo(ByteCodeGraph &byteCodeGraph);
    ByteCodeInfo GetByteCodeInfo(uint8_t *pc);
    void DeadCodeRemove(const std::map<size_t, size_t> &dfsTimestamp, ByteCodeGraph &byteCodeGraph);
    void InsertPhi(ByteCodeGraph &byteCodeGraph);
    static bool IsJump(EcmaOpcode opcode);
    static bool IsCondJump(EcmaOpcode opcode);
    static bool IsMov(EcmaOpcode opcode);
    static bool IsReturn(EcmaOpcode opcode);
    static bool IsGeneral(EcmaOpcode opcode);
    // void BuildCircuit(std::vector<ByteCodeBasicBlock> &byteCodeGraph);
    void BuildCircuit(ByteCodeGraph &byteCodeGraph);

    EcmaVM *ecmaVm_;
    ObjectFactory *factory_;
    JSThread *thread_;
    uint32_t constpoolIndex_{0};
    uint32_t numMethods_{0};
    uint32_t mainMethodIndex_{0};
    JSMethod *methods_ {nullptr};

    std::unordered_map<uint32_t, uint64_t> constpoolMap_;
    std::set<const uint8_t *> translated_code_;
    std::map<const JSMethod *, std::vector<ByteCodeBasicBlock>> methodsGraphs_;
    std::vector<ByteCodeGraph> graphs_;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_CLASS_LINKER_PANDA_FILE_TRANSLATOR_H
