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
constexpr uint32_t PANDA_FILE_ALIGNMENT = 4096;

void SnapShot::MakeSnapShot(TaggedObject *objectHeader, const panda_file::File *pf, const CString &fileName)
{
    std::fstream write;
    if (!VerifySnapShotFile(write, fileName)) {
        return;
    }

    SnapShotSerialize serialize(vm_);

    std::unordered_map<uint64_t, std::pair<uint64_t, EncodeBit>> data;
    CQueue<TaggedObject *> objectQueue;

    if (objectHeader->GetClass()->GetObjectType() == JSType::PROGRAM) {
        serialize.SetProgramSerializeStart();
    }

    serialize.EncodeTaggedObject(objectHeader, &objectQueue, &data);

    size_t rootObjSize = objectQueue.size();

    while (!objectQueue.empty()) {
        auto taggedObject = objectQueue.front();
        if (taggedObject == nullptr) {
            break;
        }
        objectQueue.pop();

        serialize.Serialize(taggedObject, &objectQueue, &data);
    }

    vm_->GetHeap()->GetSnapShotSpace()->Stop();

    WriteToFile(write, pf, rootObjSize);
}

void SnapShot::MakeSnapShot(uintptr_t startAddr, size_t size, const CString &fileName)
{
    std::fstream write;
    if (!VerifySnapShotFile(write, fileName)) {
        return;
    }

    SnapShotSerialize serialize(vm_);

    std::unordered_map<uint64_t, std::pair<uint64_t, EncodeBit>> data;
    CQueue<TaggedObject *> objectQueue;

    ObjectSlot start(startAddr);
    ObjectSlot end(startAddr + size * sizeof(JSTaggedType));

    serialize.EncodeTaggedObjectRange(start, end, &objectQueue, &data);
    size_t rootObjSize = objectQueue.size();
    while (!objectQueue.empty()) {
        auto taggedObject = objectQueue.front();
        if (taggedObject == nullptr) {
            break;
        }
        objectQueue.pop();
        serialize.Serialize(taggedObject, &objectQueue, &data);
    }

    vm_->GetHeap()->GetSnapShotSpace()->Stop();

    WriteToFile(write, nullptr, rootObjSize);
}

const JSPandaFile *SnapShot::SnapShotDeserialize(SnapShotType type, const CString &snapshotFile)
{
    SnapShotSerialize serialize(vm_);
    std::pair<bool, CString> filePath = VerifyFilePath(snapshotFile);
    if (!filePath.first) {
        LOG(ERROR, RUNTIME) << "snapshot file path error";
        return nullptr;
    }

    int fd = open(filePath.second.c_str(), O_CLOEXEC);  // NOLINT(cppcoreguidelines-pro-type-vararg)
    if (UNLIKELY(fd == -1)) {
        LOG_ECMA(FATAL) << "open file failed";
        UNREACHABLE();
    }
    size_t file_size = lseek(fd, 0, SEEK_END);
    auto readFile = ToUintPtr(mmap(nullptr, file_size, PROT_READ, MAP_PRIVATE, fd, 0));
    auto hdr = *ToNativePtr<const Header>(readFile);
    size_t defaultSnapshotSpaceCapacity = vm_->GetJSOptions().DefaultSnapshotSpaceCapacity();
    if (hdr.snapshot_size % defaultSnapshotSpaceCapacity != 0) {
        LOG_ECMA(FATAL) << "Invalid snapshot file";
        UNREACHABLE();
    }
    SnapShotSpace *space = vm_->GetHeap()->GetSnapShotSpace();

    uintptr_t snapshot_begin = readFile + sizeof(Header);
    for (size_t i = 0; i < hdr.snapshot_size / defaultSnapshotSpaceCapacity; i++) {
        Region *region = const_cast<HeapRegionAllocator *>(vm_->GetHeap()->GetHeapRegionAllocator())
                            ->AllocateAlignedRegion(space, defaultSnapshotSpaceCapacity);
        auto fileRegion = ToNativePtr<Region>(snapshot_begin + i * defaultSnapshotSpaceCapacity);

        uint64_t base = region->allocateBase_;
        RangeBitmap *markBitmap = region->markBitmap_;
        if (defaultSnapshotSpaceCapacity == 0) {
            LOG_ECMA_MEM(FATAL) << "defaultSnapshotSpaceCapacity must have a size bigger than 0";
            UNREACHABLE();
        }
        uint64_t begin = (fileRegion->begin_) % defaultSnapshotSpaceCapacity;
        uint64_t waterMark = (fileRegion->highWaterMark_) % defaultSnapshotSpaceCapacity;

        if (memcpy_s(region, defaultSnapshotSpaceCapacity, fileRegion, defaultSnapshotSpaceCapacity) != EOK) {
            LOG_ECMA(FATAL) << "memcpy_s failed";
            UNREACHABLE();
        }

        // allocate_base_
        region->allocateBase_ = base;
        // begin_
        region->begin_ = ToUintPtr(region) + begin;
        // end_
        region->end_ = ToUintPtr(region) + defaultSnapshotSpaceCapacity;
        // high_water_mark_
        region->highWaterMark_ = ToUintPtr(region) + waterMark;
        // prev_
        region->prev_ = nullptr;
        // next_
        region->next_ = nullptr;
        // mark_bitmap_
        region->markBitmap_ = markBitmap;
        // cross_region_set_
        region->crossRegionSet_ = nullptr;
        // old_to_new_set_
        region->oldToNewSet_ = nullptr;
        // space_
        region->space_ = space;
        // heap_
        auto heap = const_cast<Heap *>(vm_->GetHeap());
        region->heap_ = heap;
        // nativePoniterAllocator_
        region->nativeAreaAllocator_ = heap->GetNativeAreaAllocator();

        space->AddRegion(region);
    }
    munmap(ToNativePtr<void>(readFile), hdr.panda_file_begin);
    const JSPandaFile *jsPandaFile = nullptr;
    if (file_size > hdr.panda_file_begin) {
        uintptr_t panda_file_mem = readFile + hdr.panda_file_begin;
        auto pf = panda_file::File::OpenFromMemory(os::mem::ConstBytePtr(ToNativePtr<std::byte>(panda_file_mem),
                                                                         file_size - hdr.panda_file_begin,
                                                                         os::mem::MmapDeleter));
        jsPandaFile = JSPandaFileManager::GetInstance()->NewJSPandaFile(pf.release(), "");
    }
    close(fd);
    // relocate object field
    serialize.Relocate(type, jsPandaFile, hdr.root_object_size);
    return jsPandaFile;
}

