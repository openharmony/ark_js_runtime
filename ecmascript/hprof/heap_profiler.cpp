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

#include "ecmascript/hprof/heap_profiler.h"

#include <cerrno>
#include <ctime>
#include <unistd.h>

#include "ecmascript/ecma_vm.h"
#include "ecmascript/hprof/heap_snapshot.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/mem/assert_scope-inl.h"
#include "ecmascript/mem/concurrent_sweeper.h"
#include "ecmascript/mem/heap.h"

namespace panda::ecmascript {
HeapProfiler::~HeapProfiler()
{
    ClearSnapShot();
    const_cast<RegionFactory *>(heap_->GetRegionFactory())->Delete(jsonSerializer_);
    jsonSerializer_ = nullptr;
}

bool HeapProfiler::DumpHeapSnapShot(JSThread *thread, DumpFormat dumpFormat, const std::string &filePath, bool isVmMode)
{
    [[maybe_unused]] bool heapClean = ForceFullGC(thread);
    ASSERT(heapClean);
    auto heap = thread->GetEcmaVM()->GetHeap();
    size_t heapSize = heap->GetNewSpace()->GetHeapObjectSize() + heap->GetOldSpace()->GetHeapObjectSize()
                         + heap->GetNonMovableSpace()->GetHeapObjectSize();
    LOG(ERROR, RUNTIME) << "HeapProfiler DumpSnapshot heap size " << heapSize;
    HeapSnapShot *snapShot = MakeHeapSnapShot(thread, SampleType::ONE_SHOT, isVmMode);
    ASSERT(snapShot != nullptr);
    std::pair<bool, CString> realPath = FilePathValid(filePath);
    if (realPath.first) {
        return jsonSerializer_->Serialize(snapShot, realPath.second);
    }

    std::pair<bool, CString> realGenPath = FilePathValid(GenDumpFileName(dumpFormat));
    if (realGenPath.first) {
        return jsonSerializer_->Serialize(snapShot, realGenPath.second);
    }
    UNREACHABLE();
}

bool HeapProfiler::StartHeapTracking(JSThread *thread, double timeInterval, bool isVmMode)
{
    HeapSnapShot *snapShot = MakeHeapSnapShot(thread, SampleType::REAL_TIME, isVmMode);
    if (snapShot == nullptr) {
        return false;
    }
    heapTracker_ = std::make_unique<HeapTracker>(snapShot, timeInterval);
    thread->GetEcmaVM()->StartHeapTracking(heapTracker_.get());
    heapTracker_->StartTracing();
    return true;
}

bool HeapProfiler::StopHeapTracking(JSThread *thread, DumpFormat dumpFormat, const std::string &path)
{
    if (heapTracker_ == nullptr) {
        return false;
    }
    thread->GetEcmaVM()->StopHeapTracking();
    heapTracker_->StopTracing();

    // check path
    if (path.empty()) {
        return false;
    }
    std::pair<bool, CString> realPath = FilePathValid(path);
    if (!realPath.first) {
        return false;
    }

    HeapSnapShot *snapShot = hprofs_.at(0);
    if (snapShot == nullptr) {
        return false;
    }
    snapShot->FinishSnapShot();
    return jsonSerializer_->Serialize(snapShot, realPath.second);
}

std::pair<bool, CString> HeapProfiler::FilePathValid(const std::string &filePath)
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

std::string HeapProfiler::GenDumpFileName(DumpFormat dumpFormat)
{
    CString filename("hprof_");
    switch (dumpFormat) {
        case DumpFormat::JSON:
            filename.append(GetTimeStamp());
            break;
        case DumpFormat::BINARY:
            filename.append("unimplemented");
            break;
        case DumpFormat::OTHER:
            filename.append("unimplemented");
            break;
        default:
            filename.append("unimplemented");
            break;
    }
    filename.append(".heapsnapshot");
    return CstringConvertToString(filename);
}

CString HeapProfiler::GetTimeStamp()
{
    std::time_t timeSource = std::time(nullptr);
    tm *timeData = localtime(&timeSource);
    CString stamp;
    const int TIME_START = 1900;
    stamp.append(ToCString(timeData->tm_year + TIME_START))
        .append("-")
        .append(ToCString(timeData->tm_mon + 1))
        .append("-")
        .append(ToCString(timeData->tm_mday))
        .append("_")
        .append(ToCString(timeData->tm_hour))
        .append("-")
        .append(ToCString(timeData->tm_min))
        .append("-")
        .append(ToCString(timeData->tm_sec));
    return stamp;
}

bool HeapProfiler::ForceFullGC(JSThread *thread)
{
    auto vm = thread->GetEcmaVM();
    if (vm->IsInitialized()) {
        const_cast<Heap *>(vm->GetHeap())->CollectGarbage(TriggerGCType::SEMI_GC);
        const_cast<Heap *>(vm->GetHeap())->CollectGarbage(TriggerGCType::OLD_GC);
        return true;
    }
    return false;
}

HeapSnapShot *HeapProfiler::MakeHeapSnapShot(JSThread *thread, SampleType sampleType, bool isVmMode)
{
    LOG(ERROR, RUNTIME) << "HeapProfiler::MakeHeapSnapShot";
    DISALLOW_GARBAGE_COLLECTION;
    heap_->GetSweeper()->EnsureAllTaskFinished();
    switch (sampleType) {
        case SampleType::ONE_SHOT: {
            auto *snapShot =
                const_cast<RegionFactory *>(heap_->GetRegionFactory())->New<HeapSnapShot>(thread, heap_, isVmMode);
            if (snapShot == nullptr) {
                LOG_ECMA(FATAL) << "alloc snapshot failed";
                UNREACHABLE();
            }
            snapShot->BuildUp(thread);
            AddSnapShot(snapShot);
            return snapShot;
        }
        case SampleType::REAL_TIME: {
            auto *snapShot =
                const_cast<RegionFactory *>(heap_->GetRegionFactory())->New<HeapSnapShot>(thread, heap_, isVmMode);
            if (snapShot == nullptr) {
                LOG_ECMA(FATAL) << "alloc snapshot failed";
                UNREACHABLE();
            }
            AddSnapShot(snapShot);
            snapShot->PrepareSnapShot();
            return snapShot;
        }
        default:
            return nullptr;
    }
}

void HeapProfiler::AddSnapShot(HeapSnapShot *snapshot)
{
    if (hprofs_.size() >= MAX_NUM_HPROF) {
        ClearSnapShot();
    }
    ASSERT(snapshot != nullptr);
    hprofs_.emplace_back(snapshot);
}

void HeapProfiler::ClearSnapShot()
{
    for (auto *snapshot : hprofs_) {
        const_cast<RegionFactory *>(heap_->GetRegionFactory())->Delete(snapshot);
    }
    hprofs_.clear();
}
}  // namespace panda::ecmascript
