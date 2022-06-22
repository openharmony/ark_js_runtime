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

#ifndef ECMASCRIPT_TOOLING_BASE_PT_JSON_H
#define ECMASCRIPT_TOOLING_BASE_PT_JSON_H

#include <memory>
#include <string>

#include "cJSON.h"

namespace panda::ecmascript::tooling {
enum class Result : uint8_t {
    SUCCESS,
    NOT_EXIST,
    TYPE_ERROR,
};

class PtJson {
public:
    PtJson() = default;
    explicit PtJson(cJSON *object) : object_(object) {}
    ~PtJson() = default;

    // Create empty json object
    static std::unique_ptr<PtJson> CreateObject();
    static std::unique_ptr<PtJson> CreateArray();

    // Release cJSON object memory
    void ReleaseRoot();

    // String parse to json
    static std::unique_ptr<PtJson> Parse(const std::string &data);

    // To string
    std::string Stringify() const;

    // Add Json child
    bool Add(const char *key, bool value) const;
    bool Add(const char *key, int32_t value) const;
    bool Add(const char *key, int64_t value) const;
    bool Add(const char *key, double value) const;
    bool Add(const char *key, const char *value) const;
    bool Add(const char *key, const std::unique_ptr<PtJson> &value) const;

    // Push back to array
    bool Push(bool value) const;
    bool Push(int32_t value) const;
    bool Push(int64_t value) const;
    bool Push(double value) const;
    bool Push(const char *value) const;
    bool Push(const std::unique_ptr<PtJson> &value) const;

    // Remove Json child
    bool Remove(const char *key) const;

    bool Contains(const char *key) const;

    std::string GetKey() const;

    cJSON *GetJson() const;

    // Type check
    bool IsBool() const;
    bool IsNumber() const;
    bool IsString() const;
    bool IsObject() const;
    bool IsArray() const;
    bool IsNull() const;

    // Object accessor
    bool GetBool(bool defaultValue = false) const;
    int32_t GetInt(int32_t defaultValue = 0) const;
    int64_t GetInt64(int64_t defaultValue = 0) const;
    double GetDouble(double defaultValue = 0.0) const;
    std::string GetString() const;

    // Array accessor
    int32_t GetSize() const;
    std::unique_ptr<PtJson> Get(int32_t index) const;

    // Child item accessor
    Result GetBool(const char *key, bool *value) const;
    Result GetInt(const char *key, int32_t *value) const;
    Result GetInt64(const char *key, int64_t *value) const;
    Result GetDouble(const char *key, double *value) const;
    Result GetString(const char *key, std::string *value) const;
    Result GetObject(const char *key, std::unique_ptr<PtJson> *value) const;
    Result GetArray(const char *key, std::unique_ptr<PtJson> *value) const;
    Result GetAny(const char *key, std::unique_ptr<PtJson> *value) const;

private:
    cJSON *object_ = nullptr;
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_TOOLING_BASE_PT_JSON_H
