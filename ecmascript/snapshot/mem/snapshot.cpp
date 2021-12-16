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

#include "ecmascript/class_linker/program_object.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/jobs/micro_job_queue.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/mem/mem_manager.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/snapshot/mem/snapshot_serialize.h"
#include "libpandabase/mem/mem.h"

namespace panda::ecmascript {
constexpr uint32_t PANDA_FILE_ALIGNMENT = 4096;

void SnapShot::MakeSnapShotProgramObject(Program *program, const panda_file::File *pf, const CString &fileName)
{
    std::fstream write;
    std::pair<bool, CString> filePath = VerifyFilePath(fileName);
    if (!filePath.first) {
        LOG(ERROR, RUNTIME) << "snapshot file path error";
        return;
    }
    write.open(filePath.second.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
    if (!write.good()) {
        LOG(DEBUG, RUNTIME) << "snapshot open file failed";
        return;
    }

    SnapShotSerialize serialize(vm_, true);

    std::unordered_map<uint64_t, SlotBit> data;
    CQueue<TaggedObject *> objectQueue;

    serialize.RegisterNativeMethod();

    // handle GlobalEnvConstants
    auto constant = const_cast<GlobalEnvConstants *>(vm_->GetJSThread()->GlobalConstants());
    constant->VisitRangeSlot([&objectQueue, &data](Root type, ObjectSlot start, ObjectSlot end) {
        SerializeHelper::AddTaggedObjectRangeToData(start, end, &objectQueue, &data);
    });

    vm_->Iterate([&objectQueue, &data](Root type, ObjectSlot object) {
        SerializeHelper::AddObjectHeaderToData(object.GetTaggedObjectHeader(), &objectQueue, &data);
    });

    while (!objectQueue.empty()) {
        auto taggedObject = objectQueue.front();
        if (taggedObject == nullptr) {
            break;
        }
        objectQueue.pop();

        serialize.Serialize(taggedObject, &objectQueue, &data);
    }

    serialize.SetProgramSerializeStart();

    // handle program
    if (program != nullptr) {
        SerializeHelper::AddObjectHeaderToData(program, &objectQueue, &data);
    }

    while (!objectQueue.empty()) {
        auto taggedObject = objectQueue.front();
        if (taggedObject == nullptr) {
            break;
        }
        objectQueue.pop();
        serialize.Serialize(taggedObject, &objectQueue, &data);
    }

    serialize.SerializePandaFileMethod();

    auto region = vm_->GetHeap()->GetSnapShotSpace()->GetCurrentRegion();
    region->SetHighWaterMark(vm_->GetFactory()->GetHeapManager().GetSnapShotSpaceAllocator().GetTop());

    // write to file
    SnapShotSpace *space = vm_->GetHeap()->GetSnapShotSpace();
    uint32_t snapshot_size = space->GetRegionCount() * DEFAULT_SNAPSHOT_SPACE_SIZE;
    uint32_t panda_file_begin = RoundUp(snapshot_size + sizeof(Header), PANDA_FILE_ALIGNMENT);
    Header hdr {snapshot_size, panda_file_begin};
    write.write(reinterpret_cast<char *>(&hdr), sizeof(hdr));

    space->EnumerateRegions([&write](Region *current) {
        write.write(reinterpret_cast<char *>(current), DEFAULT_SNAPSHOT_SPACE_SIZE);
        write.flush();
    });
    space->ReclaimRegions();
    ASSERT(static_cast<size_t>(write.tellp()) == snapshot_size + sizeof(Header));

    write.seekp(panda_file_begin);
    write.write(reinterpret_cast<const char *>(pf->GetBase()), pf->GetHeader()->file_size);
    write.close();
}

std::unique_ptr<const panda_file::File> SnapShot::DeserializeGlobalEnvAndProgram(const CString &fileName)
{
    SnapShotSerialize serialize(vm_, false);

    serialize.GeneratedNativeMethod();

    std::pair<bool, CString> filePath = VerifyFilePath(fileName);
    if (!filePath.first) {
        LOG(ERROR, RUNTIME) << "snapshot file path error";
        return std::unique_ptr<const panda_file::File>();
    }

    int fd = open(filePath.second.c_str(), O_CLOEXEC);  // NOLINT(cppcoreguidelines-pro-type-vararg)
    if (UNLIKELY(fd == -1)) {
        LOG_ECMA(FATAL) << "open file failed";
        UNREACHABLE();
    }
    size_t file_size = lseek(fd, 0, SEEK_END);
    auto readFile = ToUintPtr(mmap(nullptr, file_size, PROT_READ, MAP_PRIVATE, fd, 0));
    auto hdr = *ToNativePtr<const Header>(readFile);
    if (hdr.snapshot_size % DEFAULT_SNAPSHOT_SPACE_SIZE != 0) {
        LOG_ECMA(FATAL) << "Invalid snapshot file";
        UNREACHABLE();
    }
    SnapShotSpace *space = vm_->GetHeap()->GetSnapShotSpace();

    uintptr_t snapshot_begin = readFile + sizeof(Header);
    for (size_t i = 0; i < hdr.snapshot_size / DEFAULT_SNAPSHOT_SPACE_SIZE; i++) {
        Region *region = const_cast<RegionFactory *>(vm_->GetHeap()->GetRegionFactory())
                            ->AllocateAlignedRegion(space, DEFAULT_SNAPSHOT_SPACE_SIZE);
        auto fileRegion = ToNativePtr<Region>(snapshot_begin + i * DEFAULT_SNAPSHOT_SPACE_SIZE);

        uint64_t base = region->allocateBase_;
        uint64_t begin = (fileRegion->begin_) % DEFAULT_SNAPSHOT_SPACE_SIZE;
        uint64_t waterMark = (fileRegion->highWaterMark_) % DEFAULT_SNAPSHOT_SPACE_SIZE;

        if (memcpy_s(region, DEFAULT_SNAPSHOT_SPACE_SIZE, fileRegion, DEFAULT_SNAPSHOT_SPACE_SIZE) != EOK) {
            LOG_ECMA(FATAL) << "memcpy_s failed";
            UNREACHABLE();
        }

        // space
        region->space_ = space;
        // allocate_base_
        region->allocateBase_ = base;
        // begin_
        region->begin_ = ToUintPtr(region) + begin;
        // end_
        region->end_ = ToUintPtr(region) + DEFAULT_SNAPSHOT_SPACE_SIZE;
        // high_water_mark_
        region->highWaterMark_ = ToUintPtr(region) + waterMark;
        // prev_
        region->prev_ = nullptr;
        // next_
        region->next_ = nullptr;
        // mark_bitmap_
        region->markBitmap_ = nullptr;
        // cross_region_set_
        region->crossRegionSet_ = nullptr;
        // old_to_new_set_
        region->oldToNewSet_ = nullptr;

        space->AddRegion(region);
    }
    munmap(ToNativePtr<void>(readFile), hdr.panda_file_begin);
    uintptr_t panda_file_mem = readFile + hdr.panda_file_begin;
    auto pf =
        panda_file::File::OpenFromMemory(os::mem::ConstBytePtr(ToNativePtr<std::byte>(panda_file_mem),
                                                               file_size - hdr.panda_file_begin, os::mem::MmapDeleter),
                                         fileName);
    close(fd);
    // redirect object field
    serialize.RedirectSlot(pf.get());
    return pf;
}

size_t SnapShot::AlignUpPageSize(size_t spaceSize)
{
    if (spaceSize % PAGE_SIZE_ALIGN_UP == 0) {
        return spaceSize;
    }
    return PAGE_SIZE_ALIGN_UP * (spaceSize / PAGE_SIZE_ALIGN_UP + 1);
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
}  // namespace panda::ecmascript
