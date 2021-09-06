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

#ifndef RUNTIME_ECMASCRIPT_HPROF_HEAP_SNAPSHOT_H
#define RUNTIME_ECMASCRIPT_HPROF_HEAP_SNAPSHOT_H

#include <atomic>
#include <cstdint>
#include <fstream>
#include <sys/time.h>

#include "ecmascript/mem/c_containers.h"
#include "os/mem.h"
#include "ecmascript/hprof/heap_profiler.h"
#include "ecmascript/hprof/heap_root_visitor.h"
#include "ecmascript/hprof/string_hashmap.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_object.h"
#include "ecmascript/js_tagged_value.h"

namespace panda::ecmascript {
// Define the Object Graphic
using Address = uintptr_t;

enum class NodeType : uint8_t {
    JSTYPE_DECL,
    PRIM_STRING, /* Primitive String */
    PRIM_ARRAY,  /* Primitive Array */
    SYNTHETIC    /* For Synthetic Root */
};

enum class EdgeType { CONTEXT, ELEMENT, PROPERTY, INTERNAL, HIDDEN, SHORTCUT, WEAK, DEFAULT = PROPERTY };

class Node {
public:
    explicit Node(uint64_t id, uint64_t index, CString *name, NodeType type, size_t size, uint64_t traceId,
                  Address address, bool isLive = true)
        : id_(id),
          index_(index),
          name_(name),
          type_(type),
          size_(size),
          traceId_(traceId),
          address_(address),
          isLive_(isLive)
    {
    }
    uint64_t GetId()
    {
        return id_;
    }
    void SetIndex(uint64_t index)
    {
        index_ = index;
    }
    uint64_t GetIndex()
    {
        return index_;
    }
    CString *GetName()
    {
        return name_;
    }
    NodeType GetType()
    {
        return type_;
    }
    size_t GetSelfSize()
    {
        return size_;
    }
    size_t GetEdgeCount()
    {
        return edgeCount_;
    }
    void IncEdgeCount()
    {
        edgeCount_++;
    }
    uint64_t GetStackTraceId()
    {
        return traceId_;
    }
    Address GetAddress()
    {
        return address_;
    }
    bool IsLive()
    {
        return isLive_;
    }
    void SetLive(bool isLive)
    {
        isLive_ = isLive;
    }
    static Node *NewNode(size_t id, size_t index, CString *name, NodeType type, size_t size, TaggedObject *entry,
                         bool isLive = true);
    template <typename T>
    static Address NewAddress(T *addr)
    {
        return reinterpret_cast<Address>(addr);
    }
    static constexpr int NODE_FIELD_COUNT = 7;
    ~Node() = default;
private:
    uint64_t id_{0};  // Range from 1
    uint64_t index_{0};
    CString *name_{nullptr};
    NodeType type_{NodeType::INVALID};
    size_t size_{0};
    size_t edgeCount_{0};
    uint64_t traceId_{0};
    Address address_{0x0};
    bool isLive_{true};
};

class Edge {
public:
    explicit Edge(uint64_t id, EdgeType type, Node *from, Node *to, CString *name)
        : id_(id), edgeType_(type), from_(from), to_(to), name_(name)
    {
    }
    uint64_t GetId() const
    {
        return id_;
    }
    EdgeType GetType() const
    {
        return edgeType_;
    }
    Node *GetFrom() const
    {
        return from_;
    }
    Node *GetTo() const
    {
        return to_;
    }
    CString *GetName() const
    {
        return name_;
    }
    void SetName(CString *name)
    {
        name_ = name;
    }
    void UpdateFrom(Node *node)
    {
        from_ = node;
    }
    void UpdateTo(Node *node)
    {
        to_ = node;
    }
    static Edge *NewEdge(uint64_t id, EdgeType type, Node *from, Node *to, CString *name);
    static constexpr int EDGE_FIELD_COUNT = 3;
    ~Edge() = default;
private:
    uint64_t id_{-1ULL};
    EdgeType edgeType_{EdgeType::DEFAULT};
    Node *from_{nullptr};
    Node *to_{nullptr};
    CString *name_{nullptr};
};

class TimeStamp {
public:
    explicit TimeStamp(int sequenceId) : lastSequenceId_(sequenceId), timeStampUs_(TimeStamp::Now()) {}
    ~TimeStamp() = default;

    DEFAULT_MOVE_SEMANTIC(TimeStamp);
    DEFAULT_COPY_SEMANTIC(TimeStamp);

    int GetLastSequenceId()
    {
        return lastSequenceId_;
    }

    int64_t GetTimeStamp()
    {
        return timeStampUs_;
    }

private:
    static int64_t Now()
    {
        struct timeval tv = {0, 0};
        gettimeofday(&tv, nullptr);
        const int THOUSAND = 1000;
        return tv.tv_usec + tv.tv_sec * THOUSAND * THOUSAND;
    }

