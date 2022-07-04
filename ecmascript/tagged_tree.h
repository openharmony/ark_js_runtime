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

#ifndef ECMASCRIPT_TAGGED_TREE_H
#define ECMASCRIPT_TAGGED_TREE_H

#include "ecmascript/global_env.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_tagged_value.h"
#include "js_handle.h"
#include "tagged_array.h"
#include "utils/bit_utils.h"

namespace panda::ecmascript {
enum TreeColor : uint8_t { BLACK = 0, RED };
/**
 * The tree layout is as follows:
 * 1.array[0-4] is used to store common information, such as:
 * +------------------------+-----------------------------+------------------------+------------+------------------+
 * | the number of elements | the number of hole elements | the number of capacity | root index | compare function |
 * +------------------------+-----------------------------+------------------------+------------+------------------+
 * 2.array[5,5+capacity] is used to store node of tree, map and set store nodes in different formats respectively.
 * */
template<typename Derived>
class TaggedTree : public TaggedArray {
public:
    static constexpr int MIN_CAPACITY = 7;
    static constexpr int NUMBER_OF_ELEMENTS_INDEX = 0;
    static constexpr int NUMBER_OF_HOLE_ENTRIES_INDEX = 1;
    static constexpr int CAPACITY_INDEX = 2;
    static constexpr int ROOT_INDEX = 3;
    static constexpr int COMPARE_FUNCTION_INDEX = 4;
    static constexpr int ELEMENTS_START_INDEX = 5;
    static constexpr int MIN_SHRINK_CAPACITY = 15;

    static JSHandle<Derived> Create(const JSThread *thread, int numberOfElements);

    static JSHandle<Derived> Insert(JSThread *thread, JSHandle<Derived> &tree, const JSHandle<JSTaggedValue> &key,
                                    const JSHandle<JSTaggedValue> &value);

    static JSHandle<Derived> GrowCapacity(const JSThread *thread, JSHandle<Derived> &tree);

    inline static int ComputeCapacity(int oldCapacity)
    {
        int capacity = (static_cast<uint32_t>(oldCapacity) << 1) + 1;
        return (capacity > MIN_CAPACITY) ? capacity : MIN_CAPACITY;
    }

    static void Remove(const JSThread *thread, const JSHandle<Derived> &tree, int entry);

    inline int NumberOfElements() const
    {
        return Get(NUMBER_OF_ELEMENTS_INDEX).GetInt();
    }

    inline int NumberOfDeletedElements() const
    {
        return Get(NUMBER_OF_HOLE_ENTRIES_INDEX).GetInt();
    }

    inline int Capacity() const
    {
        return Get(CAPACITY_INDEX).GetInt();
    }

    inline JSTaggedValue GetKey(int entry) const
    {
        if (entry < 0) {
            return JSTaggedValue::Hole();
        }
        int index = EntryToIndex(entry);
        return GetElement(index);
    }

    inline JSTaggedValue GetValue(int entry) const
    {
        int index = static_cast<int>(EntryToIndex(entry) + Derived::ENTRY_VALUE_INDEX);
        return GetElement(index);
    }

    inline TreeColor GetColor(int entry) const
    {
        if (entry < 0) {
            return TreeColor::BLACK;
        }
        int index = static_cast<int>(EntryToIndex(entry) + Derived::ENTRY_COLOR_INDEX);
        JSTaggedValue color = GetElement(index);
        return color.GetInt() == TreeColor::RED ? TreeColor::RED : TreeColor::BLACK;
    }

    inline void SetCapacity(const JSThread *thread, int capacity)
    {
        Set(thread, CAPACITY_INDEX, JSTaggedValue(capacity));
    }

    inline static bool IsKey(JSTaggedValue key)
    {
        return !key.IsHole();
    }

    inline void SetNumberOfElements(const JSThread *thread, int num)
    {
        Set(thread, NUMBER_OF_ELEMENTS_INDEX, JSTaggedValue(num));
    }
    
    inline void SetNumberOfDeletedElements(const JSThread *thread, int num)
    {
        Set(thread, NUMBER_OF_HOLE_ENTRIES_INDEX, JSTaggedValue(num));
    }

