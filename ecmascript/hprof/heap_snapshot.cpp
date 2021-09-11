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
    CString *name = GetString("UnKnownType");
    auto *hCls = entry->GetClass();
    if (hCls->IsTaggedArray()) {
        TaggedArray *array = TaggedArray::Cast(entry);
        CString arrayName("Array[");
        arrayName.append(ToCString(array->GetLength()));
        arrayName.append("]");
        name = GetString(arrayName);  // String type was handled singly, see#GenerateStringNode
    } else if (hCls->IsHClass()) {
        name = GetString("HiddenClass");
    } else if (hCls->IsJSNativePointer()) {
        name = GetString("JSNativePointer");
    } else {
        if (hCls->IsRealm()) {
            name = GetString("JSRealm");
        } else if (hCls->IsString()) {
            name = GetString("JsString");
        } else if (hCls->IsJSSymbol()) {
            name = GetString("JSSymbol");
        } else if (hCls->IsJSArray()) {
            name = GetString("JSArray");
        } else if (hCls->IsTypedArray()) {
            name = GetString("TypedArray");
        } else if (hCls->IsJSTypedArray()) {
            name = GetString("JSTypedArray");
        } else if (hCls->IsJSInt8Array()) {
            name = GetString("JSInt8Array");
        } else if (hCls->IsJSUint8Array()) {
            name = GetString("JSUint8Array");
        } else if (hCls->IsJSUint8ClampedArray()) {
            name = GetString("JSUint8ClampedArray");
        } else if (hCls->IsJSInt16Array()) {
            name = GetString("JSInt16Array");
        } else if (hCls->IsJSUint16Array()) {
            name = GetString("JSUint16Array");
        } else if (hCls->IsJSInt32Array()) {
            name = GetString("JSInt32Array");
        } else if (hCls->IsJSUint32Array()) {
            name = GetString("JSUint32Array");
        } else if (hCls->IsJSFloat32Array()) {
            name = GetString("JSFloat32Array");
        } else if (hCls->IsJSFloat64Array()) {
            name = GetString("JSFloat64Array");
        } else if (hCls->IsJsGlobalEnv()) {
            name = GetString("JSGlobalEnv");
        } else if (hCls->IsJSFunctionBase()) {
            name = GetString("JSFunctionBase");
        } else if (hCls->IsJsBoundFunction()) {
            name = GetString("JsBoundFunction");
        } else if (hCls->IsJSIntlBoundFunction()) {
            name = GetString("JSIntlBoundFunction");
        } else if (hCls->IsJSProxyRevocFunction()) {
            name = GetString("JSProxyRevocFunction");
        } else if (hCls->IsJSAsyncFunction()) {
            name = GetString("JSAsyncFunction");
        } else if (hCls->IsJSAsyncAwaitStatusFunction()) {
            name = GetString("JSAsyncAwaitStatusFunction");
        } else if (hCls->IsJSPromiseReactionFunction()) {
            name = GetString("JSPromiseReactionFunction");
        } else if (hCls->IsJSPromiseExecutorFunction()) {
            name = GetString("JSPromiseExecutorFuncton");
        } else if (hCls->IsJSPromiseAllResolveElementFunction()) {
            name = GetString("JSPromiseAllResolveElementFunction");
        } else if (hCls->IsJSFunctionExtraInfo()) {
            name = GetString("JSFunctionExtraInfo");
        } else if (hCls->IsMicroJobQueue()) {
            name = GetString("MicroJobQueue");
        } else if (hCls->IsPendingJob()) {
            name = GetString("PendingJob");
        } else if (hCls->IsJsPrimitiveRef()) {
            name = GetString("JsPrimitiveRef");
        } else if (hCls->IsJSSet()) {
            name = GetString("JSSet");
        } else if (hCls->IsJSMap()) {
            name = GetString("JSMap");
        } else if (hCls->IsJSWeakMap()) {
            name = GetString("JSWeakMap");
        } else if (hCls->IsJSWeakSet()) {
            name = GetString("JSWeakSet");
        } else if (hCls->IsJSFunction()) {
            name = GetString("JSFunction");
        } else if (hCls->IsJSError()) {
            name = GetString("JSError");
        } else if (hCls->IsArguments()) {
            name = GetString("Arguments");
        } else if (hCls->IsDate()) {
            name = GetString("Date");
        } else if (hCls->IsJSRegExp()) {
            name = GetString("JSRegExp");
        } else if (hCls->IsJSProxy()) {
            name = GetString("JSProxy");
        } else if (hCls->IsJSLocale()) {
            name = GetString("JSLocale");
        } else if (hCls->IsJSIntl()) {
            name = GetString("JSIntl");
        } else if (hCls->IsJSDateTimeFormat()) {
            name = GetString("JSDateTimeFormat");
        } else if (hCls->IsJSRelativeTimeFormat()) {
            name = GetString("JSRelativeTimeFormat");
        } else if (hCls->IsJSNumberFormat()) {
            name = GetString("JSNumberFormat");
        } else if (hCls->IsAccessorData()) {
            name = GetString("AccessorData");
        } else if (hCls->IsInternalAccessor()) {
            name = GetString("InternalAccessor");
        } else if (hCls->IsIterator()) {
            name = GetString("Iterator");
        } else if (hCls->IsForinIterator()) {
            name = GetString("ForinIterator");
        } else if (hCls->IsStringIterator()) {
            name = GetString("StringIterator");
        } else if (hCls->IsArrayBuffer()) {
            name = GetString("ArrayBuffer");
        } else if (hCls->IsDataView()) {
            name = GetString("DataView");
        } else if (hCls->IsJSSetIterator()) {
            name = GetString("JSSetIterator");
        } else if (hCls->IsJSMapIterator()) {
            name = GetString("JSMapIterator");
        } else if (hCls->IsJSArrayIterator()) {
            name = GetString("JSArrayIterator");
        } else if (hCls->IsPrototypeHandler()) {
            name = GetString("PrototypeHandler");
        } else if (hCls->IsTransitionHandler()) {
            name = GetString("TransitionHandler");
        } else if (hCls->IsPropertyBox()) {
            name = GetString("PropertyBox");
        } else if (hCls->IsProtoChangeMarker()) {
            name = GetString("ProtoChangeMarker");
        } else if (hCls->IsProtoChangeDetails()) {
            name = GetString("ProtoChangeDetails");
        } else if (hCls->IsProgram()) {
            name = GetString("Program");
        } else if (hCls->IsEcmaModule()) {
            name = GetString("EcmaModule");
        } else if (hCls->IsLexicalFunction()) {
            name = GetString("LexicalFunction");
        } else if (hCls->IsConstructor()) {
            name = GetString("Constructor");
        } else if (hCls->IsExtensible()) {
            name = GetString("Extensible");
        } else if (hCls->IsPrototype()) {
            name = GetString("Prototype");
        } else if (hCls->IsLiteral()) {
            name = GetString("Literal");
        } else if (hCls->IsClassConstructor()) {
            name = GetString("ClassConstructor");
        } else if (hCls->IsJSGlobalObject()) {
            name = GetString("JSGlobalObject");
        } else if (hCls->IsClassPrototype()) {
            name = GetString("ClassPrototype");
        } else if (hCls->IsObjectWrapper()) {
            name = GetString("ObjectWrapper");
        } else if (hCls->IsGeneratorFunction()) {
            name = GetString("GeneratorFunction");
        } else if (hCls->IsGeneratorObject()) {
            name = GetString("GeneratorObject");
        } else if (hCls->IsAsyncFuncObject()) {
            name = GetString("AsyncFunction");
        } else if (hCls->IsJSPromise()) {
            name = GetString("JSPromise");
        } else if (hCls->IsResolvingFunctionsRecord()) {
            name = GetString("ResolvingFunctionsRecord");
        } else if (hCls->IsPromiseRecord()) {
            name = GetString("PromiseRecord");
        } else if (hCls->IsPromiseIteratorRecord()) {
            name = GetString("JSPromiseIteratorRecord");
        } else if (hCls->IsPromiseCapability()) {
            name = GetString("PromiseCapability");
        } else if (hCls->IsPromiseReaction()) {
            name = GetString("JSPromiseReaction");
        } else if (hCls->IsCompletionRecord()) {
            name = GetString("CompletionRecord");
        } else if (hCls->IsRecord()) {
            name = GetString("Record");
        } else if (hCls->IsTemplateMap()) {
            name = GetString("TemplateMap");
        } else if (hCls->IsFreeObjectWithOneField()) {
            name = GetString("FreeObjectWithOneField");
        } else if (hCls->IsFreeObjectWithTwoField()) {
            name = GetString("FreeObjectWithTwoField");
        } else if (hCls->IsJSObject()) {
            const GlobalEnvConstants *globalConst = thread->GlobalConstants();
            CString objName = CString("JSOBJECT(Ctor=");  // Ctor-name
            JSTaggedValue proto = JSObject::Cast(entry)->GetPrototype(thread);
            JSHandle<JSTaggedValue> protoHandle(thread, proto);
            if (protoHandle->IsNull() || protoHandle->IsUndefined()) {
                name = GetString("JSObject(Ctor=UnKnown)");
                return name;
            }
            JSHandle<JSTaggedValue> ctor =
                JSObject::GetProperty(thread, protoHandle, globalConst->GetHandledConstructorString()).GetValue();
            if (ctor->IsJSFunction()) {
                JSHandle<JSTaggedValue> nameKey = globalConst->GetHandledNameString();
                JSHandle<JSTaggedValue> value = JSObject::GetProperty(thread, ctor, nameKey).GetValue();
                CString ctorName = EntryVisitor::ConvertKey(value.GetTaggedValue());
                objName.append(ctorName).append(")");
                name = GetString(objName);
            }
        } else if (hCls->IsECMAObject()) {
            name = GetString("ECMAObject");
        } else {
            name = GetString("UnEmuratedJSType");
        }
        return name;  // Cached in String-Table
    }
    return name;
}

