/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ecmascript/tooling/base/pt_json.h"

#include "libpandabase/macros.h"

namespace panda::ecmascript::tooling {
std::unique_ptr<PtJson> PtJson::CreateObject()
{
    return std::make_unique<PtJson>(cJSON_CreateObject());
}

std::unique_ptr<PtJson> PtJson::CreateArray()
{
    return std::make_unique<PtJson>(cJSON_CreateArray());
}

void PtJson::ReleaseRoot()
{
    if (object_ != nullptr) {
        cJSON_Delete(object_);
        object_ = nullptr;
    }
}

std::unique_ptr<PtJson> PtJson::Parse(const std::string &data)
{
    cJSON *value = cJSON_ParseWithOpts(data.c_str(), nullptr, true);
    return std::make_unique<PtJson>(value);
}

std::string PtJson::Stringify() const
{
    if (object_ == nullptr) {
        return "";
    }

    char *str = cJSON_PrintUnformatted(object_);
    if (str == nullptr) {
        return "";
    }

    std::string result(str);
    cJSON_free(str);
    return result;
}

bool PtJson::Add(const char *key, bool value) const
{
    ASSERT(key != nullptr && !Contains(key));

    cJSON *node = cJSON_CreateBool(value);
    if (node == nullptr) {
        return false;
    }

    cJSON_bool ret = cJSON_AddItemToObject(object_, key, node);
    if (ret == 0) {
        cJSON_Delete(node);
        return false;
    }

    return true;
}

bool PtJson::Add(const char *key, int32_t value) const
{
    return Add(key, static_cast<double>(value));
}

bool PtJson::Add(const char *key, int64_t value) const
{
    return Add(key, static_cast<double>(value));
}

bool PtJson::Add(const char *key, double value) const
{
    ASSERT(key != nullptr && !Contains(key));

    cJSON *node = cJSON_CreateNumber(value);
    if (node == nullptr) {
        return false;
    }

    cJSON_bool ret = cJSON_AddItemToObject(object_, key, node);
    if (ret == 0) {
        cJSON_Delete(node);
        return false;
    }

    return true;
}

bool PtJson::Add(const char *key, const char *value) const
{
    ASSERT(key != nullptr && !Contains(key));

    cJSON *node = cJSON_CreateString(value);
    if (node == nullptr) {
        return false;
    }

    cJSON_bool ret = cJSON_AddItemToObject(object_, key, node);
    if (ret == 0) {
        cJSON_Delete(node);
        return false;
    }

    return true;
}

bool PtJson::Add(const char *key, const std::unique_ptr<PtJson> &value) const
{
    ASSERT(key != nullptr && !Contains(key));

    cJSON *node = value->GetJson();
    if (node == nullptr) {
        return false;
    }

    cJSON_bool ret = cJSON_AddItemToObject(object_, key, node);
    if (ret == 0) {
        return false;
    }

    return true;
}

bool PtJson::Push(bool value) const
{
    cJSON *node = cJSON_CreateBool(value);
    if (node == nullptr) {
        return false;
    }

    cJSON_bool ret = cJSON_AddItemToArray(object_, node);
    if (ret == 0) {
        cJSON_Delete(node);
        return false;
    }

    return true;
}

bool PtJson::Push(int32_t value) const
{
    return Push(static_cast<double>(value));
}

bool PtJson::Push(int64_t value) const
{
    return Push(static_cast<double>(value));
}

bool PtJson::Push(double value) const
{
    cJSON *node = cJSON_CreateNumber(value);
    if (node == nullptr) {
        return false;
    }

    cJSON_bool ret = cJSON_AddItemToArray(object_, node);
    if (ret == 0) {
        cJSON_Delete(node);
        return false;
    }

    return true;
}

bool PtJson::Push(const char *value) const
{
    cJSON *node = cJSON_CreateString(value);
    if (node == nullptr) {
        return false;
    }

    cJSON_bool ret = cJSON_AddItemToArray(object_, node);
    if (ret == 0) {
        cJSON_Delete(node);
        return false;
    }

    return true;
}

bool PtJson::Push(const std::unique_ptr<PtJson> &value) const
{
    if (value == nullptr) {
        return false;
    }

    cJSON *node = value->GetJson();
    if (node == nullptr) {
        return false;
    }

    cJSON_bool ret = cJSON_AddItemToArray(object_, node);
    if (ret == 0) {
        return false;
    }

    return true;
}

bool PtJson::Remove(const char *key) const
{
    ASSERT(key != nullptr && Contains(key));

    cJSON_DeleteItemFromObject(object_, key);
    return true;
}

bool PtJson::Contains(const char *key) const
{
    cJSON *node = cJSON_GetObjectItemCaseSensitive(object_, key);
    return node != nullptr;
}

std::string PtJson::GetKey() const
{
    if (object_ == nullptr || object_->string == nullptr) {
        return "";
    }

    return std::string(object_->string);
}

cJSON *PtJson::GetJson() const
{
    return object_;
}

bool PtJson::IsBool() const
{
    return cJSON_IsBool(object_) != 0;
}

bool PtJson::IsNumber() const
{
    return cJSON_IsNumber(object_) != 0;
}

bool PtJson::IsString() const
{
    return cJSON_IsString(object_) != 0;
}

bool PtJson::IsObject() const
{
    return cJSON_IsObject(object_) != 0;
}

bool PtJson::IsArray() const
{
    return cJSON_IsArray(object_) != 0;
}

bool PtJson::IsNull() const
{
    return cJSON_IsNull(object_) != 0;
}

bool PtJson::GetBool(bool defaultValue) const
{
    if (!IsBool()) {
        return defaultValue;
    }

    return cJSON_IsTrue(object_) != 0;
}

int32_t PtJson::GetInt(int32_t defaultValue) const
{
    if (!IsNumber()) {
        return defaultValue;
    }

    return static_cast<int32_t>(object_->valuedouble);
}

int64_t PtJson::GetInt64(int64_t defaultValue) const
{
    if (!IsNumber()) {
        return defaultValue;
    }

    return static_cast<int64_t>(object_->valuedouble);
}

double PtJson::GetDouble(double defaultValue) const
{
    if (!IsNumber()) {
        return defaultValue;
    }

    return object_->valuedouble;
}

std::string PtJson::GetString() const
{
    if (!IsString()) {
        return "";
    }

    return std::string(object_->valuestring);
}

int32_t PtJson::GetSize() const
{
    return cJSON_GetArraySize(object_);
}

std::unique_ptr<PtJson> PtJson::Get(int32_t index) const
{
    return std::make_unique<PtJson>(cJSON_GetArrayItem(object_, index));
}

Result PtJson::GetBool(const char *key, bool *value) const
{
    cJSON *item = cJSON_GetObjectItem(object_, key);
    if (item == nullptr) {
        return Result::NOT_EXIST;
    }
    if (cJSON_IsBool(item) == 0) {
        return Result::TYPE_ERROR;
    }

    *value = cJSON_IsTrue(item) != 0;
    return Result::SUCCESS;
}

Result PtJson::GetInt(const char *key, int32_t *value) const
{
    double result;
    Result ret = GetDouble(key, &result);
    if (ret == Result::SUCCESS) {
        *value = static_cast<int32_t>(result);
    }
    return ret;
}

Result PtJson::GetInt64(const char *key, int64_t *value) const
{
    double result;
    Result ret = GetDouble(key, &result);
    if (ret == Result::SUCCESS) {
        *value = static_cast<int64_t>(result);
    }
    return ret;
}

Result PtJson::GetDouble(const char *key, double *value) const
{
    cJSON *item = cJSON_GetObjectItem(object_, key);
    if (item == nullptr) {
        return Result::NOT_EXIST;
    }
    if (cJSON_IsNumber(item) == 0) {
        return Result::TYPE_ERROR;
    }

    *value = item->valuedouble;
    return Result::SUCCESS;
}

Result PtJson::GetString(const char *key, std::string *value) const
{
    cJSON *item = cJSON_GetObjectItem(object_, key);
    if (item == nullptr) {
        return Result::NOT_EXIST;
    }
    if (cJSON_IsString(item) == 0) {
        return Result::TYPE_ERROR;
    }

    *value = item->valuestring;
    return Result::SUCCESS;
}

Result PtJson::GetObject(const char *key, std::unique_ptr<PtJson> *value) const
{
    cJSON *item = cJSON_GetObjectItem(object_, key);
    if (item == nullptr) {
        return Result::NOT_EXIST;
    }
    if (cJSON_IsObject(item) == 0) {
        return Result::TYPE_ERROR;
    }

    *value = std::make_unique<PtJson>(item);
    return Result::SUCCESS;
}

Result PtJson::GetArray(const char *key, std::unique_ptr<PtJson> *value) const
{
    cJSON *item = cJSON_GetObjectItem(object_, key);
    if (item == nullptr) {
        return Result::NOT_EXIST;
    }
    if (cJSON_IsArray(item) == 0) {
        return Result::TYPE_ERROR;
    }

    *value = std::make_unique<PtJson>(item);
    return Result::SUCCESS;
}
}  // namespace panda::ecmascript
