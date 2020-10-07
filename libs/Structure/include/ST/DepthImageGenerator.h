/*
    DepthImageGenerator.h

    Copyright © 2020 Occipital, Inc. All rights reserved.
    This file is part of the Structure SDK.
    Unauthorized copying of this file, via any medium is strictly prohibited.
    Proprietary and confidential.

    http://structure.io
*/

#pragma once

#include <ST/CameraFrames.h>

namespace ST
{

struct ST_API DepthImageGenerator
{
    DepthImageGenerator();
    ~DepthImageGenerator();
    
    void setMinAndMaxDepthInMm(float min_depth, float max_depth);
    
    ST::DepthFrame generateDepthFromSynchronizedDepthAndInfrared(
        const ST::DepthFrame& depth, const ST::InfraredFrame& infrared
    );
    
    ST_DECLARE_OPAQUE_INTERNALS(DepthImageGenerator);
};

};