    inline void SetRootEntries(const JSThread *thread, int num)
    {
        Set(thread, ROOT_INDEX, JSTaggedValue(num));
    }

    inline int GetRootEntries() const
    {
        return Get(ROOT_INDEX).GetInt();
    }

    static int FindEntry(JSThread *thread, const JSHandle<Derived> &tree, const JSHandle<JSTaggedValue> &key);

    inline static ComparisonResult EntryCompare(JSThread *thread, const JSHandle<JSTaggedValue> valueX,
                                                const JSHandle<JSTaggedValue> valueY, JSHandle<Derived> tree)
    {
        JSTaggedValue fn = tree->GetCompare();
        if (fn.IsHole()) {
            return OrdinayEntryCompare(thread, valueX, valueY);
        }

        JSHandle<JSTaggedValue> compareFn(thread, fn);
        JSHandle<JSTaggedValue> thisArgHandle = thread->GlobalConstants()->GetHandledUndefined();
        const int32_t argsLength = 2;
        JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
        EcmaRuntimeCallInfo *info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, compareFn, thisArgHandle, undefined, argsLength);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, ComparisonResult::UNDEFINED);
        info->SetCallArg(valueX.GetTaggedValue(), valueY.GetTaggedValue());
        JSTaggedValue callResult = JSFunction::Call(info);
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

    inline void SetKey(const JSThread *thread, uint32_t entry, JSTaggedValue key)
    {
        int index = EntryToIndex(entry);
        SetElement(thread, index, key);
    }

    inline void SetValue(const JSThread *thread, uint32_t entry, JSTaggedValue value)
    {
        int index = static_cast<int>(EntryToIndex(entry) + Derived::ENTRY_VALUE_INDEX);
        SetElement(thread, index, value);
    }

    inline void SetCompare(const JSThread *thread, JSTaggedValue fn)
    {
        Set(thread, COMPARE_FUNCTION_INDEX, fn);
    }

    inline JSTaggedValue GetCompare() const
    {
        return Get(COMPARE_FUNCTION_INDEX);
    }

    inline int GetMinimum(int entry) const
    {
        JSTaggedValue child = GetLeftChild(entry);
        while (!child.IsHole()) {
            entry = child.GetInt();
            child = GetLeftChild(entry);
        }
        return entry;
    }

    inline int GetMaximum(int entry) const
    {
        JSTaggedValue child = GetRightChild(entry);
        while (!child.IsHole()) {
            entry = child.GetInt();
            child = GetRightChild(entry);
        }
        return entry;
    }

    inline int GetParent(int entry) const
    {
        int index = static_cast<int>(EntryToIndex(entry) + Derived::ENTRY_PARENT_INDEX);
        JSTaggedValue parent = GetElement(index);
        return parent.GetInt();
    }

    inline JSTaggedValue GetLeftChild(int parent) const
    {
        if (parent < 0) {
            return JSTaggedValue::Hole();
        }
        int index = static_cast<int>(EntryToIndex(parent) + Derived::ENTRY_LEFT_CHILD_INDEX);
        return Get(index);
    }

    inline JSTaggedValue GetRightChild(int parent) const
    {
        if (parent < 0) {
            return JSTaggedValue::Hole();
        }
        int index = static_cast<int>(EntryToIndex(parent) + Derived::ENTRY_RIGHT_CHILD_INDEX);
        return Get(index);
    }

    inline int GetLeftChildIndex(int parent) const
    {
        if (parent < 0) {
            return -1;
        }
        int index = static_cast<int>(EntryToIndex(parent) + Derived::ENTRY_LEFT_CHILD_INDEX);
        JSTaggedValue child = Get(index);
        return child.IsHole() ? -1 : child.GetInt();
    }

    inline int GetRightChildIndex(int parent) const
    {
        if (parent < 0) {
            return -1;
        }
        int index = static_cast<int>(EntryToIndex(parent) + Derived::ENTRY_RIGHT_CHILD_INDEX);
        JSTaggedValue child = Get(index);
        return child.IsHole() ? -1: child.GetInt();
    }

protected:
    inline JSTaggedValue GetElement(int index) const
    {
        ASSERT(index >= 0 && index < static_cast<int>(GetLength()));
        return Get(index);
    }

