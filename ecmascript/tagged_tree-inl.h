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

#ifndef ECMASCRIPT_TAGGED_TREE_INL_H
#define ECMASCRIPT_TAGGED_TREE_INL_H

#include "ecmascript/tagged_tree.h"

#include "ecmascript/global_env.h"
#include "ecmascript/interpreter/interpreter.h"
#include "tagged_array-inl.h"
#include "utils/bit_utils.h"

namespace panda::ecmascript {
// TaggedTree
template<typename Derived>
JSTaggedValue TaggedTree<Derived>::GetRootKey() const
{
    return GetKey(GetRootEntries());
}

template<typename Derived>
void TaggedTree<Derived>::SetRoot(JSThread *thread, int index, JSTaggedValue key, JSTaggedValue value)
{
    SetKey(thread, 0, key);
    SetValue(thread, 0, value);
    SetParent(thread, 0, JSTaggedValue(-1));
    SetColor(thread, 0, TreeColor::BLACK);
    SetRootEntries(thread, index);
}

template<typename Derived>
void TaggedTree<Derived>::SetKey(const JSThread *thread, uint32_t entry, JSTaggedValue key)
{
    int index = EntryToIndex(entry);
    SetElement(thread, index, key);
}

template<typename Derived>
void TaggedTree<Derived>::SetValue(const JSThread *thread, uint32_t entry, JSTaggedValue value)
{
    int index = static_cast<int>(EntryToIndex(entry) + Derived::ENTRY_VALUE_INDEX);
    SetElement(thread, index, value);
}

template<typename Derived>
void TaggedTree<Derived>::SetColor(const JSThread *thread, int entry, TreeColor color)
{
    if (entry >= 0) {
        int index = static_cast<int>(EntryToIndex(entry) + Derived::ENTRY_COLOR_INDEX);
        SetElement(thread, index, JSTaggedValue(static_cast<int>(color)));
    }
}

template<typename Derived>
void TaggedTree<Derived>::SetParent(const JSThread *thread, int entry, JSTaggedValue value)
{
    if (entry < 0) {
        return;
    }
    int index = static_cast<int>(EntryToIndex(entry) + Derived::ENTRY_PARENT_INDEX);
    SetElement(thread, index, value);
}

template<typename Derived>
void TaggedTree<Derived>::SetLeftChild(const JSThread *thread, uint32_t entry, JSTaggedValue value)
{
    int index = static_cast<int>(EntryToIndex(entry) + Derived::ENTRY_LEFT_CHILD_INDEX);
    SetElement(thread, index, value);
}

template<typename Derived>
void TaggedTree<Derived>::SetRightChild(const JSThread *thread, uint32_t entry, JSTaggedValue value)
{
    int index = static_cast<int>(EntryToIndex(entry) + Derived::ENTRY_RIGHT_CHILD_INDEX);
    SetElement(thread, index, value);
}

template<typename Derived>
void TaggedTree<Derived>::SetCompare(const JSThread *thread, JSTaggedValue fn)
{
    Set(thread, COMPARE_FUNCTION_INDEX, fn);
}

template<typename Derived>
JSTaggedValue TaggedTree<Derived>::GetCompare() const
{
    return Get(COMPARE_FUNCTION_INDEX);
}

template<typename Derived>
JSTaggedValue TaggedTree<Derived>::GetKey(int entry) const
{
    if (entry < 0) {
        return JSTaggedValue::Hole();
    }
    int index = EntryToIndex(entry);
    return GetElement(index);
}

template<typename Derived>
JSTaggedValue TaggedTree<Derived>::GetValue(int entry) const
{
    int index = static_cast<int>(EntryToIndex(entry) + Derived::ENTRY_VALUE_INDEX);
    return GetElement(index);
}

template<typename Derived>
TreeColor TaggedTree<Derived>::GetColor(int entry) const
{
    if (entry < 0) {
        return TreeColor::BLACK;
    }
    int index = static_cast<int>(EntryToIndex(entry) + Derived::ENTRY_COLOR_INDEX);
    JSTaggedValue color = GetElement(index);
    return color.GetInt() == TreeColor::RED ? TreeColor::RED : TreeColor::BLACK;
}

template<typename Derived>
int TaggedTree<Derived>::EntryToIndex(uint32_t entry) const
{
    return ELEMENTS_START_INDEX + entry * (Derived::ENTRY_SIZE);
}

template<typename Derived>
void TaggedTree<Derived>::SetElement(const JSThread *thread, uint32_t index, JSTaggedValue element)
{
    ASSERT(index >= 0 && index < GetLength());
    Set(thread, index, element);
}

template<typename Derived>
JSTaggedValue TaggedTree<Derived>::GetElement(int index) const
{
    ASSERT(index >= 0 && index < static_cast<int>(GetLength()));
    return Get(index);
}

template<typename Derived>
int TaggedTree<Derived>::NumberOfElements() const
{
    return Get(NUMBER_OF_ELEMENTS_INDEX).GetInt();
}

template<typename Derived>
int TaggedTree<Derived>::NumberOfDeletedElements() const
{
    return Get(NUMBER_OF_HOLE_ENTRIES_INDEX).GetInt();
}

template<typename Derived>
void TaggedTree<Derived>::SetNumberOfElements(const JSThread *thread, int num)
{
    Set(thread, NUMBER_OF_ELEMENTS_INDEX, JSTaggedValue(num));
}

template<typename Derived>
void TaggedTree<Derived>::SetNumberOfDeletedElements(const JSThread *thread, int num)
{
    Set(thread, NUMBER_OF_HOLE_ENTRIES_INDEX, JSTaggedValue(num));
}

template<typename Derived>
void TaggedTree<Derived>::SetRootEntries(const JSThread *thread, int num)
{
    Set(thread, ROOT_INDEX, JSTaggedValue(num));
}

template<typename Derived>
int TaggedTree<Derived>::GetRootEntries() const
{
    return Get(ROOT_INDEX).GetInt();
}

template<typename Derived>
JSTaggedValue TaggedTree<Derived>::GetLeftChild(int parent) const
{
    if (parent < 0) {
        return JSTaggedValue::Hole();
    }
    int index = static_cast<int>(EntryToIndex(parent) + Derived::ENTRY_LEFT_CHILD_INDEX);
    return Get(index);
}

template<typename Derived>
JSTaggedValue TaggedTree<Derived>::GetRightChild(int parent) const
{
    if (parent < 0) {
        return JSTaggedValue::Hole();
    }
    int index = static_cast<int>(EntryToIndex(parent) + Derived::ENTRY_RIGHT_CHILD_INDEX);
    return Get(index);
}

template<typename Derived>
int TaggedTree<Derived>::GetLeftChildIndex(int parent) const
{
    if (parent < 0) {
        return -1;
    }
    int index = static_cast<int>(EntryToIndex(parent) + Derived::ENTRY_LEFT_CHILD_INDEX);
    JSTaggedValue child = Get(index);
    return child.IsHole() ? -1 : child.GetInt();
}

template<typename Derived>
int TaggedTree<Derived>::GetRightChildIndex(int parent) const
{
    if (parent < 0) {
        return -1;
    }
    int index = static_cast<int>(EntryToIndex(parent) + Derived::ENTRY_RIGHT_CHILD_INDEX);
    JSTaggedValue child = Get(index);
    return child.IsHole() ? -1: child.GetInt();
}

template<typename Derived>
int TaggedTree<Derived>::GetParent(int entry) const
{
    int index = static_cast<int>(EntryToIndex(entry) + Derived::ENTRY_PARENT_INDEX);
    JSTaggedValue parent = GetElement(index);
    return parent.GetInt();
}

template<typename Derived>
int TaggedTree<Derived>::GetMinimum(int entry) const
{
    JSTaggedValue child = GetLeftChild(entry);
    while (!child.IsHole()) {
        entry = child.GetInt();
        child = GetLeftChild(entry);
    }
    return entry;
}

template<typename Derived>
int TaggedTree<Derived>::GetMaximum(int entry) const
{
    JSTaggedValue child = GetRightChild(entry);
    while (!child.IsHole()) {
        entry = child.GetInt();
        child = GetRightChild(entry);
    }
    return entry;
}

template<typename Derived>
bool TaggedTree<Derived>::IsLeft(int entry) const
{
    JSTaggedValue child = GetLeftChild(GetParent(entry));
    return child.IsHole() ? false : (child.GetInt() == entry);
}

template<typename Derived>
bool TaggedTree<Derived>::IsRight(int entry) const
{
    JSTaggedValue child = GetRightChild(GetParent(entry));
    return child.IsHole() ? false : (child.GetInt() == entry);
}

template<typename Derived>
bool TaggedTree<Derived>::IsValidIndex(int entry) const
{
    return entry != GetRootEntries() && !GetKey(entry).IsHole();
}

template<typename Derived>
int TaggedTree<Derived>::GetLeftBrother(int entry) const
{
    JSTaggedValue child = GetRightChild(GetParent(entry));
    return child.IsHole() ? -1 : child.GetInt();
}

template<typename Derived>
int TaggedTree<Derived>::GetRightBrother(int entry) const
{
    JSTaggedValue child = GetLeftChild(GetParent(entry));
    return child.IsHole() ? -1 : child.GetInt();
}

template<typename Derived>
void TaggedTree<Derived>::SetCapacity(const JSThread *thread, int capacity)
{
    Set(thread, CAPACITY_INDEX, JSTaggedValue(capacity));
}

template<typename Derived>
int TaggedTree<Derived>::Capacity() const
{
    return Get(CAPACITY_INDEX).GetInt();
}

template<typename Derived>
int TaggedTree<Derived>::ComputeCapacity(int oldCapacity)
{
    int capacity = (static_cast<uint32_t>(oldCapacity) << 1) + 1;
    return (capacity > MIN_CAPACITY) ? capacity : MIN_CAPACITY;
}

template<typename Derived>
void TaggedTree<Derived>::InsertLeftEntry(const JSThread *thread, uint32_t parentIndex, uint32_t entry,
                                          JSTaggedValue key, JSTaggedValue value)
{
    SetKey(thread, entry, key);
    SetValue(thread, entry, value);
    SetColor(thread, entry, TreeColor::RED);
    SetParent(thread, entry, JSTaggedValue(parentIndex));
    SetLeftChild(thread, parentIndex, JSTaggedValue(entry));
}

template<typename Derived>
void TaggedTree<Derived>::InsertRightEntry(const JSThread *thread, uint32_t parentIndex, uint32_t entry,
                                           JSTaggedValue key, JSTaggedValue value)
{
    SetKey(thread, entry, key);
    SetValue(thread, entry, value);
    SetColor(thread, entry, TreeColor::RED);
    SetParent(thread, entry, JSTaggedValue(parentIndex));
    SetRightChild(thread, parentIndex, JSTaggedValue(entry));
}

template<typename Derived>
void TaggedTree<Derived>::CopyEntry(const JSThread *thread, int parent, const JSHandle<Derived> &newTree, int index)
{
    newTree->SetKey(thread, index, GetKey(parent));
    newTree->SetColor(thread, index, GetColor(parent));
}

template<typename Derived>
void TaggedTree<Derived>::CopyData(const JSThread *thread, int dst, int src)
{
    SetKey(thread, dst, GetKey(src));
}

template<typename Derived>
void TaggedTree<Derived>::CopyAllData(const JSThread *thread, int parent, const JSHandle<Derived> &newTree, int index)
{
    newTree->SetKey(thread, index, GetKey(parent));
    newTree->SetValue(thread, index, GetValue(parent));
    newTree->SetColor(thread, index, GetColor(parent));
    newTree->SetParent(thread, index, JSTaggedValue(GetParent(parent)));
    newTree->SetRightChild(thread, index, GetRightChild(parent));
    newTree->SetLeftChild(thread, index, GetLeftChild(parent));
}

template<typename Derived>
void TaggedTree<Derived>::RemoveEntry(const JSThread *thread, int index)
{
    SetKey(thread, index, JSTaggedValue::Hole());
    SetParent(thread, index, JSTaggedValue::Hole());
    SetLeftChild(thread, index, JSTaggedValue::Hole());
    SetRightChild(thread, index, JSTaggedValue::Hole());
}

template<typename Derived>
ComparisonResult TaggedTree<Derived>::EntryCompare(JSThread *thread, const JSHandle<JSTaggedValue> valueX,
                                                   const JSHandle<JSTaggedValue> valueY, JSHandle<Derived> tree)
{
    JSTaggedValue fn = tree->GetCompare();
    if (fn.IsHole()) {
        return OrdinayEntryCompare(thread, valueX, valueY);
    }

    JSHandle<JSTaggedValue> compareFn(thread, fn);
    JSHandle<JSTaggedValue> thisArgHandle = thread->GlobalConstants()->GetHandledUndefined();
    const size_t argsLength = 2;
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo info =
        EcmaInterpreter::NewRuntimeCallInfo(thread, compareFn, thisArgHandle, undefined, argsLength);
    info.SetCallArg(valueX.GetTaggedValue(), valueY.GetTaggedValue());
    JSTaggedValue callResult = JSFunction::Call(&info);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, ComparisonResult::UNDEFINED);
    int compareResult = 0;
    if (callResult.IsInt()) {
        compareResult = callResult.GetInt();
    } else {
        JSHandle<JSTaggedValue> resultHandle(thread, callResult);
        JSTaggedNumber v = JSTaggedValue::ToNumber(thread, resultHandle);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, ComparisonResult::UNDEFINED);
        double value = v.GetNumber();
        if (std::isnan(value)) {
            THROW_TYPE_ERROR_AND_RETURN(thread, "CompareFn has illegal return value", ComparisonResult::UNDEFINED);
        }
        compareResult = static_cast<int>(value);
    }
    return compareResult > 0 ? ComparisonResult::GREAT :
                               (compareResult < 0 ? ComparisonResult::LESS : ComparisonResult::EQUAL);
}

