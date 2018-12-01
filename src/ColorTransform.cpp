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

#include "ColorTransform.h"
#include "ScopeGuard.inl"
#include "Macros.inl"
#include <cassert>
#include <wmcodecdsp.h>


namespace cdi {

ColorTransform::ColorTransform()
    : m_input_type(nullptr)
    , m_transform(nullptr)
    , m_output_sample(nullptr)
    , m_output_buffer(nullptr)
    , m_locked_buffer(nullptr)
{
}

ColorTransform::~ColorTransform()
{
    uninit();
}

bool ColorTransform::init(IMFMediaType* input, const GUID& mf_video_format)
{
    cdi::util::ScopeGuard uninit_guard;
    uninit_guard += [this]() { uninit(); };

    if(m_input_type != nullptr)
    {
        return false;
    }

    m_input_type = input;

    m_input_type->AddRef();

    FAILED_RETURN(CoCreateInstance(
        CLSID_CColorConvertDMO,
        NULL, CLSCTX_INPROC_SERVER,
        IID_IMFTransform,
        (void**)&m_transform), false);

    // Connect transform with the device-output
    FAILED_RETURN(m_transform->SetInputType(0, input, 0), false);

    // Create output type
    {
        cdi::util::ScopeGuard guard;
        IMFMediaType* output_type = nullptr;
        guard += [&output_type](){ SAFE_RELEASE(output_type); };

        FAILED_RETURN(MFCreateMediaType(&output_type), false);
        FAILED_RETURN(output_type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video), false);
        FAILED_RETURN(output_type->SetGUID(MF_MT_SUBTYPE, mf_video_format), false);
        
        uint32_t width = 0;
        uint32_t height = 0;
        FAILED_RETURN( MFGetAttributeSize(m_input_type, MF_MT_FRAME_SIZE, &width, &height), false);
        FAILED_RETURN(MFSetAttributeSize(output_type, MF_MT_FRAME_SIZE, width, height), false);
        FAILED_RETURN(m_transform->SetOutputType(0, output_type, 0), false);
    }

    DWORD status = 0;
    FAILED_RETURN(m_transform->GetInputStatus(0, &status), false);
    
    // Create output sample and Buffer
    {
        // Create sample/frame
        FAILED_RETURN(MFCreateSample(&m_output_sample), false);

        // Fetch target Buffer size
        MFT_OUTPUT_STREAM_INFO output_info = {};
        FAILED_RETURN(m_transform->GetOutputStreamInfo(0, &output_info), false);

        // Create output Buffer
        FAILED_RETURN(MFCreateMemoryBuffer(output_info.cbSize, &m_output_buffer), false);

        // Attach the output Buffer to the output sample
        FAILED_RETURN(m_output_sample->AddBuffer(m_output_buffer), false);
    }

    uninit_guard.cancel();

    return true;
}

void ColorTransform::transform(IMFSample* sample)
{
    HRESULT res = S_FALSE;

    // Configure color space conversion
    res = m_transform->ProcessInput(0, sample, 0);
    assert(SUCCEEDED(res) && "Error processing input by MF transform");

    // Process color space conversion
    MFT_OUTPUT_DATA_BUFFER output_buffer_info = {};
    output_buffer_info.pSample = m_output_sample;
    DWORD proces_output_status = 0;
    res = m_transform->ProcessOutput(0, 1, &output_buffer_info, &proces_output_status);
    assert(SUCCEEDED(res) && "Error processing output by MF transform");
}

const void* ColorTransform::lock(size_t& bytes)
{
    HRESULT res = S_FALSE;

    res = m_output_sample->ConvertToContiguousBuffer(&m_locked_buffer);
    assert(SUCCEEDED(res) && "Error converting to contiguous Buffer");

    BYTE* data = 0;
    DWORD buffer_length_max = 0;
    DWORD buffer_length_curr = 0;
    res = m_locked_buffer->Lock(&data, &buffer_length_max, &buffer_length_curr);
    assert(SUCCEEDED(res) && "Error locking contiguous Buffer");

    bytes = static_cast<size_t>(buffer_length_curr);

    return data;
}

void ColorTransform::unlock()
{
    if(m_locked_buffer != nullptr)
    {
        const HRESULT res = m_locked_buffer->Unlock();
        assert(SUCCEEDED(res) && "Error unlocking contiguous Buffer");
        SAFE_RELEASE(m_locked_buffer);
    }
}

void ColorTransform::uninit()
{
    assert(m_locked_buffer == nullptr
           && "Before Buffer can be destroyed, it needs to be unlocked");

    SAFE_RELEASE(m_output_sample);
    SAFE_RELEASE(m_output_buffer);
    SAFE_RELEASE(m_transform);
    SAFE_RELEASE(m_input_type);
}

}
