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

#include "ecmascript/hprof/heap_snapshot_json_serializer.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/hprof/heap_snapshot.h"
#include "ecmascript/hprof/string_hashmap.h"

namespace panda::ecmascript {
bool HeapSnapShotJSONSerializer::Serialize(HeapSnapShot *snapShot, const CString &fileName)
{
    // Serialize Node/Edge/String-Table
    LOG(ERROR, RUNTIME) << "HeapSnapShotJSONSerializer::Serialize begin";
    snapShot_ = snapShot;
    ASSERT(snapShot_->GetNodes() != nullptr && snapShot_->GetEdges() != nullptr &&
           snapShot_->GetEcmaStringTable() != nullptr);
    stringBuffer_.str("");  // Clear Buffer

    SerializeSnapShotHeader();     // 1.
    SerializeNodes();              // 2.
    SerializeEdges();              // 3.
    SerializeTraceFunctionInfo();  // 4.
    SerializeTraceTree();          // 5.
    SerializeSamples();            // 6.
    SerializeLocations();          // 7.
    SerializeStringTable();        // 8.
    SerializerSnapShotClosure();   // 9.

    WriteJSON(fileName);           // 10.
    LOG(ERROR, RUNTIME) << "HeapSnapShotJSONSerializer::Serialize exit";
    return true;
}

void HeapSnapShotJSONSerializer::SerializeSnapShotHeader()
{
    stringBuffer_ << "{\"snapshot\":\n";  // 1.
    stringBuffer_ << "{\"meta\":\n";      // 2.
    // NOLINTNEXTLINE(modernize-raw-string-literal)
    stringBuffer_ << "{\"node_fields\":[\"type\",\"name\",\"id\",\"self_size\",\"edge_count\",\"trace_node_id\",";
    stringBuffer_ << "\"detachedness\"],\n";  // 3.
    // NOLINTNEXTLINE(modernize-raw-string-literal)
    stringBuffer_ << "\"node_types\":[[\"hidden\",\"array\",\"string\",\"object\",\"code\",\"closure\",\"regexp\",";
    // NOLINTNEXTLINE(modernize-raw-string-literal)
    stringBuffer_ << "\"number\",\"native\",\"synthetic\",\"concatenated string\",\"slicedstring\",\"symbol\",";
    // NOLINTNEXTLINE(modernize-raw-string-literal)
    stringBuffer_ << "\"bigint\"],\"string\",\"number\",\"number\",\"number\",\"number\",\"number\"],\n";  // 4.
    // NOLINTNEXTLINE(modernize-raw-string-literal)
    stringBuffer_ << "\"edge_fields\":[\"type\",\"name_or_index\",\"to_node\"],\n";  // 5.
    // NOLINTNEXTLINE(modernize-raw-string-literal)
    stringBuffer_ << "\"edge_types\":[[\"context\",\"element\",\"property\",\"internal\",\"hidden\",\"shortcut\",";
    // NOLINTNEXTLINE(modernize-raw-string-literal)
    stringBuffer_ << "\"weak\"],\"string_or_number\",\"node\"],\n";  // 6.
    // NOLINTNEXTLINE(modernize-raw-string-literal)
    stringBuffer_ << "\"trace_function_info_fields\":[\"function_id\",\"name\",\"script_name\",\"script_id\",";
    // NOLINTNEXTLINE(modernize-raw-string-literal)
    stringBuffer_ << "\"line\",\"column\"],\n";  // 7.
    // NOLINTNEXTLINE(modernize-raw-string-literal)
    stringBuffer_ << "\"trace_node_fields\":[\"id\",\"function_info_index\",\"count\",\"size\",\"children\"],\n";
    // NOLINTNEXTLINE(modernize-raw-string-literal)
    stringBuffer_ << "\"sample_fields\":[\"timestamp_us\",\"last_assigned_id\"],\n";  // 9.
    // NOLINTNEXTLINE(modernize-raw-string-literal)
    stringBuffer_ << "\"location_fields\":[\"object_index\",\"script_id\",\"line\",\"column\"]},\n";  // 10.
    stringBuffer_ << "\"node_count\":" << snapShot_->GetNodeCount() << ",\n";                         // 11.
    stringBuffer_ << "\"edge_count\":" << snapShot_->GetEdgeCount() << ",\n";                         // 12.
    stringBuffer_ << "\"trace_function_count\":"
                  << "0\n";   // 13.
    stringBuffer_ << "},\n";  // 14.
}

void HeapSnapShotJSONSerializer::SerializeNodes()
{
    const CList<Node *> *nodes = snapShot_->GetNodes();
    const StringHashMap *stringTable = snapShot_->GetEcmaStringTable();
    ASSERT(nodes != nullptr);
    stringBuffer_ << "\"nodes\":[";  // Section Header
    size_t i = 0;
    for (auto *node : *nodes) {
        if (i > 0) {
            stringBuffer_ << ",";  // add comma except first line
        }
        stringBuffer_ << static_cast<int>(NodeTypeConverter::Convert(node->GetType())) << ",";  // 1.
        stringBuffer_ << stringTable->GetStringId(node->GetName()) << ",";                      // 2.
        stringBuffer_ << node->GetId() << ",";                                                  // 3.
        stringBuffer_ << node->GetSelfSize() << ",";                                            // 4.
        stringBuffer_ << node->GetEdgeCount() << ",";                                           // 5.
        stringBuffer_ << node->GetStackTraceId() << ",";                                        // 6.
        if (i == nodes->size() - 1) {  // add comma at last the line
            stringBuffer_ << "0"
                          << "],\n";  // 7. detachedness default
        } else {
            stringBuffer_ << "0\n";  // 7.
        }
        i++;
    }
}

void HeapSnapShotJSONSerializer::SerializeEdges()
{
    const CVector<Edge *> *edges = snapShot_->GetEdges();
    const StringHashMap *stringTable = snapShot_->GetEcmaStringTable();
    ASSERT(edges != nullptr);
    stringBuffer_ << "\"edges\":[";
    size_t i = 0;
    for (auto *edge : *edges) {
        if (i > 0) {  // add comma except the first line
            stringBuffer_ << ",";
        }
        stringBuffer_ << static_cast<int>(edge->GetType()) << ",";          // 1.
        stringBuffer_ << stringTable->GetStringId(edge->GetName()) << ",";  // 2. Use StringId

        if (i == edges->size() - 1) {  // add comma at last the line
            stringBuffer_ << edge->GetTo()->GetIndex() * Node::NODE_FIELD_COUNT << "],\n";  // 3.
        } else {
            stringBuffer_ << edge->GetTo()->GetIndex() * Node::NODE_FIELD_COUNT << "\n";    // 3.
        }
        i++;
    }
}

void HeapSnapShotJSONSerializer::SerializeTraceFunctionInfo()
{
    stringBuffer_ << "\"trace_function_infos\":[],\n";  // Empty
}

void HeapSnapShotJSONSerializer::SerializeTraceTree()
{
    stringBuffer_ << "\"trace_tree\":[],\n";  // Empty
}

void HeapSnapShotJSONSerializer::SerializeSamples()
{
    stringBuffer_ << "\"samples\":[";
    const CVector<TimeStamp> &timeStamps = snapShot_->GetTimeStamps();
    if (!timeStamps.empty()) {
        auto firstTimeStamp = timeStamps[0];
        bool isFirst = true;
        for (auto timeStamp : timeStamps) {
            if (!isFirst) {
                stringBuffer_ << "\n, ";
            } else {
                isFirst = false;
            }
            stringBuffer_ << timeStamp.GetTimeStamp() - firstTimeStamp.GetTimeStamp() << ", ";
            stringBuffer_ << timeStamp.GetLastSequenceId();
        }
    }
    stringBuffer_ << "],\n";
}

void HeapSnapShotJSONSerializer::SerializeLocations()
{
    stringBuffer_ << "\"locations\":[],\n";
}

void HeapSnapShotJSONSerializer::SerializeStringTable()
{
    const StringHashMap *stringTable = snapShot_->GetEcmaStringTable();
    ASSERT(stringTable != nullptr);
    stringBuffer_ << "\"strings\":[\"<dummy>\",\n";
    stringBuffer_ << "\"\",\n";
    stringBuffer_ << "\"GC roots\",\n";
    // StringId Range from 3
    size_t capcity = stringTable->GetCapcity();
    size_t i = 0;
    for (auto key : stringTable->GetOrderedKeyStorage()) {
        if (i == capcity - 1) {
            stringBuffer_ << "\"" << *(stringTable->GetStringByKey(key)) << "\"\n";  // No Comma for the last line
        } else {
            stringBuffer_ << "\"" << *(stringTable->GetStringByKey(key)) << "\",\n";
        }
        i++;
    }
    stringBuffer_ << "]\n";
}

void HeapSnapShotJSONSerializer::SerializerSnapShotClosure()
{
    stringBuffer_ << "}\n";
}

void HeapSnapShotJSONSerializer::WriteJSON(const CString &fileName)
{
    std::string fName(fileName);
    LOG(ERROR, RUNTIME) << "HeapSnapShotJSONSerializer::WriteJSON" << fName;
    outputStream_.open(fName, std::ios::out);
    if (!outputStream_.good()) {
        LOG_ECMA(ERROR) << "open file failed";
        return;
    }
    outputStream_ << stringBuffer_.str();
    outputStream_.close();
    outputStream_.clear();  // Make sure the next open operation success
}
}  // namespace panda::ecmascript
