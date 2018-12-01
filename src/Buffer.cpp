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
#include "cdi/cdi.h"
#include "DevicePool.h"
#include "Device.h"

#include <map>


namespace cdi
{

class impl_Buffer : public IBuffer
{
public:
    impl_Buffer()
        : m_device(nullptr)
    {
    }

    virtual ~impl_Buffer()
    {
    }

    bool init(
        const uint32_t& device_index,
        const uint32_t& width,
        const uint32_t& height,
        const OutputFormat& encoding)
    {
        // For now just pick first available device
        if(m_pool.get_count() > device_index)
        {
            m_device = std::make_unique<Device>();

            // Select closes matching resolution
            DevicePool::Format selected_format;
            uint32_t selected_quare_delta = std::numeric_limits<uint32_t>::max();
            const std::vector<DevicePool::Format> formats(m_pool.get_formats(device_index));
            for (const DevicePool::Format& fmt : formats)
            {
                const uint32_t curr_square_delta = abs(static_cast<int32_t>((width*width + height*height) - (fmt.width*fmt.width + fmt.height*fmt.height)));
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

            if(!m_device->init(m_pool.get_device(device_index), selected_format.width, selected_format.height, selected_format.format, encoding))
            {
                return false;
            }
        }

        return true;
    }

    uint32_t width() const final
    {
        return m_device->width();
    }

    uint32_t height() const final
    {
        return m_device->height();
    }

    OutputFormat encoding() const final
    {
        return m_device->encoding();
    }

    size_t size() const final
    {
        return m_device->size();
    }

    const void* lock() final
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

    void unlock() final
    {
        if (m_device)
        {
            m_device->unlock();
        }
    }

private:
    DevicePool m_pool;
    std::unique_ptr<Device> m_device;
};

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

std::unique_ptr<IBuffer> openDevice(
    const uint32_t& device_index,
    const uint32_t& width,
    const uint32_t& height,
    const OutputFormat& encoding)
{
    impl_Buffer* capture = nullptr;

    if(encoding != OutputFormat::UNKNOWN)
    {
        capture = new impl_Buffer();
        if(!capture->init(device_index, width, height, encoding))
        {
            delete capture;
            capture = nullptr;
        }
    }

    return std::unique_ptr<IBuffer>(capture);
}

}