NodeType HeapSnapShot::GenerateNodeType(TaggedObject *entry)
{
    NodeType nodeType;
    auto *hCls = entry->GetClass();
    if (hCls->IsTaggedArray()) {
        nodeType = NodeType::JS_ARRAY;
    } else if (hCls->IsHClass()) {
        nodeType = NodeType::PROPERTY_BOX;
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
        if (entry.IsString()) {
            return GenerateStringNode(entry, sequenceId);
        }
        TaggedObject *obj = entry.GetTaggedObject();
        auto *baseClass = obj->GetClass();
        if (baseClass != nullptr) {
            node = Node::NewNode(heap_, sequenceId, nodeCount_, GenerateNodeName(thread, obj), GenerateNodeType(obj),
                                 obj->GetObjectSize(), obj);
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
        auto *obj = reinterpret_cast<TaggedObject *>(entry.GetRawData());
        CString primitiveName;
        JSTaggedValue primitiveObj(obj);
        if (primitiveObj.IsInt()) {
            primitiveName.append("Int:");
        } else if (primitiveObj.IsDouble()) {
            primitiveName.append("Double:");
        } else if (primitiveObj.IsSpecial()) {
            if (primitiveObj.IsHole()) {
                primitiveName.append("Hole");
            } else if (primitiveObj.IsNull()) {
                primitiveName.append("Null");
            } else if (primitiveObj.IsTrue()) {
                primitiveName.append("Boolean:true");
            } else if (primitiveObj.IsFalse()) {
                primitiveName.append("Boolean:false");
            } else if (primitiveObj.IsException()) {
                primitiveName.append("Exception");
            } else if (primitiveObj.IsUndefined()) {
                primitiveName.append("Undefined");
            }
        } else {
            primitiveName.append("Illegal_Primitive");
        }

        node = Node::NewNode(heap_, sequenceId, nodeCount_, GetString(primitiveName), NodeType::JS_PRIMITIVE_REF, 0,
                             obj);
        Node *existNode = entryMap_.FindOrInsertNode(node);  // Fast Index
        if (existNode == node) {
            if (sequenceId == sequenceId_ + SEQ_STEP) {
                sequenceId_ = sequenceId;  // Odd Digit
            }
            InsertNodeUnique(node);
        } else {
            const_cast<RegionFactory *>(heap_->GetRegionFactory())->Delete(node);
            node = nullptr;
            existNode->SetLive(true);
        }
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
        JSTaggedValue(objFrom).DumpForSnapshot(thread, nameResources);
        for (auto const &it : nameResources) {
            auto *to = reinterpret_cast<TaggedObject *>(it.second.GetRawData());
            Node *entryTo = entryMap_.FindEntry(Node::NewAddress(to));
            if (entryTo == nullptr) {
                entryTo = GenerateNode(thread, it.second);
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
    Node *syntheticRoot = Node::NewNode(heap_, 1, nodeCount_, GetString("SyntheticRoot"), NodeType::SYNTHETIC, 0,
                                        nullptr);
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
    int length;
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
    FrontType fType;
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
