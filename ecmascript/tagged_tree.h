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

#ifndef ECMASCRIPT_TAGGED_TREE_H
#define ECMASCRIPT_TAGGED_TREE_H

#include "ecmascript/js_tagged_value.h"
#include "js_handle.h"
#include "tagged_array.h"

namespace panda::ecmascript {
enum TreeColor : uint8_t { BLACK = 0, RED };
/**
 * The tree layout is as follows:
 * 1.array[0-4] is used to store common information, sush as:
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

    inline static int ComputeCapacity(int oldCapacity);

    static void Remove(const JSThread *thread, const JSHandle<Derived> &tree, int entry);

    inline int NumberOfElements() const;
    inline int NumberOfDeletedElements() const;

    inline int Capacity() const;

    inline JSTaggedValue GetKey(int entry) const;
    inline JSTaggedValue GetValue(int entry) const;

    inline TreeColor GetColor(int entry) const;

    inline void SetCapacity(const JSThread *thread, int capacity);

    inline static bool IsKey(JSTaggedValue key)
    {
        return !key.IsHole();
    }

    inline void SetNumberOfElements(const JSThread *thread, int num);
    inline void SetNumberOfDeletedElements(const JSThread *thread, int num);

    inline void SetRootEntries(const JSThread *thread, int num);
    inline int GetRootEntries() const;

    static int FindEntry(JSThread *thread, const JSHandle<Derived> &tree, const JSHandle<JSTaggedValue> &key);

    inline static ComparisonResult EntryCompare(JSThread *thread, const JSHandle<JSTaggedValue> valueX,
                                                const JSHandle<JSTaggedValue> valueY, JSHandle<Derived> tree);

    inline void SetKey(const JSThread *thread, uint32_t entry, JSTaggedValue key);
    inline void SetValue(const JSThread *thread, uint32_t entry, JSTaggedValue value);
    inline void SetCompare(const JSThread *thread, JSTaggedValue fn);
    inline JSTaggedValue GetCompare() const;

    inline int GetMinimum(int entry) const;
    inline int GetMaximum(int entry) const;

    inline int GetParent(int entry) const;
    inline JSTaggedValue GetLeftChild(int parent) const;
    inline JSTaggedValue GetRightChild(int parent) const;
    inline int GetLeftChildIndex(int parent) const;
    inline int GetRightChildIndex(int parent) const;

protected:
    inline JSTaggedValue GetElement(int index) const;

    // get root
    inline JSTaggedValue GetRootKey() const;

    inline void SetRoot(JSThread *thread, int index, JSTaggedValue key, JSTaggedValue value);

    inline void SetElement(const JSThread *thread, uint32_t index, JSTaggedValue element);
    inline void SetColor(const JSThread *thread, int entry, TreeColor color);
    inline void SetParent(const JSThread *thread, int entry, JSTaggedValue value);

    inline uint32_t EntryToIndex(uint32_t entry) const;
    inline void InsertLeftEntry(const JSThread *thread, uint32_t parentIndex, uint32_t entry, JSTaggedValue key,
                                JSTaggedValue value);
    inline void InsertRightEntry(const JSThread *thread, uint32_t parentIndex, uint32_t entry, JSTaggedValue key,
                                 JSTaggedValue value);

    void InsertRebalance(const JSThread *thread, int index);
    void DeleteRebalance(const JSThread *thread, int index);
    inline bool IsVaildIndex(int entry) const;

    inline int GetLeftBrother(int entry) const;
    inline int GetRightBrother(int entry) const;

    inline bool IsLeft(int entry) const;
    inline bool IsRight(int entry) const;

    int GetPreDecessor(int entry) const;
    int GetSuccessor(int entry) const;

    inline void SetLeftChild(const JSThread *thread, uint32_t entry, JSTaggedValue value);
    inline void SetRightChild(const JSThread *thread, uint32_t entry, JSTaggedValue value);

    void LeftRotate(const JSThread *thread, int index);
    void RightRotate(const JSThread *thread, int index);

    static JSHandle<Derived> AdjustTaggedTree(const JSThread *thread, const JSHandle<Derived> &tree, int len);
    inline static ComparisonResult OrdinayEntryCompare(JSThread *thread, const JSHandle<JSTaggedValue> valueX,
                                                       const JSHandle<JSTaggedValue> valueY);

    inline void CopyEntry(const JSThread *thread, int parent, const JSHandle<Derived> &newTree, int index);
    inline void CopyData(const JSThread *thread, int dst, int src);
    inline void CopyAllData(const JSThread *thread, int parent, const JSHandle<Derived> &newTree, int index);

    inline void RemoveEntry(const JSThread *thread, int index);

    inline JSTaggedValue Transform(JSTaggedValue v) const;
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
    static TaggedTreeMap *Cast(ObjectHeader *obj)
    {
        return static_cast<TaggedTreeMap *>(obj);
    }

    static JSTaggedValue Create(const JSThread *thread, int numberOfElements = MIN_CAPACITY);
    static JSTaggedValue Set(JSThread *thread, JSHandle<TaggedTreeMap> &obj,
                                    const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &value);
    inline static JSTaggedValue Get(JSThread *thread, const JSHandle<TaggedTreeMap> &map,
                                    const JSHandle<JSTaggedValue> &key);
    static JSTaggedValue Delete(JSThread *thread, const JSHandle<TaggedTreeMap> &map, int entry);
    bool HasValue(const JSThread *thread, JSTaggedValue value) const;

    static JSTaggedValue GetLowerKey(JSThread *thread, const JSHandle<TaggedTreeMap> &map,
                                     const JSHandle<JSTaggedValue> &key);
    static JSTaggedValue GetHigherKey(JSThread *thread, const JSHandle<TaggedTreeMap> &map,
                                      const JSHandle<JSTaggedValue> &key);

    inline JSTaggedValue GetFirstKey() const;
    inline JSTaggedValue GetLastKey() const;

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

    inline void CopyEntry(const JSThread *thread, int parent, const JSHandle<TaggedTreeMap> &newTree, int index);
    inline void CopyData(const JSThread *thread, int dst, int src);
    inline void RemoveEntry(const JSThread *thread, int index);
};

class TaggedTreeSet : public TaggedTree<TaggedTreeSet> {
public:
    using RBTree = TaggedTree<TaggedTreeSet>;
    static TaggedTreeSet *Cast(ObjectHeader *obj)
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

    inline JSTaggedValue GetFirstKey() const;
    inline JSTaggedValue GetLastKey() const;
    static JSHandle<TaggedArray> GetArrayFromSet(const JSThread *thread, const JSHandle<TaggedTreeSet> &set);
    static int FindEntry(JSThread *thread, const JSHandle<TaggedTreeSet> &set, const JSHandle<JSTaggedValue> &key);

    static const int ENTRY_SIZE = 5;
    static const int ENTRY_VALUE_INDEX = 0;
    static const int ENTRY_COLOR_INDEX = 1;
    static const int ENTRY_PARENT_INDEX = 2;
    static const int ENTRY_LEFT_CHILD_INDEX = 3;
    static const int ENTRY_RIGHT_CHILD_INDEX = 4;

    DECL_DUMP()

    inline void CopyEntry(const JSThread *thread, int parent, const JSHandle<TaggedTreeSet> &newTree, int index);
    inline void CopyData(const JSThread *thread, int dst, int src);
    inline void RemoveEntry(const JSThread *thread, int index);
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_TAGGED_TREE_H
