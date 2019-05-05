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
#include <cstdint>
#include <vector>
#include <string>
#include <memory>

#ifndef CDI_DLL_EXPORT
#   define CDI_DLL_EXPORT __declspec(dllexport)
#endif

namespace cdi
{

enum class Encoding
{
    UNKNOWN,
    I420,
    RGB24,
    RGBA32,
};

struct Resolution
{
    Resolution() : width(0), height(0) {}
    Resolution(const uint32_t& width, const uint32_t& height): width(width), height(height) {}
    uint32_t width;
    uint32_t height;
};

class IBuffer
{
public:
    virtual ~IBuffer() {}
    virtual uint32_t width() const = 0;
    virtual uint32_t height() const = 0;
    virtual Encoding encoding() const = 0;
    virtual size_t size() const = 0;
    virtual const void* lock() = 0;
    virtual void unlock() = 0;
};

CDI_DLL_EXPORT std::vector<std::wstring> list_devices();

CDI_DLL_EXPORT std::vector<Resolution> get_resolutions(const uint32_t& device_index);

// Will select closest available resolution
CDI_DLL_EXPORT std::unique_ptr<IBuffer> open_device(
    const uint32_t& device_index,
    const uint32_t& width,
    const uint32_t& height,
    const Encoding& encoding);

}