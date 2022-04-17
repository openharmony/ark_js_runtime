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

#ifndef ECMASCRIPT_TOOLING_BASE_PT_SCRIPT_H
#define ECMASCRIPT_TOOLING_BASE_PT_SCRIPT_H

#include "libpandabase/macros.h"
#include "ecmascript/tooling/base/pt_types.h"

namespace panda::ecmascript::tooling {
enum class ScriptMatchType : uint8_t {
    URL,
    FILE_NAME,
    HASH,
};

class PtScript {
public:
    PtScript(ScriptId scriptId, const CString &fileName, const CString &url, const CString &source);
    ~PtScript() = default;

    ScriptId GetScriptId() const
    {
        return scriptId_;
    }

    void SetScriptId(ScriptId scriptId)
    {
        scriptId_ = scriptId;
    }

    const CString &GetFileName() const
    {
        return fileName_;
    }

    void SetFileName(const CString &fileName)
    {
        fileName_ = fileName;
    }

    const CString &GetUrl() const
    {
        return url_;
    }

    void SetUrl(const CString &url)
    {
        url_ = url;
    }

    const CString &GetHash() const
    {
        return hash_;
    }

    void SetHash(const CString &hash)
    {
        hash_ = hash;
    }

    const CString &GetScriptSource() const
    {
        return scriptSource_;
    }

    void SetScriptSource(const CString &scriptSource)
    {
        scriptSource_ = scriptSource;
    }

    const CString &GetSourceMapUrl() const
    {
        return sourceMapUrl_;
    }

    void SetSourceMapUrl(const CString &sourceMapUrl)
    {
        sourceMapUrl_ = sourceMapUrl;
    }

    int32_t GetEndLine() const
    {
        return endLine_;
    }

    void SetEndLine(int32_t endLine)
    {
        endLine_ = endLine;
    }

private:
    NO_COPY_SEMANTIC(PtScript);
    NO_MOVE_SEMANTIC(PtScript);

    ScriptId scriptId_ {};         // start from 0, such as "0","1","2"...
    CString fileName_ {};      // binary file name, such as xx.bin
    CString url_ {};           // source file name, such as xx.js
    CString hash_ {};          // js source file hash code
    CString scriptSource_ {};  // js source code
    CString sourceMapUrl_ {};  // source map url
    int32_t endLine_ {0};      // total line number of source file
};
}  // namespace panda::ecmascript::tooling
#endif