    // get root
    inline JSTaggedValue GetRootKey() const
    {
        return GetKey(GetRootEntries());
    }

    inline int EntryToIndex(uint32_t entry) const
    {
        return ELEMENTS_START_INDEX + entry * (Derived::ENTRY_SIZE);
    }

    inline void SetElement(const JSThread *thread, uint32_t index, JSTaggedValue element)
    {
        ASSERT(index >= 0 && index < GetLength());
        Set(thread, index, element);
    }

    inline void SetParent(const JSThread *thread, int entry, JSTaggedValue value)
    {
        if (entry < 0) {
            return;
        }
        int index = static_cast<int>(EntryToIndex(entry) + Derived::ENTRY_PARENT_INDEX);
        SetElement(thread, index, value);
    }

    inline void SetRoot(JSThread *thread, int index, JSTaggedValue key, JSTaggedValue value)
    {
        SetKey(thread, 0, key);
        SetValue(thread, 0, value);
        SetParent(thread, 0, JSTaggedValue(-1));
        SetColor(thread, 0, TreeColor::BLACK);
        SetRootEntries(thread, index);
    }

    inline void SetColor(const JSThread *thread, int entry, TreeColor color)
    {
        if (entry >= 0) {
            int index = static_cast<int>(EntryToIndex(entry) + Derived::ENTRY_COLOR_INDEX);
            SetElement(thread, index, JSTaggedValue(static_cast<int>(color)));
        }
    }

    inline void InsertLeftEntry(const JSThread *thread, uint32_t parentIndex, uint32_t entry, JSTaggedValue key,
                                JSTaggedValue value)
    {
        SetKey(thread, entry, key);
        SetValue(thread, entry, value);
        SetColor(thread, entry, TreeColor::RED);
        SetParent(thread, entry, JSTaggedValue(parentIndex));
        SetLeftChild(thread, parentIndex, JSTaggedValue(entry));
    }

    inline void InsertRightEntry(const JSThread *thread, uint32_t parentIndex, uint32_t entry, JSTaggedValue key,
                                 JSTaggedValue value)
    {
        SetKey(thread, entry, key);
        SetValue(thread, entry, value);
        SetColor(thread, entry, TreeColor::RED);
        SetParent(thread, entry, JSTaggedValue(parentIndex));
        SetRightChild(thread, parentIndex, JSTaggedValue(entry));
    }

    void InsertRebalance(const JSThread *thread, int index);
    void DeleteRebalance(const JSThread *thread, int index);

    inline bool IsValidIndex(int entry) const
    {
        return entry != GetRootEntries() && !GetKey(entry).IsHole();
    }

    inline int GetLeftBrother(int entry) const
    {
        JSTaggedValue child = GetRightChild(GetParent(entry));
        return child.IsHole() ? -1 : child.GetInt();
    }

    inline int GetRightBrother(int entry) const
    {
        JSTaggedValue child = GetLeftChild(GetParent(entry));
        return child.IsHole() ? -1 : child.GetInt();
    }

    inline bool IsLeft(int entry) const
    {
        JSTaggedValue child = GetLeftChild(GetParent(entry));
        return child.IsHole() ? false : (child.GetInt() == entry);
    }

    inline bool IsRight(int entry) const
    {
        JSTaggedValue child = GetRightChild(GetParent(entry));
        return child.IsHole() ? false : (child.GetInt() == entry);
    }

    int GetPreDecessor(int entry) const;
    int GetSuccessor(int entry) const;

    inline void SetLeftChild(const JSThread *thread, uint32_t entry, JSTaggedValue value)
    {
        int index = static_cast<int>(EntryToIndex(entry) + Derived::ENTRY_LEFT_CHILD_INDEX);
        SetElement(thread, index, value);
    }
    inline void SetRightChild(const JSThread *thread, uint32_t entry, JSTaggedValue value)
    {
        int index = static_cast<int>(EntryToIndex(entry) + Derived::ENTRY_RIGHT_CHILD_INDEX);
        SetElement(thread, index, value);
    }

    void LeftRotate(const JSThread *thread, int index);
    void RightRotate(const JSThread *thread, int index);