template<typename Derived>
ComparisonResult TaggedTree<Derived>::OrdinayEntryCompare(JSThread *thread, const JSHandle<JSTaggedValue> valueX,
                                                          const JSHandle<JSTaggedValue> valueY)
{
    if (valueX->IsString() && valueY->IsString()) {
        auto xString = static_cast<EcmaString *>(valueX->GetTaggedObject());
        auto yString = static_cast<EcmaString *>(valueY->GetTaggedObject());
        int result = xString->Compare(yString);
        if (result < 0) {
            return ComparisonResult::LESS;
        }
        if (result == 0) {
            return ComparisonResult::EQUAL;
        }
        return ComparisonResult::GREAT;
    }

    if (valueX->IsNumber() && valueY->IsNumber()) {
        return JSTaggedValue::StrictNumberCompare(valueX->GetNumber(), valueY->GetNumber());
    }

    if (valueX->IsNumber() && valueY->IsString()) {
        return ComparisonResult::LESS;
    }
    if (valueX->IsString() && valueY->IsNumber()) {
        return ComparisonResult::GREAT;
    }

    JSHandle<JSTaggedValue> xValueHandle(JSTaggedValue::ToString(thread, valueX));
    JSHandle<JSTaggedValue> yValueHandle(JSTaggedValue::ToString(thread, valueY));
    ASSERT_NO_ABRUPT_COMPLETION(thread);
    return JSTaggedValue::Compare(thread, xValueHandle, yValueHandle);
}

