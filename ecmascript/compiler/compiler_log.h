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

#ifndef ECMASCRIPT_COMPILER_LOG_H
#define ECMASCRIPT_COMPILER_LOG_H

#include <algorithm>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace panda::ecmascript::kungfu {
class CompilerLog {
public:
    explicit CompilerLog(const std::string &logMehtods) : methods_(logMehtods) {}
    ~CompilerLog() = default;

    bool IsAlwaysEnabled() const
    {
        return methods_.compare("all") == 0;
    }

    bool IsAlwaysDisabled() const
    {
        return methods_.compare("none") == 0;
    }

    bool IncludesMethod(const std::string &methodName) const
    {
        return methods_.find(methodName) != std::string::npos;
    }

private:
    std::string methods_ {"none"};
};

class AotLog : public CompilerLog {
public:
    static const char fileSplitSign = ':';
    static const char methodSplitSign = ',';

    explicit AotLog(const std::string &logMehtods) : CompilerLog(logMehtods)
    {
        ParseFileMethodsName(logMehtods);
    }
    ~AotLog() = default;

    bool IncludesMethod(const std::string &fileName, const std::string &methodName) const
    {
        if (fileMethods_.find(fileName) == fileMethods_.end()) {
            return false;
        }
        std::vector mehtodVector = fileMethods_.at(fileName);
        auto it = find(mehtodVector.begin(), mehtodVector.end(), methodName);
        return (it != mehtodVector.end());
    }

private:
    std::vector<std::string> spiltString(const std::string &str, const char ch)
    {
        std::vector<std::string> vec{};
        std::istringstream sstr(str.c_str());
        std::string spilt;
        while(getline(sstr, spilt, ch)) {
            vec.emplace_back(spilt);
        }
        return vec;
    }

    void ParseFileMethodsName(const std::string &logMehtods)
    {
        if (IsAlwaysEnabled() || IsAlwaysDisabled()) {
            return;
        }

        std::vector<std::string> fileVector = spiltString(logMehtods, fileSplitSign);
        std::vector<std::string> itemVector;
        for (size_t index = 0; index < fileVector.size(); ++index) {
            itemVector = spiltString(fileVector[index], methodSplitSign);
            std::vector<std::string> methodVector(itemVector.begin() + 1, itemVector.end());
            fileMethods_[itemVector[0]] = methodVector;
        }
    }

    std::map<std::string, std::vector<std::string>> fileMethods_ {};
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_LOG_H