size_t SnapShot::AlignUpPageSize(size_t spaceSize)
{
    if (spaceSize % Constants::PAGE_SIZE_ALIGN_UP == 0) {
        return spaceSize;
    }
    return Constants::PAGE_SIZE_ALIGN_UP * (spaceSize / Constants::PAGE_SIZE_ALIGN_UP + 1);
}

std::pair<bool, CString> SnapShot::VerifyFilePath(const CString &filePath)
{
    if (filePath.size() > PATH_MAX) {
        return std::make_pair(false, "");
    }
    CVector<char> resolvedPath(PATH_MAX);
    auto result = realpath(filePath.c_str(), resolvedPath.data());
    if (result == resolvedPath.data() || errno == ENOENT) {
        return std::make_pair(true, CString(resolvedPath.data()));
    }
    return std::make_pair(false, "");
}

bool SnapShot::VerifySnapShotFile(std::fstream &write, const CString &fileName)
{
    std::pair<bool, CString> filePath = VerifyFilePath(fileName);
    if (!filePath.first) {
        LOG(ERROR, RUNTIME) << "snapshot file path error";
        return false;
    }
    write.open(filePath.second.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
    if (!write.good()) {
        LOG(DEBUG, RUNTIME) << "snapshot open file failed";
        return false;
    }
    return true;
}

void SnapShot::WriteToFile(std::fstream &write, const panda_file::File *pf, size_t size)
{
    SnapShotSpace *space = vm_->GetHeap()->GetSnapShotSpace();
    size_t defaultSnapshotSpaceCapacity = vm_->GetJSOptions().DefaultSnapshotSpaceCapacity();
    uint32_t snapshot_size = space->GetRegionCount() * defaultSnapshotSpaceCapacity;
    uint32_t panda_file_begin = RoundUp(snapshot_size + sizeof(Header), PANDA_FILE_ALIGNMENT);
    Header hdr {snapshot_size, panda_file_begin, size};
    write.write(reinterpret_cast<char *>(&hdr), sizeof(hdr));

    space->EnumerateRegions([&write, &defaultSnapshotSpaceCapacity](Region *current) {
        write.write(reinterpret_cast<char *>(current), defaultSnapshotSpaceCapacity);
        write.flush();
    });
    space->ReclaimRegions();
    ASSERT(static_cast<size_t>(write.tellp()) == snapshot_size + sizeof(Header));
    if (pf) {
        write.seekp(panda_file_begin);
        write.write(reinterpret_cast<const char *>(pf->GetBase()), pf->GetHeader()->file_size);
    }
    write.close();
}
}  // namespace panda::ecmascript
