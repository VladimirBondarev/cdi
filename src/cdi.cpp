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

#define NOMINMAX
#include "cdi/cdi.h"
#include "Buffer.h"
#include "DevicePool.h"

#include <map>


namespace cdi
{

std::vector<std::wstring> list_devices()
{
    DevicePool devices;
    return devices.get_device_names();
}

struct res_cmp
{
    bool operator() (const Resolution& l, const Resolution& r) const
    {
        return (l.width * l.height) < (r.width * r.height);
    }
};

std::vector<Resolution> get_resolutions(const uint32_t& device_index)
{
    DevicePool devices;

    std::map<Resolution, uint32_t, res_cmp> resolution_map;
    for (const DevicePool::Format& fmt : devices.get_formats(device_index))
    {
        resolution_map[Resolution(fmt.width, fmt.height)]++;
    }

    std::vector<Resolution> resolutions;
    for(const auto& kv : resolution_map)
    {
        resolutions.push_back(kv.first);
    }

    return resolutions;
}

std::unique_ptr<IBuffer> open_device(
    const uint32_t& device_index,
    const uint32_t& width,
    const uint32_t& height,
    const Encoding& encoding)
{
    std::unique_ptr<Buffer> buffer;

    if(encoding != Encoding::UNKNOWN)
    {
        buffer = std::make_unique<Buffer>();
        if(!buffer->init(device_index, width, height, encoding))
        {
            buffer.reset();
        }
    }

    return std::move(buffer);
}

}