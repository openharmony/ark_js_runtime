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

#include "ecmascript/hprof/heap_snapshot.h"

#include <functional>

#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/global_dictionary.h"
#include "ecmascript/global_env.h"
#include "ecmascript/hprof/heap_root_visitor.h"
#include "ecmascript/ic/property_box.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_hclass-inl.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_symbol.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/mem/assert_scope-inl.h"
#include "ecmascript/property_attributes.h"
#include "ecmascript/tagged_array.h"
#include "ecmascript/tagged_dictionary.h"

namespace panda::ecmascript {
CString *HeapSnapShot::GetString(const CString &as)
{
    return stringTable_.GetString(as);
}

Node *Node::NewNode(const Heap *heap, size_t id, size_t index, CString *name, NodeType type, size_t size,
                    TaggedObject *entry, bool isLive)
{
    auto node = const_cast<RegionFactory *>(
        heap->GetRegionFactory())->New<Node>(id, index, name, type, size, 0, NewAddress<TaggedObject>(entry), isLive);
    if (UNLIKELY(node == nullptr)) {
        LOG_ECMA(FATAL) << "internal allocator failed";
        UNREACHABLE();
    }
    return node;
}

Edge *Edge::NewEdge(const Heap *heap, uint64_t id, EdgeType type, Node *from, Node *to, CString *name)
{
    auto edge = const_cast<RegionFactory *>(heap->GetRegionFactory())->New<Edge>(id, type, from, to, name);
    if (UNLIKELY(edge == nullptr)) {
        LOG_ECMA(FATAL) << "internal allocator failed";
        UNREACHABLE();
    }
    return edge;
}

HeapSnapShot::~HeapSnapShot()
{
    const Heap *heap = thread_->GetEcmaVM()->GetHeap();
    for (Node *node : nodes_) {
        const_cast<RegionFactory *>(heap->GetRegionFactory())->Delete(node);
    }
    for (Edge *edge : edges_) {
        const_cast<RegionFactory *>(heap->GetRegionFactory())->Delete(edge);
    }
    nodes_.clear();
    edges_.clear();
}

bool HeapSnapShot::BuildUp(JSThread *thread)
{
    FillNodes(thread);
    FillEdges(thread);
    AddSyntheticRoot(thread);
    return Verify();
}

bool HeapSnapShot::Verify()
{
    GetString(CString("HeapVerify:").append(ToCString(totalNodesSize_)));
    return (edgeCount_ > nodeCount_) && (totalNodesSize_ > 0);
}

void HeapSnapShot::PrepareSnapShot()
{
    FillNodes(thread_);
}

void HeapSnapShot::UpdateNode()
{
    for (Node *node : nodes_) {
        node->SetLive(false);
    }
    FillNodes(thread_);

    for (auto iter = nodes_.begin(); iter != nodes_.end();) {
        if (!(*iter)->IsLive()) {
            iter = nodes_.erase(iter);
        } else {
            iter++;
        }
    }
}

bool HeapSnapShot::FinishSnapShot()
{
    UpdateNode();
    FillEdges(thread_);
    AddSyntheticRoot(thread_);
    return Verify();
}

void HeapSnapShot::RecordSampleTime()
{
    timeStamps_.emplace_back(sequenceId_);
}

void HeapSnapShot::AddNode(uintptr_t address)
{
    GenerateNode(thread_, JSTaggedValue(address));
}

void HeapSnapShot::MoveNode(uintptr_t address, uintptr_t forward_address)
{
    int sequenceId = -1;
    Node *node = entryMap_.FindAndEraseNode(address);
    if (node != nullptr) {
        sequenceId = node->GetId();
        EraseNodeUnique(node);
    }
    GenerateNode(thread_, JSTaggedValue(forward_address), sequenceId);
}

// NOLINTNEXTLINE(readability-function-size)
CString *HeapSnapShot::GenerateNodeName(JSThread *thread, TaggedObject *entry)
{
    auto *hCls = entry->GetClass();
    JSType type = hCls->GetObjectType();
    switch (type) {
        case JSType::TAGGED_ARRAY: {
            CString arrayName;
            TaggedArray *array = TaggedArray::Cast(entry);
            arrayName = "TaggedArray[";
            arrayName.append(ToCString(array->GetLength()));
            arrayName.append("]");
            return GetString(arrayName);  // String type was handled singly, see#GenerateStringNode
        }
        case JSType::HCLASS:
            return GetString("HiddenClass");
        case JSType::TAGGED_DICTIONARY: {
            CString dictName;
            TaggedArray *dict = TaggedArray::Cast(entry);
            dictName = "TaggedDict[";
            dictName.append(ToCString(dict->GetLength()));
            dictName.append("]");
            return GetString(dictName);
        }
        case JSType::STRING:
            return GetString("BaseString");
        case JSType::JS_OBJECT: {
            CString objName = CString("JSOBJECT(Ctor=");  // Ctor-name
            return GetString(objName);
        }
        case JSType::FREE_OBJECT_WITH_ONE_FIELD:
        case JSType::FREE_OBJECT_WITH_NONE_FIELD:
        case JSType::FREE_OBJECT_WITH_TWO_FIELD:
        case JSType::JS_NATIVE_POINTER:
        {
            break;
        }
        case JSType::JS_FUNCTION_BASE:
            return GetString("JSFunctionBase");
        case JSType::JS_FUNCTION:
            return GetString("JSFunction");
        case JSType::JS_ERROR:
            return GetString("Error");
        case JSType::JS_EVAL_ERROR:
            return GetString("Eval Error");
        case JSType::JS_RANGE_ERROR:
            return GetString("Range Error");
        case JSType::JS_TYPE_ERROR:
            return GetString("Type Error");
        case JSType::JS_REFERENCE_ERROR:
            return GetString("Reference Error");
        case JSType::JS_URI_ERROR:
            return GetString("Uri Error");
        case JSType::JS_SYNTAX_ERROR:
            return GetString("Syntax Error");
        case JSType::JS_REG_EXP:
            return GetString("Regexp");
        case JSType::JS_SET:
            return GetString("Set");
        case JSType::JS_MAP:
            return GetString("Map");
        case JSType::JS_WEAK_SET:
            return GetString("WeakSet");
        case JSType::JS_WEAK_MAP:
            return GetString("WeakMap");
        case JSType::JS_DATE:
            return GetString("Date");
        case JSType::JS_BOUND_FUNCTION:
            return GetString("Bound Function");
        case JSType::JS_ARRAY: {
            JSArray *jsArray = JSArray::Cast(entry);
            CString jsArrayName("JSArray[");
            jsArrayName.append(ToCString(jsArray->GetLength().GetInt()));
            jsArrayName.append("]");
            return GetString(jsArrayName);
        }
        case JSType::JS_TYPED_ARRAY:
            return GetString("Typed Array");
        case JSType::JS_INT8_ARRAY:
            return GetString("Int8 Array");
        case JSType::JS_UINT8_ARRAY:
            return GetString("Uint8 Array");
        case JSType::JS_UINT8_CLAMPED_ARRAY:
            return GetString("Uint8 Clamped Array");
        case JSType::JS_INT16_ARRAY:
            return GetString("Int16 Array");
        case JSType::JS_UINT16_ARRAY:
            return GetString("Uint16 Array");
        case JSType::JS_INT32_ARRAY:
            return GetString("Int32 Array");
        case JSType::JS_UINT32_ARRAY:
            return GetString("Uint32 Array");
        case JSType::JS_FLOAT32_ARRAY:
            return GetString("Float32 Array");
        case JSType::JS_FLOAT64_ARRAY:
            return GetString("Float64 Array");
        case JSType::JS_ARGUMENTS:
            return GetString("Arguments");
        case JSType::JS_PROXY:
            return GetString("Proxy");
        case JSType::JS_PRIMITIVE_REF:
            return GetString("Primitive");
        case JSType::JS_DATA_VIEW:
            return GetString("DataView");
        case JSType::JS_ITERATOR:
            return GetString("Iterator");
        case JSType::JS_FORIN_ITERATOR:
            return GetString("ForinInterator");
        case JSType::JS_MAP_ITERATOR:
            return GetString("MapIterator");
        case JSType::JS_SET_ITERATOR:
            return GetString("SetIterator");
        case JSType::JS_ARRAY_ITERATOR:
            return GetString("ArrayIterator");
        case JSType::JS_STRING_ITERATOR:
            return GetString("StringIterator");
        case JSType::JS_ARRAY_BUFFER:
            return GetString("ArrayBuffer");
        case JSType::JS_PROXY_REVOC_FUNCTION:
            return GetString("ProxyRevocFunction");
        case JSType::PROMISE_REACTIONS:
            return GetString("PromiseReaction");
        case JSType::PROMISE_CAPABILITY:
            return GetString("PromiseCapability");
        case JSType::PROMISE_ITERATOR_RECORD:
            return GetString("PromiseIteratorRecord");
        case JSType::PROMISE_RECORD:
            return GetString("PromiseRecord");
        case JSType::RESOLVING_FUNCTIONS_RECORD:
            return GetString("ResolvingFunctionsRecord");
        case JSType::JS_PROMISE:
            return GetString("Promise");
        case JSType::JS_PROMISE_REACTIONS_FUNCTION:
            return GetString("PromiseReactionsFunction");
        case JSType::JS_PROMISE_EXECUTOR_FUNCTION:
            return GetString("PromiseExecutorFunction");
        case JSType::JS_PROMISE_ALL_RESOLVE_ELEMENT_FUNCTION:
            return GetString("PromiseAllResolveElementFunction");
        case JSType::JS_GENERATOR_FUNCTION:
            return GetString("JSGeneratorFunction");
        case JSType::SYMBOL:
            return GetString("Symbol");
        case JSType::JS_ASYNC_FUNCTION:
            return GetString("AsyncFunction");
        case JSType::JS_INTL_BOUND_FUNCTION:
            return GetString("JSIntlBoundFunction");
        case JSType::JS_ASYNC_AWAIT_STATUS_FUNCTION:
            return GetString("AsyncAwaitStatusFunction");
        case JSType::JS_ASYNC_FUNC_OBJECT:
            return GetString("AsyncFunctionObject");
        case JSType::JS_REALM:
            return GetString("Realm");
        case JSType::JS_GLOBAL_OBJECT:
            return GetString("GlobalObject");
        case JSType::JS_INTL:
            return GetString("JSIntl");
        case JSType::JS_LOCALE:
            return GetString("JSLocale");
        case JSType::JS_DATE_TIME_FORMAT:
            return GetString("JSDateTimeFormat");
        case JSType::JS_RELATIVE_TIME_FORMAT:
            return GetString("JSRelativeTimeFormat");
        case JSType::JS_NUMBER_FORMAT:
            return GetString("JSNumberFormat");
        case JSType::JS_COLLATOR:
            return GetString("JSCollator");
        case JSType::JS_PLURAL_RULES:
            return GetString("JSPluralRules");
        case JSType::JS_GENERATOR_OBJECT:
            return GetString("JSGeneratorObject");
        case JSType::JS_GENERATOR_CONTEXT:
            return GetString("JSGeneratorContext");
        case JSType::ACCESSOR_DATA:
            return GetString("AccessorData");
        case JSType::INTERNAL_ACCESSOR:
            return GetString("InternalAccessor");
        case JSType::MICRO_JOB_QUEUE:
            return GetString("MicroJobQueue");
        case JSType::PENDING_JOB:
            return GetString("PendingJob");
        case JSType::COMPLETION_RECORD:
            return GetString("CompletionRecord");
        case JSType::SOURCE_TEXT_MODULE_RECORD:
            return GetString("SourceTextModule");
        case JSType::IMPORTENTRY_RECORD:
            return GetString("ImportEntry");
        case JSType::EXPORTENTRY_RECORD:
            return GetString("ExportEntry");
        case JSType::RESOLVEDBINDING_RECORD:
            return GetString("ResolvedBinding");
        case JSType::JS_MODULE_NAMESPACE:
            return GetString("ModuleNamespace");
        case JSType::JS_ARRAY_LIST:
            return GetString("ArrayList");
        case JSType::JS_API_TREE_MAP:
            return GetString("TreeMap");
        case JSType::JS_API_TREE_SET:
            return GetString("TreeSet");
        case JSType::JS_API_TREEMAP_ITERATOR:
            return GetString("TreeMapIterator");
        case JSType::JS_API_TREESET_ITERATOR:
            return GetString("TreeSetIterator");
        default:
            break;
    }
    if (IsInVmMode()) {
        switch (type) {
            case JSType::PROPERTY_BOX:
                return GetString("PropertyBox");
            case JSType::GLOBAL_ENV:
                return GetString("GlobalEnv");
            case JSType::PROTOTYPE_HANDLER:
                return GetString("ProtoTypeHandler");
            case JSType::TRANSITION_HANDLER:
                return GetString("TransitionHandler");
            case JSType::PROTO_CHANGE_MARKER:
                return GetString("ProtoChangeMarker");
            case JSType::PROTOTYPE_INFO:
                return GetString("ProtoChangeDetails");
            case JSType::TEMPLATE_MAP:
                return GetString("TemplateMap");
            case JSType::PROGRAM:
                return GetString("Program");
            case JSType::MACHINE_CODE_OBJECT:
                return GetString("MachineCode");
            case JSType::CLASS_INFO_EXTRACTOR:
                return GetString("ClassInfoExtractor");
            case JSType::TS_OBJECT_TYPE:
                return GetString("TSObjectType");
            case JSType::TS_INTERFACE_TYPE:
                return GetString("TSInterfaceType");
            case JSType::TS_CLASS_TYPE:
                return GetString("TSClassType");
            case JSType::TS_UNION_TYPE:
                return GetString("TSUnionType");
            case JSType::TS_IMPORT_TYPE:
                return GetString("TSImportType");
            case JSType::TS_CLASS_INSTANCE_TYPE:
                return GetString("TSClassInstanceType");
            default:
                break;
        }
    } else {
        return GetString("Hidden Object");
    }
    return GetString("UnKnownType");
}

NodeType HeapSnapShot::GenerateNodeType(TaggedObject *entry)
{
    NodeType nodeType = NodeType::INVALID;
    auto *hCls = entry->GetClass();
    if (hCls->IsTaggedArray()) {
        nodeType = NodeType::JS_ARRAY;
    } else if (hCls->IsHClass()) {
        nodeType = NodeType::HCLASS;
    } else {
        nodeType = NodeType(hCls->GetObjectType());
    }
    return nodeType;
}

void HeapSnapShot::FillNodes(JSThread *thread)
{
    // Iterate Heap Object
    auto heap = thread_->GetEcmaVM()->GetHeap();
    if (heap != nullptr) {
        heap->IteratorOverObjects(
            [this, &thread](TaggedObject *obj) { this->GenerateNode(thread, JSTaggedValue(obj)); });
    }
}

Node *HeapSnapShot::GenerateNode(JSThread *thread, JSTaggedValue entry, int sequenceId)
{
    Node *node = nullptr;
    if (sequenceId == -1) {
        sequenceId = sequenceId_ + SEQ_STEP;
    }
    if (entry.IsHeapObject()) {
        if (entry.IsWeak()) {
            entry.RemoveWeakTag();
        }
        if (entry.IsString()) {
            node = GenerateStringNode(entry, sequenceId);
            if (node == nullptr) {
                LOG(DEBUG, RUNTIME) << "string node nullptr";
            }
        }
        TaggedObject *obj = entry.GetTaggedObject();
        auto *baseClass = obj->GetClass();
        if (baseClass != nullptr) {
            node = Node::NewNode(heap_, sequenceId, nodeCount_, GenerateNodeName(thread, obj), GenerateNodeType(obj),
                                 obj->GetClass()->SizeFromJSHClass(obj), obj);
            Node *existNode = entryMap_.FindOrInsertNode(node);  // Fast Index
            if (existNode == node) {
                if (sequenceId == sequenceId_ + SEQ_STEP) {
                    sequenceId_ = sequenceId;  // Odd Digit
                }
                InsertNodeUnique(node);
                ASSERT(entryMap_.FindEntry(node->GetAddress())->GetAddress() == node->GetAddress());
            } else {
                existNode->SetLive(true);
                ASSERT(entryMap_.FindEntry(node->GetAddress())->GetAddress() == node->GetAddress());
                const_cast<RegionFactory *>(heap_->GetRegionFactory())->Delete(node);
                return nullptr;
            }
        }
    } else {
        CString primitiveName;
        // A primitive value with tag will be regarded as a pointer
        auto *obj = reinterpret_cast<TaggedObject *>(entry.GetRawData());
        if (entry.IsInt()) {
            primitiveName.append("Int:" + ToCString(entry.GetInt()));
        } else if (entry.IsDouble()) {
            primitiveName.append("Double:");
        } else if (entry.IsHole()) {
            primitiveName.append("Hole");
        } else if (entry.IsNull()) {
            primitiveName.append("Null");
        } else if (entry.IsTrue()) {
            primitiveName.append("Boolean:true");
        } else if (entry.IsFalse()) {
            primitiveName.append("Boolean:false");
        } else if (entry.IsException()) {
            primitiveName.append("Exception");
        } else if (entry.IsUndefined()) {
            primitiveName.append("Undefined");
        } else {
            primitiveName.append("Illegal_Primitive");
        }

        node = Node::NewNode(heap_, sequenceId, nodeCount_, GetString(primitiveName), NodeType::JS_PRIMITIVE_REF, 0,
                             obj);
        entryMap_.InsertEntry(node);  // Fast Index
        if (sequenceId == sequenceId_ + SEQ_STEP) {
            sequenceId_ = sequenceId;  // Odd Digit
        }
        InsertNodeUnique(node);
    }
    return node;
}

Node *HeapSnapShot::GenerateStringNode(JSTaggedValue entry, int sequenceId)
{
    Node *node = nullptr;
    auto originStr = static_cast<EcmaString *>(entry.GetTaggedObject());
    size_t selfsize = originStr->ObjectSize();
    CString strContent;
    strContent.append(EntryVisitor::ConvertKey(entry));
    node = Node::NewNode(heap_, sequenceId, nodeCount_, GetString(strContent), NodeType::PRIM_STRING, selfsize,
                         entry.GetTaggedObject());
    Node *existNode = entryMap_.FindOrInsertNode(node);  // Fast Index
    if (existNode == node) {
        if (sequenceId == sequenceId_ + SEQ_STEP) {
            sequenceId_ = sequenceId;  // Odd Digit
        }
        InsertNodeUnique(node);
    } else {
        existNode->SetLive(true);
    }
    ASSERT(entryMap_.FindEntry(node->GetAddress())->GetAddress() == node->GetAddress());
    if (existNode != node) {
        const_cast<RegionFactory *>(heap_->GetRegionFactory())->Delete(node);
        return nullptr;
    }
    return node;
}

void HeapSnapShot::FillEdges(JSThread *thread)
{
    size_t length = nodes_.size();
    auto iter = nodes_.begin();
    size_t count = 0;
    while (++count < length) {
        ASSERT(*iter != nullptr);
        auto *objFrom = reinterpret_cast<TaggedObject *>((*iter)->GetAddress());
        std::vector<std::pair<CString, JSTaggedValue>> nameResources;
        JSTaggedValue(objFrom).DumpForSnapshot(thread, nameResources, isVmMode_);
        JSTaggedValue objValue(objFrom);
        for (auto const &it : nameResources) {
            JSTaggedValue toValue = it.second;
            Node *entryTo = nullptr;
            if (toValue.IsHeapObject()) {
                auto *to = reinterpret_cast<TaggedObject *>(toValue.GetHeapObject());
                entryTo = entryMap_.FindEntry(Node::NewAddress(to));
            }
            if (entryTo == nullptr) {
                entryTo = GenerateNode(thread, toValue);
            }
            if (entryTo != nullptr) {
                Edge *edge = Edge::NewEdge(heap_, edgeCount_, EdgeType::DEFAULT, *iter, entryTo, GetString(it.first));
                InsertEdgeUnique(edge);
                (*iter)->IncEdgeCount();  // Update Node's edgeCount_ here
            }
        }
        iter++;
    }
    // Fill Primitive Edge
    size_t lengthExtend = nodes_.size();
    while (++count < lengthExtend) {
        ASSERT(*iter != nullptr);
        if ((*iter)->GetType() == NodeType::JS_PRIMITIVE_REF) {
            JSTaggedValue jsFrom(reinterpret_cast<TaggedObject *>((*iter)->GetAddress()));
            CString valueName;
            if (jsFrom.IsInt()) {
                valueName.append(ToCString(jsFrom.GetInt()));
            } else if (jsFrom.IsDouble()) {
                valueName.append(FloatToCString(jsFrom.GetDouble()));
            } else {
                valueName.append("NaN");
            }
            Edge *edge = Edge::NewEdge(heap_, edgeCount_, EdgeType::DEFAULT, (*iter), (*iter), GetString(valueName));
            InsertEdgeUnique(edge);
            (*iter)->IncEdgeCount();  // Update Node's edgeCount_ here
        }
        iter++;
    }
}

void HeapSnapShot::BridgeAllReferences()
{
    // This Function is Unused
    for (Edge *edge : edges_) {
        auto *from = reinterpret_cast<TaggedObject *>(edge->GetFrom()->GetAddress());
        auto *to = reinterpret_cast<TaggedObject *>(edge->GetTo()->GetAddress());
        if (!JSTaggedValue(from).IsECMAObject()) {
            continue;  // named it by other way
        }
        edge->SetName(GenerateEdgeName(from, to));
    }
}

CString *HeapSnapShot::GenerateEdgeName([[maybe_unused]] TaggedObject *from, [[maybe_unused]] TaggedObject *to)
{
    // This Function is Unused
    ASSERT(from != nullptr && from != to);
    return GetString("[]");  // unAnalysed
}

Node *HeapSnapShot::InsertNodeUnique(Node *node)
{
    AccumulateNodeSize(node->GetSelfSize());
    nodes_.emplace_back(node);
    nodeCount_++;
    return node;
}

void HeapSnapShot::EraseNodeUnique(Node *node)
{
    auto iter = std::find(nodes_.begin(), nodes_.end(), node);
    if (iter != nodes_.end()) {
        DecreaseNodeSize(node->GetSelfSize());
        nodes_.erase(iter);
        nodeCount_--;
    }
}

Edge *HeapSnapShot::InsertEdgeUnique(Edge *edge)
{
    edges_.emplace_back(edge);
    edgeCount_++;
    return edge;
}

void HeapSnapShot::AddSyntheticRoot(JSThread *thread)
{
    Node *syntheticRoot = Node::NewNode(heap_, 1, nodeCount_, GetString("SyntheticRoot"),
                                        NodeType::SYNTHETIC, 0, nullptr);
    InsertNodeAt(0, syntheticRoot);

    int edgeOffset = 0;
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ROOT_EDGE_BUILDER_CORE(type, slot)                                                                      \
    JSTaggedValue value(slot.GetTaggedType());                                                                  \
    if (value.IsHeapObject()) {                                                                                 \
        TaggedObject *root = value.GetTaggedObject();                                                           \
        Node *rootNode = entryMap_.FindEntry(Node::NewAddress(root));                                           \
        if (rootNode != nullptr) {                                                                              \
            Edge *edge =                                                                                        \
                Edge::NewEdge(heap_, edgeCount_, EdgeType::SHORTCUT, syntheticRoot, rootNode, GetString("-subroot-")); \
            InsertEdgeAt(edgeOffset, edge);                                                                     \
            edgeOffset++;                                                                                       \
            syntheticRoot->IncEdgeCount();                                                                      \
        }                                                                                                       \
    }

    RootVisitor rootEdgeBuilder = [this, syntheticRoot, &edgeOffset]([[maybe_unused]] Root type, ObjectSlot slot) {
        ROOT_EDGE_BUILDER_CORE(type, slot);
    };

    RootRangeVisitor rootRangeEdgeBuilder = [this, syntheticRoot, &edgeOffset]([[maybe_unused]] Root type,
                                                                               ObjectSlot start, ObjectSlot end) {
        for (ObjectSlot slot = start; slot < end; slot++) {
            ROOT_EDGE_BUILDER_CORE(type, slot);
        }
    };
#undef ROOT_EDGE_BUILDER_CORE
    rootVisitor_.VisitHeapRoots(thread, rootEdgeBuilder, rootRangeEdgeBuilder);

    int reindex = 0;
    for (Node *node : nodes_) {
        node->SetIndex(reindex);
        reindex++;
    }
}

Node *HeapSnapShot::InsertNodeAt(size_t pos, Node *node)
{
    ASSERT(node != nullptr);
    auto iter = nodes_.begin();
    std::advance(iter, pos);
    nodes_.insert(iter, node);
    nodeCount_++;
    return node;
}

Edge *HeapSnapShot::InsertEdgeAt(size_t pos, Edge *edge)
{
    ASSERT(edge != nullptr);
    edges_.insert(edges_.begin() + pos, edge);
    edgeCount_++;
    return edge;
}

CString EntryVisitor::ConvertKey(JSTaggedValue key)
{
    ASSERT(key.GetTaggedObject() != nullptr);
    EcmaString *keyString = EcmaString::Cast(key.GetTaggedObject());
    if (key.IsSymbol()) {
        JSSymbol *symbol = JSSymbol::Cast(key.GetTaggedObject());
        keyString = EcmaString::Cast(symbol->GetDescription().GetTaggedObject());
    }
    // convert, expensive but safe
    int length = 0;
    if (keyString->IsUtf8()) {
        length = keyString->GetUtf8Length();
        std::vector<uint8_t> buffer(length);
        [[maybe_unused]] int size = keyString->CopyDataUtf8(buffer.data(), length);
        ASSERT(size == length);
        CString keyCopy(reinterpret_cast<char *>(buffer.data()));
        return keyCopy;
    } else {  // NOLINT(readability-else-after-return)
        length = keyString->GetLength();
        std::vector<uint16_t> buffer(length);
        [[maybe_unused]] int size = keyString->CopyDataUtf16(buffer.data(), length);
        ASSERT(size == length);
        CString keyCopy(reinterpret_cast<char *>(buffer.data()));
        return keyCopy;
    }
}

Node *HeapEntryMap::FindOrInsertNode(Node *node)
{
    ASSERT(node != nullptr);
    auto it = nodesMap_.find(node->GetAddress());
    if (it != nodesMap_.end()) {
        return it->second;
    }
    InsertEntry(node);
    return node;
}

Node *HeapEntryMap::FindAndEraseNode(Address addr)
{
    auto it = nodesMap_.find(addr);
    if (it != nodesMap_.end()) {
        Node *node = it->second;
        nodesMap_.erase(it);
        nodeEntryCount_--;
        return node;
    }
    return nullptr;
}

Node *HeapEntryMap::FindEntry(Address addr)
{
    auto it = nodesMap_.find(addr);
    return it != nodesMap_.end() ? it->second : nullptr;
}

void HeapEntryMap::InsertEntry(Node *node)
{
    nodeEntryCount_++;
    nodesMap_.insert(std::make_pair(node->GetAddress(), node));
}

FrontType NodeTypeConverter::Convert(NodeType type)
{
    FrontType  fType = FrontType::DEFAULT;
    if (type == NodeType::PROPERTY_BOX) {
        fType = FrontType::HIDDEN;
    } else if (type == NodeType::JS_ARRAY || type == NodeType::JS_TYPED_ARRAY) {
        fType = FrontType::ARRAY;
    } else if (type == NodeType::PRIM_STRING) {  // STRING
        fType = FrontType::STRING;
    } else if (type == NodeType::JS_OBJECT) {
        fType = FrontType::OBJECT;
    } else if (type >= NodeType::JS_FUNCTION_BEGIN && type <= NodeType::JS_FUNCTION_END) {
        fType = FrontType::CLOSURE;
    } else if (type == NodeType::JS_BOUND_FUNCTION) {
        fType = FrontType::CLOSURE;
    } else if (type == NodeType::JS_FUNCTION_BASE) {
        fType = FrontType::CLOSURE;
    } else if (type == NodeType::JS_REG_EXP) {
        fType = FrontType::REGEXP;
    } else if (type == NodeType::SYMBOL) {
        fType = FrontType::SYMBOL;
    } else if (type == NodeType::JS_PRIMITIVE_REF) {
        fType = FrontType::HEAPNUMBER;
    } else if (type == NodeType::SYNTHETIC) {
        fType = FrontType::SYNTHETIC;
    } else {
        fType = FrontType::DEFAULT;
        // NATIVE,           /* kNative */
        // CONSSTRING,       /* kConsString */
        // SLICEDSTRING,     /* kSlicedString */
        // SYMBOL,           /* kSymbol */
        // BIGINT,           /* kBigInt */
    }
    return fType;
}
}  // namespace panda::ecmascript