    static JSHandle<Derived> AdjustTaggedTree(const JSThread *thread, const JSHandle<Derived> &tree, int len);
    inline static ComparisonResult OrdinayEntryCompare(JSThread *thread, const JSHandle<JSTaggedValue> valueX,
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

    inline void CopyEntry(const JSThread *thread, int parent, const JSHandle<Derived> &newTree, int index)
    {
        newTree->SetKey(thread, index, GetKey(parent));
        newTree->SetColor(thread, index, GetColor(parent));
    }

    inline void CopyData(const JSThread *thread, int dst, int src)
    {
        SetKey(thread, dst, GetKey(src));
    }
    inline void CopyAllData(const JSThread *thread, int parent, const JSHandle<Derived> &newTree, int index)
    {
        newTree->SetKey(thread, index, GetKey(parent));
        newTree->SetValue(thread, index, GetValue(parent));
        newTree->SetColor(thread, index, GetColor(parent));
        newTree->SetParent(thread, index, JSTaggedValue(GetParent(parent)));
        newTree->SetRightChild(thread, index, GetRightChild(parent));
        newTree->SetLeftChild(thread, index, GetLeftChild(parent));
    }

    inline void RemoveEntry(const JSThread *thread, int index)
    {
        SetKey(thread, index, JSTaggedValue::Hole());
        SetParent(thread, index, JSTaggedValue::Hole());
        SetLeftChild(thread, index, JSTaggedValue::Hole());
        SetRightChild(thread, index, JSTaggedValue::Hole());
    }

    inline JSTaggedValue Transform(JSTaggedValue v) const
    {
        return v.IsHole() ? JSTaggedValue::Undefined() : v;
    }

    void Transplant(const JSThread *thread, int dst, int src);

    static JSTaggedValue GetLowerKey(JSThread *thread, const JSHandle<Derived> &tree,
                                     const JSHandle<JSTaggedValue> &key);
    static JSTaggedValue GetHigherKey(JSThread *thread, const JSHandle<Derived> &tree,
                                      const JSHandle<JSTaggedValue> &key);
    static JSHandle<TaggedArray> GetSortArray(const JSThread *thread, const JSHandle<Derived> &tree);
    static JSHandle<Derived> Shrink(const JSThread *thread, const JSHandle<Derived> &tree);
};


class TaggedTreeMap : public TaggedTree<TaggedTreeMap> {
public:
    using RBTree = TaggedTree<TaggedTreeMap>;
    static TaggedTreeMap *Cast(TaggedObject *obj)
    {
        return static_cast<TaggedTreeMap *>(obj);
    }

    static JSTaggedValue Create(const JSThread *thread, int numberOfElements = MIN_CAPACITY);
    static JSTaggedValue Set(JSThread *thread, JSHandle<TaggedTreeMap> &obj,
                                    const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &value);
    inline static JSTaggedValue Get(JSThread *thread, const JSHandle<TaggedTreeMap> &map,
                                    const JSHandle<JSTaggedValue> &key)
    {
        int index = RBTree::FindEntry(thread, map, key);
        return index == -1 ? JSTaggedValue::Undefined() : map->GetValue(index);
    }

    static JSTaggedValue Delete(JSThread *thread, const JSHandle<TaggedTreeMap> &map, int entry);
    bool HasValue(const JSThread *thread, JSTaggedValue value) const;

    static JSTaggedValue GetLowerKey(JSThread *thread, const JSHandle<TaggedTreeMap> &map,
                                     const JSHandle<JSTaggedValue> &key);
    static JSTaggedValue GetHigherKey(JSThread *thread, const JSHandle<TaggedTreeMap> &map,
                                      const JSHandle<JSTaggedValue> &key);

    inline JSTaggedValue GetFirstKey() const
    {
        JSTaggedValue key = GetKey(GetMinimum(GetRootEntries()));
        return Transform(key);
    }

    inline JSTaggedValue GetLastKey() const
    {
        JSTaggedValue key = GetKey(GetMaximum(GetRootEntries()));
        return Transform(key);
    }