template<typename Derived>
JSTaggedValue TaggedTree<Derived>::Transform(JSTaggedValue v) const
{
    return v.IsHole() ? JSTaggedValue::Undefined() : v;
}

// TaggedTreeMap
void TaggedTreeMap::CopyEntry(const JSThread *thread, int parent, const JSHandle<TaggedTreeMap> &newMap, int index)
{
    RBTree::CopyEntry(thread, parent, newMap, index);
    newMap->SetValue(thread, index, GetValue(parent));
}

void TaggedTreeMap::CopyData(const JSThread *thread, int dst, int src)
{
    RBTree::CopyData(thread, dst, src);
    SetValue(thread, dst, GetValue(src));
}

void TaggedTreeMap::RemoveEntry(const JSThread *thread, int index)
{
    RBTree::RemoveEntry(thread, index);
    SetValue(thread, index, JSTaggedValue::Hole());
}

JSTaggedValue TaggedTreeMap::Get(JSThread *thread, const JSHandle<TaggedTreeMap> &map,
                                 const JSHandle<JSTaggedValue> &key)
{
    int index = RBTree::FindEntry(thread, map, key);
    return index == -1 ? JSTaggedValue::Undefined() : map->GetValue(index);
}

JSTaggedValue TaggedTreeMap::GetFirstKey() const
{
    JSTaggedValue key = GetKey(GetMinimum(GetRootEntries()));
    return Transform(key);
}

