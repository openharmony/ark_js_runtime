/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "ecmascript/snapshot/mem/snapshot.h"

#include <cerrno>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/jobs/micro_job_queue.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/ts_types/ts_loader.h"
#include "libpandabase/mem/mem.h"

namespace panda::ecmascript {
void Snapshot::Serialize(TaggedObject *objectHeader, const panda_file::File *pf, const CString &fileName)
{
    std::pair<bool, CString> filePath = VerifyFilePath(fileName, true);
    if (!filePath.first) {
        LOG_ECMA(FATAL) << "snapshot file path error";
    }
    std::fstream writer(fileName.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
    if (!writer.good()) {
        writer.close();
        LOG_ECMA(FATAL) << "snapshot open file failed";
    }

    SnapshotProcessor processor(vm_);
    processor.Initialize();
    vm_->GetSnapshotEnv()->Initialize();

    std::unordered_map<uint64_t, std::pair<uint64_t, EncodeBit>> data;
    CQueue<TaggedObject *> objectQueue;

    if (objectHeader->GetClass()->GetObjectType() == JSType::PROGRAM) {
        processor.SetProgramSerializeStart();
    }

    processor.EncodeTaggedObject(objectHeader, &objectQueue, &data);
    size_t rootObjSize = objectQueue.size();
    processor.ProcessObjectQueue(&objectQueue, &data);
    WriteToFile(writer, pf, rootObjSize, processor);
    vm_->GetSnapshotEnv()->ClearEnvMap();
}

void Snapshot::Serialize(uintptr_t startAddr, size_t size, const CString &fileName)
{
    std::pair<bool, CString> filePath = VerifyFilePath(fileName, true);
    if (!filePath.first) {
        LOG_ECMA(FATAL) << "snapshot file path error";
    }
    std::fstream writer(fileName.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
    if (!writer.good()) {
        writer.close();
        LOG_ECMA(FATAL) << "snapshot open file failed";
    }

    SnapshotProcessor processor(vm_);
    processor.Initialize();
    vm_->GetSnapshotEnv()->Initialize();

    std::unordered_map<uint64_t, std::pair<uint64_t, EncodeBit>> data;
    CQueue<TaggedObject *> objectQueue;

    ObjectSlot start(startAddr);
    ObjectSlot end(startAddr + size * sizeof(JSTaggedType));
    processor.EncodeTaggedObjectRange(start, end, &objectQueue, &data);

    processor.ProcessObjectQueue(&objectQueue, &data);
    WriteToFile(writer, nullptr, size, processor);
    vm_->GetSnapshotEnv()->ClearEnvMap();
}

void Snapshot::SerializeBuiltins(const CString &fileName)
{
    std::pair<bool, CString> filePath = VerifyFilePath(fileName, true);
    if (!filePath.first) {
        LOG_ECMA(FATAL) << "snapshot file path error";
    }
    // if builtins.snapshot file has exist, return directly
    if (!filePath.second.empty()) {
        return;
    }
    std::fstream write(fileName.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
    if (!write.good()) {
        write.close();
        LOG_ECMA(FATAL) << "snapshot open file failed";
    }

    SnapshotProcessor processor(vm_);
    processor.Initialize();
    processor.SetBuiltinsSerializeStart();

    std::unordered_map<uint64_t, std::pair<uint64_t, EncodeBit>> data;
    CQueue<TaggedObject *> objectQueue;

    auto globalEnvHandle = vm_->GetGlobalEnv();
    auto constant = const_cast<GlobalEnvConstants *>(vm_->GetJSThread()->GlobalConstants());
    constant->VisitRangeSlot([&objectQueue, &data, &processor]([[maybe_unused]]Root type,
                                                               ObjectSlot start, ObjectSlot end) {
        processor.EncodeTaggedObjectRange(start, end, &objectQueue, &data);
    });
    processor.EncodeTaggedObject(*globalEnvHandle, &objectQueue, &data);
    size_t rootObjSize = objectQueue.size();
    processor.ProcessObjectQueue(&objectQueue, &data);
    WriteToFile(write, nullptr, rootObjSize, processor);
}

const JSPandaFile *Snapshot::Deserialize(SnapshotType type, const CString &snapshotFile, bool isBuiltins)
{
    std::pair<bool, CString> filePath = VerifyFilePath(snapshotFile, false);
    if (!filePath.first) {
        LOG_ECMA(FATAL) << "snapshot file path error";
        UNREACHABLE();
    }
    int fd = open(filePath.second.c_str(), O_CLOEXEC);  // NOLINT(cppcoreguidelines-pro-type-vararg)
    if (UNLIKELY(fd == -1)) {
        LOG_ECMA(FATAL) << "open file failed";
        UNREACHABLE();
    }
    int32_t file_size = lseek(fd, 0, SEEK_END);
    if (file_size == -1) {
        LOG_ECMA(FATAL) << "lseek failed";
        UNREACHABLE();
    }

    SnapshotProcessor processor(vm_);
    if (isBuiltins) {
        processor.SetBuiltinsDeserializeStart();
        processor.GeneratedNativeMethod();
    }
    auto readFile = ToUintPtr(mmap(nullptr, file_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0));
    auto hdr = *ToNativePtr<const Header>(readFile);
    uintptr_t oldSpaceBegin = readFile + sizeof(Header);
    processor.DeserializeObjectExcludeString(oldSpaceBegin, hdr.oldSpaceObjSize, hdr.nonMovableObjSize,
                                             hdr.machineCodeObjSize, hdr.snapshotObjSize);
    uintptr_t stringBegin =
        oldSpaceBegin + hdr.oldSpaceObjSize + hdr.nonMovableObjSize + hdr.machineCodeObjSize + hdr.snapshotObjSize;
    uintptr_t stringEnd = stringBegin + hdr.stringSize;
    processor.DeserializeString(stringBegin, stringEnd);

    if (type == SnapshotType::TS_LOADER) {
        auto stringVector = processor.GetStringVector();
        for (uint32_t i = 0; i < hdr.rootObjectSize; ++i) {
            JSTaggedValue result(reinterpret_cast<EcmaString *>(stringVector[i]));
            vm_->GetTSLoader()->AddConstString(result);
        }
    }
    munmap(ToNativePtr<void>(readFile), hdr.pandaFileBegin);
    const JSPandaFile *jsPandaFile = nullptr;
    if (static_cast<uint32_t>(file_size) > hdr.pandaFileBegin) {
        uintptr_t panda_file_mem = readFile + hdr.pandaFileBegin;
        auto pf = panda_file::File::OpenFromMemory(os::mem::ConstBytePtr(ToNativePtr<std::byte>(panda_file_mem),
            static_cast<uint32_t>(file_size) - hdr.pandaFileBegin, os::mem::MmapDeleter));
        jsPandaFile = JSPandaFileManager::GetInstance()->NewJSPandaFile(pf.release(), "");
    }
    close(fd);
    // relocate object field
    processor.Relocate(type, jsPandaFile, hdr.rootObjectSize);
    return jsPandaFile;
}

size_t Snapshot::AlignUpPageSize(size_t spaceSize)
{
    if (spaceSize % Constants::PAGE_SIZE_ALIGN_UP == 0) {
        return spaceSize;
    }
    return Constants::PAGE_SIZE_ALIGN_UP * (spaceSize / Constants::PAGE_SIZE_ALIGN_UP + 1);
}

std::pair<bool, CString> Snapshot::VerifyFilePath(const CString &filePath, bool toGenerate)
{
    if (filePath.size() > PATH_MAX) {
        return std::make_pair(false, "");
    }
    CVector<char> resolvedPath(PATH_MAX);
    auto result = realpath(filePath.c_str(), resolvedPath.data());
    if (toGenerate && errno == ENOENT) {
        return std::make_pair(true, "");
    }
    if (!result) {
        return std::make_pair(false, "");
    }
    return std::make_pair(true, CString(resolvedPath.data()));
}

void Snapshot::WriteToFile(std::fstream &writer, const panda_file::File *pf, size_t size, SnapshotProcessor &processor)
{
    uint32_t totalStringSize = 0U;
    CVector<uintptr_t> stringVector = processor.GetStringVector();
    for (size_t i = 0; i < stringVector.size(); ++i) {
        auto str = reinterpret_cast<EcmaString *>(stringVector[i]);
        size_t objectSize = AlignUp(str->ObjectSize(), static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
        totalStringSize += objectSize;
    }

    std::vector<uint32_t> objSizeVector = processor.StatisticsObjectSize();
    size_t totalObjSize = totalStringSize;
    for (uint32_t objSize : objSizeVector) {
        totalObjSize += objSize;
    }
    uint32_t pandaFileBegin = RoundUp(totalObjSize + sizeof(Header), Constants::PAGE_SIZE_ALIGN_UP);
    Header hdr {objSizeVector[0], objSizeVector[1], objSizeVector[2], objSizeVector[3], // 0,1,2,3: index of element
                totalStringSize, pandaFileBegin, static_cast<uint32_t>(size)};
    writer.write(reinterpret_cast<char *>(&hdr), sizeof(hdr));

    processor.WriteObjectToFile(writer);

    for (size_t i = 0; i < stringVector.size(); ++i) {
        auto str = reinterpret_cast<EcmaString *>(stringVector[i]);
        size_t strSize = AlignUp(str->ObjectSize(), static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
        writer.write(reinterpret_cast<char *>(str), strSize);
        writer.flush();
    }
    ASSERT(static_cast<size_t>(writer.tellp()) == totalObjSize + sizeof(Header));
    if (pf) {
        writer.seekp(pandaFileBegin);
        writer.write(reinterpret_cast<const char *>(pf->GetBase()), pf->GetHeader()->file_size);
    }
    writer.close();
}
}  // namespace panda::ecmascript
