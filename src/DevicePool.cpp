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

#include "DevicePool.h"
#include "GuidToString.h"
#include "Macros.inl"

#include <map>

#include <mfapi.h>
#include <mfidl.h>

namespace cdi {

DevicePool::Format::Format()
    : framerate(0)
    , width(0)
    , height(0)
    , format(MFVideoFormat_Base)
    , format_translation("MFVideoFormat_Base")
{
}

DevicePool::DevicePool()
    : m_devices(nullptr)
    , m_count(0)
{
    std::unique_ptr<cdi::util::ScopeGuard> class_guard(std::make_unique<cdi::util::ScopeGuard>());
    cdi::util::ScopeGuard local_guard;

    FAILED_RETURN(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE),);
    FAILED_RETURN(MFStartup(MF_VERSION),);
    *class_guard += []() { MFShutdown(); };

    // Create empty attribute filter
    IMFAttributes* attributes = nullptr;
    FAILED_RETURN(MFCreateAttributes(&attributes, 1),);
    local_guard += [&attributes]() { SAFE_RELEASE(attributes); };

    // Configure attribute filter
    FAILED_RETURN(attributes->SetGUID(
        MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
        MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID),);

    // Fetch devices based on the attribute filter
    FAILED_RETURN(MFEnumDeviceSources(attributes, &m_devices, &m_count),);
    *class_guard += [this]()
    {
        for (uint32_t i = 0; i < m_count; i++)
        {
            SAFE_RELEASE(m_devices[i]);
        }
        CoTaskMemFree(m_devices);
    };

    // Move deinitialization
    m_uninit_guard = std::move(class_guard);
}

DevicePool::~DevicePool()
{
}

uint32_t DevicePool::get_count() const
{
    return m_count;
}

IMFActivate* DevicePool::get_device(const uint32_t& device_index)
{
    IMFActivate* device = nullptr;
    if(device_index < m_count)
    {   
        device = m_devices[device_index];
    }
    return device;
}

std::vector<std::wstring> DevicePool::get_device_names()
{
    std::vector<std::wstring> names;

    // Fetch device names
    for (uint32_t i = 0; i < m_count; i++)
    {
        wchar_t* device_name = nullptr;
        const HRESULT res = m_devices[i]->GetAllocatedString(
            MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, &device_name, nullptr);
        if (SUCCEEDED(res))
        {
            names.push_back(device_name);
            CoTaskMemFree(device_name);
            device_name = nullptr;
        }
        else
        {
            break;
        }
    }

    return names;
}

std::vector<DevicePool::Format> DevicePool::get_formats(const uint32_t& device_index)
{
    cdi::util::ScopeGuard local_guard;
    std::vector<Format> formats;

    IMFActivate* device = get_device(device_index);

    IMFMediaSource* source = nullptr;
    FAILED_RETURN(device->ActivateObject(IID_PPV_ARGS(&source)), formats);
    local_guard += [&source]() { SAFE_RELEASE(source); };

    IMFPresentationDescriptor* presentation_desc = nullptr;
    FAILED_RETURN(source->CreatePresentationDescriptor(&presentation_desc), formats);
    local_guard += [&presentation_desc]() { SAFE_RELEASE(presentation_desc); };

    BOOL selected = FALSE;
    IMFStreamDescriptor* stream_desc = nullptr;
    FAILED_RETURN(presentation_desc->GetStreamDescriptorByIndex(0, &selected, &stream_desc), formats);
    local_guard += [&stream_desc]() { SAFE_RELEASE(stream_desc); };

    IMFMediaTypeHandler* type_handler = nullptr;
    FAILED_RETURN(stream_desc->GetMediaTypeHandler(&type_handler), formats);
    local_guard += [&type_handler]() { SAFE_RELEASE(type_handler); };

    DWORD type_count = 0;
    FAILED_RETURN(type_handler->GetMediaTypeCount(&type_count), formats);

    for (DWORD i = 0; i < type_count; i++)
    {
        cdi::util::ScopeGuard guard;
        IMFMediaType* type = nullptr;
        FAILED_RETURN(type_handler->GetMediaTypeByIndex(i, &type), formats);
        guard += [&type](){ SAFE_RELEASE(type); };

        Format fmt;
        PROPVARIANT prop = {};

        FAILED_RETURN(type->GetItem(MF_MT_SUBTYPE, &prop), formats);
        if(prop.vt == VT_CLSID)
        {
            fmt.format = *prop.puuid;
            fmt.format_translation = GuidToString(*prop.puuid);
        }

        FAILED_RETURN(type->GetItem(MF_MT_FRAME_SIZE, &prop), formats);
        if(prop.vt == VT_UI8)
        {
            fmt.width = prop.uhVal.HighPart;
            fmt.height = prop.uhVal.LowPart;
        }

        FAILED_RETURN(type->GetItem(MF_MT_FRAME_RATE, &prop), formats);
        if (prop.vt == VT_UI8)
        {
            fmt.framerate = prop.uhVal.HighPart;
        }

        formats.push_back(fmt);
    }

    return formats;
}

}