JSTaggedValue TaggedTreeMap::GetLastKey() const
{
    JSTaggedValue key = GetKey(GetMaximum(GetRootEntries()));
    return Transform(key);
}

// TaggedTreeSet
void TaggedTreeSet::CopyEntry(const JSThread *thread, int parent, const JSHandle<TaggedTreeSet> &newMap, int index)
{
    RBTree::CopyEntry(thread, parent, newMap, index);
    newMap->SetValue(thread, index, GetValue(parent));
}

void TaggedTreeSet::CopyData(const JSThread *thread, int dst, int src)
{
    RBTree::CopyData(thread, dst, src);
    SetValue(thread, dst, GetValue(src));
}

void TaggedTreeSet::RemoveEntry(const JSThread *thread, int index)
{
    RBTree::RemoveEntry(thread, index);
    SetValue(thread, index, JSTaggedValue::Hole());
}

JSTaggedValue TaggedTreeSet::GetFirstKey() const
{
    JSTaggedValue key = GetKey(GetMinimum(GetRootEntries()));
    return Transform(key);
}

JSTaggedValue TaggedTreeSet::GetLastKey() const
{
    JSTaggedValue key = GetKey(GetMaximum(GetRootEntries()));
    return Transform(key);
}
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_TAGGED_TREE_INL_H