    int lastSequenceId_{0};
    int64_t timeStampUs_{0};
};

class HeapEntryMap {
public:
    HeapEntryMap() = default;
    ~HeapEntryMap() = default;
    NO_MOVE_SEMANTIC(HeapEntryMap);
    NO_COPY_SEMANTIC(HeapEntryMap);
    Node *FindOrInsertNode(Node *node);
    Node *FindAndEraseNode(Address addr);
    Node *FindEntry(Address addr);
    size_t GetCapcity()
    {
        return nodesMap_.size();
    }
    size_t GetEntryCount()
    {
        return nodeEntryCount_;
    }

private:
    void InsertEntry(Node *node);
    size_t nodeEntryCount_{0};
    CUnorderedMap<Address, Node *> nodesMap_{};
};

class HeapSnapShot {
public:
    static constexpr int SEQ_STEP = 2;
    NO_MOVE_SEMANTIC(HeapSnapShot);
    NO_COPY_SEMANTIC(HeapSnapShot);
    explicit HeapSnapShot(JSThread *thread, CAddressAllocator<JSTaggedType> *allocator)
        : stringTable_(allocator), thread_(thread)
    {
        allocator_ = allocator;
        ASSERT(allocator_ != nullptr);
    }
    ~HeapSnapShot();
    bool BuildUp(JSThread *thread);
    bool Verify();

    void PrepareSnapShot();
    void UpdateNode();
    void AddNode(uintptr_t address);
    void MoveNode(uintptr_t address, uintptr_t forward_address);
    void RecordSampleTime();
    bool FinishSnapShot();

    CVector<TimeStamp> &GetTimeStamps()
    {
        return timeStamps_;
    }

    size_t GetNodeCount()
    {
        return nodeCount_;
    }
    size_t GetEdgeCount()
    {
        return edgeCount_;
    }
    size_t GetTotalNodeSize()
    {
        return totalNodesSize_;
    }
    void AccumulateNodeSize(size_t size)
    {
        totalNodesSize_ += size;
    }
    void DecreaseNodeSize(size_t size)
    {
        totalNodesSize_ -= size;
    }
    CString *GenerateNodeName(JSThread *thread, TaggedObject *entry);
    NodeType GenerateNodeType(TaggedObject *entry);
    CList<Node *> *GetNodes()
    {
        return &nodes_;
    }
    CVector<Edge *> *GetEdges()
    {
        return &edges_;
    }
    StringHashMap *GetEcmaStringTable()
    {
        return &stringTable_;
    }

    static CAddressAllocator<JSTaggedType> *GetAllocator();
    CString *GetString(const CString &as);

private:
    void FillNodes(JSThread *thread);
    Node *GenerateNode(JSThread *thread, JSTaggedValue entry, int sequenceId = -1);
    Node *GenerateStringNode(JSTaggedValue entry, int sequenceId);
    void FillEdges(JSThread *thread);
    void BridgeAllReferences();
    CString *GenerateEdgeName(TaggedObject *from, TaggedObject *to);

    Node *InsertNodeUnique(Node *node);
    void EraseNodeUnique(Node *node);
    Edge *InsertEdgeUnique(Edge *edge);
    void AddSyntheticRoot(JSThread *thread);
    Node *InsertNodeAt(size_t pos, Node *node);
    Edge *InsertEdgeAt(size_t pos, Edge *edge);

    static CAddressAllocator<JSTaggedType> *allocator_;
    StringHashMap stringTable_;
    CList<Node *> nodes_{};
    CVector<Edge *> edges_{};
    CVector<TimeStamp> timeStamps_{};
    std::atomic_int sequenceId_{1};  // 1 Reversed for SyntheticRoot
    int nodeCount_{0};
    int edgeCount_{0};
    int totalNodesSize_{0};
    HeapEntryMap entryMap_;
    panda::ecmascript::HeapRootVisitor rootVisitor_;
    JSThread *thread_;
};

class EntryVisitor {
public:
    NO_MOVE_SEMANTIC(EntryVisitor);
    NO_COPY_SEMANTIC(EntryVisitor);
    explicit EntryVisitor() = default;
    ~EntryVisitor() = default;
    static CString ConvertKey(JSTaggedValue key);
};

enum class FrontType {
    HIDDEN,           /* kHidden */
    ARRAY,            /* kArray */
    STRING,           /* kString */
    OBJECT,           /* kObject */
    CODE,             /* kCode */
    CLOSURE,          /* kClosure */
    REGEXP,           /* kRegExp */
    HEAPNUMBER,       /* kHeapNumber */
    NATIVE,           /* kNative */
    SYNTHETIC,        /* kSynthetic */
    CONSSTRING,       /* kConsString */
    SLICEDSTRING,     /* kSlicedString */
    SYMBOL,           /* kSymbol */
    BIGINT,           /* kBigInt */
    DEFAULT = NATIVE, /* kDefault */
};

class NodeTypeConverter {
public:
    explicit NodeTypeConverter() = default;
    ~NodeTypeConverter() = default;
    NO_MOVE_SEMANTIC(NodeTypeConverter);
    NO_COPY_SEMANTIC(NodeTypeConverter);
    /*
     * For Front-End to Show Statistics Correctly
     */
    static FrontType Convert(NodeType type);
};
}  // namespace panda::ecmascript
#endif  // RUNTIME_ECMASCRIPT_HPROF_HEAP_SNAPSHOT_H
