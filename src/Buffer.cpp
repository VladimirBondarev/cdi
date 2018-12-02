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

#define NOMINMAX
#include "Buffer.h"
#include "DevicePool.h"
#include "Device.h"


namespace cdi
{

Buffer::Buffer()
    : m_pool(std::make_unique<DevicePool>())
    , m_device(nullptr)
{
}

Buffer::~Buffer()
{
}

bool Buffer::init(
    const uint32_t& device_index,
    const uint32_t& width,
    const uint32_t& height,
    const OutputFormat& encoding)
{
    // For now just pick first available device
    if(m_pool->get_count() > device_index)
    {
        m_device = std::make_unique<Device>();

        // Select closes matching resolution
        DevicePool::Format selected_format;
        uint32_t selected_quare_delta = std::numeric_limits<uint32_t>::max();
        const std::vector<DevicePool::Format> formats(m_pool->get_formats(device_index));
        const int32_t requestedLen2 = static_cast<int32_t>(width*width + height*height);

        for (const DevicePool::Format& fmt : formats)
        {
            const int32_t currLen2 = static_cast<int32_t>(fmt.width*fmt.width + fmt.height*fmt.height);
            const uint32_t curr_square_delta = std::abs(requestedLen2 - currLen2);

            bool select = false;
            if(curr_square_delta < selected_quare_delta)
            {
                select = true;
            }
            else if(curr_square_delta == selected_quare_delta)
            {
                // Prefer uncompressed format over compressed
                select = fmt.format_translation.find("RGB") == std::string::npos;
            }

            if(select)
            {
                selected_format = fmt;
                selected_quare_delta = curr_square_delta;
            }
        }

        if(!m_device->init(
            m_pool->get_device(device_index),
            selected_format.width,
            selected_format.height,
            selected_format.format,
            encoding))
        {
            return false;
        }
    }

    return true;
}

uint32_t Buffer::width() const
{
    return m_device->width();
}

uint32_t Buffer::height() const
{
    return m_device->height();
}

OutputFormat Buffer::encoding() const
{
    return m_device->encoding();
}

size_t Buffer::size() const
{
    return m_device->size();
}

const void* Buffer::lock()
{
    const void* data = nullptr;

    if(m_device)
    {
        m_device->sample();
            
        size_t bytes = 0;
        data = m_device->lock(bytes);
    }

    return data;
}

void Buffer::unlock()
{
    if (m_device)
    {
        m_device->unlock();
    }
}

}