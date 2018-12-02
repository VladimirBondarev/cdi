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

#include "Device.h"
#include "ColorTransform.h"
#include "ScopeGuard.inl"
#include "Macros.inl"

#include <cassert>


namespace cdi {

Device::Device()
    : m_device(nullptr)
    , m_source(nullptr)
    , m_attributes(nullptr)
    , m_reader(nullptr)
    , m_readerEx(nullptr)
    , m_device_output(nullptr)
    , m_width(0)
    , m_height(0)
    , m_output_format(Encoding::UNKNOWN)
    , m_size(0)
{
}

Device::~Device()
{
    uninit();
}

bool Device::init(
    IMFActivate* device,
    const uint32_t& width,
    const uint32_t& height,
    const GUID& mf_format,
    const Encoding& output_format)
{
    cdi::util::ScopeGuard uninit_guard;
    uninit_guard += [this]() { uninit(); };

    if(m_device != nullptr)
    {
        return false;
    }

    m_device = device;
    m_width = width;
    m_height = height;
    m_output_format = output_format;

    // Fetch device name
    {
        wchar_t* device_name = nullptr;
        FAILED_RETURN(m_device->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, &device_name, nullptr), false);

        m_name = device_name;
        CoTaskMemFree(device_name);
        device_name = nullptr;
    }

    // Activate
    FAILED_RETURN(m_device->ActivateObject(IID_PPV_ARGS(&m_source)), false);

    // Create video attribute
    FAILED_RETURN(MFCreateAttributes(&m_attributes, 1), false);
    FAILED_RETURN(m_attributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID), false);

    // Create Reader
    FAILED_RETURN(MFCreateSourceReaderFromMediaSource(m_source, m_attributes, &m_reader), false);

    // Fetch SourceReaderEx
    FAILED_RETURN(m_reader->QueryInterface(IID_PPV_ARGS(&m_readerEx)), false);

    // Create device output
    FAILED_RETURN(MFCreateMediaType(&m_device_output), false);
    FAILED_RETURN(m_device_output->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video), false);
    FAILED_RETURN(m_device_output->SetGUID(MF_MT_SUBTYPE, mf_format), false);
    FAILED_RETURN(MFSetAttributeSize(m_device_output, MF_MT_FRAME_SIZE, m_width, m_height), false);

    // Connect reader to the media output
    FAILED_RETURN(m_reader->SetCurrentMediaType(0, nullptr, m_device_output), false);

    GUID mf_video_format = MFVideoFormat_I420;
    switch (output_format)
    {
    case Encoding::I420:
        mf_video_format = MFVideoFormat_I420;
        m_size = m_width * m_height + (m_width * m_height >> 2) * 2;
        break;
    case Encoding::RGB24:
        mf_video_format = MFVideoFormat_RGB24;
        m_size = m_width * m_height * 3;
        break;
    case Encoding::RGBA32:
        mf_video_format = MFVideoFormat_RGB32;
        m_size = m_width * m_height * 4;
        break;
    default:
        break;
    }

    m_transform = std::make_unique<ColorTransform>();
    if (!m_transform->init(m_device_output, mf_video_format))
    {
        return false;
    }

    // Presample, this ensures next sample will have a valid data
    sample();

    uninit_guard.cancel();

    return true;
}

void Device::sample()
{
    if(m_reader == nullptr)
    {
        return;
    }

    DWORD stream_index = 0;
    DWORD flags = 0;
    LONGLONG timestamp = 0;
    IMFSample* sample = nullptr;

    FAILED_RETURN(m_reader->ReadSample(
        static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM),
        0,
        &stream_index,
        &flags,
        &timestamp,
        &sample),);

    if(sample == nullptr)
    {
        return;
    }

    // call color converter here
    m_transform->transform(sample);
    SAFE_RELEASE(sample);
}

const void* Device::lock(size_t& bytes)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    bytes = 0;
    const void* data = nullptr;

    if(m_transform)
    {
        data = m_transform->lock(bytes);
    }

    return data;
}

void Device::unlock()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if(m_transform)
    {
        m_transform->unlock();
    }
}

uint32_t Device::width() const
{
    return m_width;
}

uint32_t Device::height() const
{
    return m_height;
}

Encoding Device::encoding() const
{
    return m_output_format;
}

size_t Device::size() const
{
    return m_size;
}

void Device::uninit()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    m_transform.reset();

    SAFE_RELEASE(m_reader);
    SAFE_RELEASE(m_readerEx);
    SAFE_RELEASE(m_attributes);
    SAFE_RELEASE(m_source);
    SAFE_RELEASE(m_device);
}

}
