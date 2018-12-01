/*
    BSD 3-Clause License

    Copyright (c) 2018, Vladimir Bondarev
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.

    2. Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

    3. Neither the name of the copyright holder nor the names of its
    contributors may be used to endorse or promote products derived from
    this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once
#include "cdi/cdi.h"
#include <cstdint>
#include <memory>
#include <mutex>

#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>

namespace cdi {

class ColorTransform;

class Device
{
public:
    Device();

    ~Device();

    bool init(
        IMFActivate* device,
        const uint32_t& width,
        const uint32_t& height,
        const GUID& mf_format,
        const OutputFormat& output_format);

    void sample();

    const void* lock(size_t& bytes);

    void unlock();

    uint32_t width() const;

    uint32_t height() const;

    OutputFormat encoding() const;

    size_t size() const;

private:
    void uninit();

private:
    IMFActivate* m_device;
    std::wstring m_name;
    IMFMediaSource* m_source;
    IMFAttributes* m_attributes;
    IMFSourceReader* m_reader;
    IMFSourceReaderEx* m_readerEx;
    IMFMediaType* m_device_output;
    uint32_t m_width;
    uint32_t m_height;
    OutputFormat m_output_format;
    size_t m_size;

    // Color space transformation
    std::unique_ptr<ColorTransform> m_transform;
    std::mutex m_mutex;
};

}