    static JSTaggedValue SetAll(JSThread *thread, JSHandle<TaggedTreeMap> &dst, const JSHandle<TaggedTreeMap> &src);
    static JSHandle<TaggedArray> GetArrayFromMap(const JSThread *thread, const JSHandle<TaggedTreeMap> &map);
    static int FindEntry(JSThread *thread, const JSHandle<TaggedTreeMap> &map, const JSHandle<JSTaggedValue> &key);

    static const int ENTRY_SIZE = 6;
    static const int ENTRY_VALUE_INDEX = 1;
    static const int ENTRY_COLOR_INDEX = 2;
    static const int ENTRY_PARENT_INDEX = 3;
    static const int ENTRY_LEFT_CHILD_INDEX = 4;
    static const int ENTRY_RIGHT_CHILD_INDEX = 5;
    DECL_DUMP()

    inline void CopyEntry(const JSThread *thread, int parent, const JSHandle<TaggedTreeMap> &newMap, int index)
    {
        RBTree::CopyEntry(thread, parent, newMap, index);
        newMap->SetValue(thread, index, GetValue(parent));
    }
    inline void CopyData(const JSThread *thread, int dst, int src)
    {
        RBTree::CopyData(thread, dst, src);
        SetValue(thread, dst, GetValue(src));
    }

    inline void RemoveEntry(const JSThread *thread, int index)
    {
        RBTree::RemoveEntry(thread, index);
        SetValue(thread, index, JSTaggedValue::Hole());
    }
};

class TaggedTreeSet : public TaggedTree<TaggedTreeSet> {
public:
    using RBTree = TaggedTree<TaggedTreeSet>;
    static TaggedTreeSet *Cast(TaggedObject *obj)
    {
        return static_cast<TaggedTreeSet *>(obj);
    }

    static JSTaggedValue Create(const JSThread *thread, int numberOfElements = MIN_CAPACITY);
    static JSTaggedValue Add(JSThread *thread, JSHandle<TaggedTreeSet> &obj, const JSHandle<JSTaggedValue> &value);
    static JSTaggedValue Delete(JSThread *thread, const JSHandle<TaggedTreeSet> &set, int entry);

    static JSTaggedValue GetLowerKey(JSThread *thread, const JSHandle<TaggedTreeSet> &set,
                                     const JSHandle<JSTaggedValue> &key);
    static JSTaggedValue GetHigherKey(JSThread *thread, const JSHandle<TaggedTreeSet> &set,
                                      const JSHandle<JSTaggedValue> &key);

    inline JSTaggedValue GetFirstKey() const
    {
        JSTaggedValue key = GetKey(GetMinimum(GetRootEntries()));
        return Transform(key);
    }

    inline JSTaggedValue GetLastKey() const
    {
        JSTaggedValue key = GetKey(GetMaximum(GetRootEntries()));
        return Transform(key);
    }

    static JSHandle<TaggedArray> GetArrayFromSet(const JSThread *thread, const JSHandle<TaggedTreeSet> &set);
    static int FindEntry(JSThread *thread, const JSHandle<TaggedTreeSet> &set, const JSHandle<JSTaggedValue> &key);

    static const int ENTRY_SIZE = 5;
    static const int ENTRY_VALUE_INDEX = 0;
    static const int ENTRY_COLOR_INDEX = 1;
    static const int ENTRY_PARENT_INDEX = 2;
    static const int ENTRY_LEFT_CHILD_INDEX = 3;
    static const int ENTRY_RIGHT_CHILD_INDEX = 4;

    DECL_DUMP()

    inline void CopyEntry(const JSThread *thread, int parent, const JSHandle<TaggedTreeSet> &newMap, int index)
    {
        RBTree::CopyEntry(thread, parent, newMap, index);
        newMap->SetValue(thread, index, GetValue(parent));
    }

    inline void CopyData(const JSThread *thread, int dst, int src)
    {
        RBTree::CopyData(thread, dst, src);
        SetValue(thread, dst, GetValue(src));
    }

    inline void RemoveEntry(const JSThread *thread, int index)
    {
        RBTree::RemoveEntry(thread, index);
        SetValue(thread, index, JSTaggedValue::Hole());
    }
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_TAGGED_TREE_H
