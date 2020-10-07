/*
    DeviceManager.h

    Copyright © 2020 Occipital, Inc. All rights reserved.
    This file is part of the Structure SDK.
    Unauthorized copying of this file, via any medium is strictly prohibited.
    Proprietary and confidential.

    http://structure.io
*/

#pragma once

#include <ST/CaptureSession.h>
#include <ST/CaptureSessionTypes.h>
#include <ST/Macros.h>

namespace ST
{

struct ST_API DeviceManager
{
    DeviceManager();
    ~DeviceManager();
    
    bool initialize(ST::CaptureSessionDelegate* delegate);
    void release();

    ST::CaptureSession* getDevicePtr(size_t index = 0);
    ST::CaptureSession* getDevicePtr(const char* serial);
    
    size_t getNumberOfDevices() const;
    
    ST_DECLARE_OPAQUE_INTERNALS(DeviceManager);
};

};
