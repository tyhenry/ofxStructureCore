/*
    CaptureSessionSettings.h

    Copyright © 2017 Occipital, Inc. All rights reserved.
    This file is part of the Structure SDK.
    Unauthorized copying of this file, via any medium is strictly prohibited.
    Proprietary and confidential.

    http://structure.io
*/

#pragma once

#include <ST/Macros.h>

struct ALooper;

namespace ST
{

//------------------------------------------------------------------------------

/** @brief Possible sources for CaptureSession streaming. */
enum class CaptureSessionSourceId
{
    Invalid = -1,

    /** Stream from an OCC file. */
    OCC,

    /** Stream from a Structure Core. */
    StructureCore,

    HowMany
};

//------------------------------------------------------------------------------

/** @brief Possible playback modes for OCC streaming. */
enum class CaptureSessionOCCPlaybackMode
{
    Invalid = -1,

    /** @brief Force the FPS to a set limit. @see CaptureSessionSettings::OCCPlaybackSettings::rateLimit */
    RateLimited,

    /** @brief Stream all data in the file frame by frame and wait for the callback to return before sending another. */
    NonDropping,

    /** @brief Attempt to emulate Structure Core streaming fully, follows time interval between frames. */
    RealTime,

    HowMany,

    // Aliases
    Default = RealTime,
};

//------------------------------------------------------------------------------

/** @brief Possible resolutions for depth frames. */
enum class StructureCoreDepthResolution
{
    Invalid = -1,

    _320x240,
    _640x480,
    _1280x960,

    HowMany,

    // Aliases
       QVGA = _320x240,
        VGA = _640x480,
       SXGA = _1280x960,
    Default = VGA,
};

/** @brief Preset depth range modes to use for different situations. */
enum class StructureCoreDepthRangeMode
{
    Invalid = -1,

    /** Estimated range of 0.35m to 0.92m */
    VeryShort,

    /** Estimated range of 0.41m to 1.36m */
    Short,

    /** Estimated range of 0.52m to 5.23m */
    Medium,

    /** Estimated range of 0.58m to 8.0m */
    Long,

    /** Estimated range of 0.58m to 10.0m */
    VeryLong,

    /** Estimated range of 0.35m to 10.0m */
    Hybrid,

    /** Specific configuration for scanning human bodies */
    BodyScanning,

    /** Don't use a preset, use the provided configuration options. */
    Default,
};

//------------------------------------------------------------------------------

/** @brief Dynamic calibration modes to compensate for misalignment of the
    stereo infrared cameras caused by mechanical stress.

    Note: Dynamic calibration is a beta feature. */
enum class StructureCoreDynamicCalibrationMode
{
    /** No dynamic calibration is performed. If misalignment of the stereo
        infrared cameras exceeds 0.1 mm, the quality of depth data may be
        reduced. */
    Off,

    /** A single dynamic calibration cycle is performed when depth streaming
        starts. This mode is sufficient to correct for misalignment caused by
        static mechanical stress, such as that caused by attaching the sensor to
        a fixture which is not perfectly aligned to the body of the sensor. Once
        a suitable calibration is determined, it is stored persistently on the
        sensor. If the dynamic calibration matches the exsting calibration
        stored on the sensor (if any), no update is performed. */
    OneShotPersistent,

    /** Dynamic calibration is performed continuously as the sensor streams
        depth data. This mode should be employed in cases where the sensor is
        subject to continuously varying mechanical stresses such as vibration,
        thermal expansion/contraction, etc. The calibration is not stored on the
        sensor in this mode. */
    ContinuousNonPersistent,

    /** Dynamic calibration is disabled by default. */
    Default = Off,
};

//------------------------------------------------------------------------------

/** @brief Possible resolutions for infrared frames. */
enum class StructureCoreInfraredResolution
{
    Invalid = -1,

    _1280x960,

    HowMany,

    // Aliases
    SXGA = _1280x960,
    Default = SXGA,
};

/** @brief Different data-format modes for infrared frames. */
enum class StructureCoreInfraredMode
{
    Invalid = -1,

    /** Only stream infrared frames from the left camera. */
    LeftCameraOnly,

    /** Only stream infrared frames from the right camera. */
    RightCameraOnly,

    /** Stream the left and the right cameras as one large infrared frame. New images with have width x 2. */
    BothCameras,

    HowMany,

    // Aliases
    Default = BothCameras,
};

//------------------------------------------------------------------------------

/** @brief Possible resolutions for visible frames. */
enum class StructureCoreVisibleResolution
{
    Invalid = -1,

    _640x480,

    HowMany,

    // Aliases
    VGA = _640x480,
    Default = VGA,
};

//------------------------------------------------------------------------------

/** @brief Possible demosaicking options for visible frames with color-enabled Structure Core. */
enum class StructureCoreDemosaicMethod
{
    Invalid = -1,

    /** Very fast, but will sacrifice accuracy on edges. */
    Bilinear,

    /** Slower, but will provide increased accuracy on edges. */
    EdgeAware,

    HowMany,

    // Aliases
    Default = EdgeAware,
};

//------------------------------------------------------------------------------

/** @brief Possible update/stream rates for IMU data. */
enum class StructureCoreIMUUpdateRate
{
    Invalid = -1,

    AccelAndGyro_100Hz,
    AccelAndGyro_200Hz,
    AccelAndGyro_800Hz,
    AccelAndGyro_1000Hz,

    HowMany,

    // Aliases
    Default = AccelAndGyro_800Hz,
};

//------------------------------------------------------------------------------

/** @brief Structure Core sensor boot mode, mostly for debugging. */
enum class StructureCoreBootMode
{
    Invalid = -1,

    FromFlash,
    FromDisk,

    HowMany,

    // Aliases
    Default = FromFlash,
};

//------------------------------------------------------------------------------

/** @brief Possible frame/data dispatch options. For advanced users only. */
enum class CaptureSessionDispatcherId
{
    Invalid = -1,

    AndroidLooper,
    BackgroundThread,

    HowMany,

    // Aliases
#if ANDROID
    Default = AndroidLooper,
#elif _WIN32
    Default = BackgroundThread,
#else
    Default = BackgroundThread,
#endif
};

//------------------------------------------------------------------------------

/** @brief Settings container for initializing CaptureSession. */
struct ST_API CaptureSessionSettings
{
    /** @brief The capture session source to stream. @see CaptureSessionSourceId */
    CaptureSessionSourceId source = CaptureSessionSourceId::Invalid;

    /** @brief Set to true to enable frame synchronization between visible or color and depth. */
    bool frameSyncEnabled = true;

    /** @brief Set to true to deliver IMU events on a separate, dedicated background thread. */
    bool lowLatencyIMU = true;

    /** @brief Set to true to apply a correction and clean filter to the depth before streaming, such that depth frames
        received from the driver are post-processed before arrival. This may effect streaming performance,
        but will provide much better depth data on streamed frames.
        To run correction when needed on-the-fly, set this to false and call DepthFrame::applyExpensiveCorrection.
        @see DepthFrame::applyExpensiveCorrection
    */
    bool applyExpensiveCorrection = false;

    /** @brief StructureCore specific settings. */
    struct ST_API StructureCoreSettings
    {
        /** @brief Set to true to enable depth streaming. */
        bool depthEnabled = true;

        /** @brief Set to true to enable infrared streaming. */
        bool infraredEnabled = false;
        
        /** @brief Set to true to enable visible streaming. */
        bool visibleEnabled = false;
        
        /** @brief Set to true to enable accelerometer streaming. */
        bool accelerometerEnabled = false;
        
        /** @brief Set to true to enable gyroscope streaming. */
        bool gyroscopeEnabled = false;

        /** @brief The target resolution for streamed depth frames. @see StructureCoreDepthResolution */
        StructureCoreDepthResolution depthResolution = StructureCoreDepthResolution::Default;

        /** @brief The preset depth range mode for streamed depth frames. Modifies the min/max range of the depth values. */
        StructureCoreDepthRangeMode depthRangeMode = StructureCoreDepthRangeMode::Default;
    
        /** @brief The target resolution for streamed depth frames. @see StructureCoreInfraredResolution
            Non-default infrared and visible resolutions are currently unavailable.
        */
        StructureCoreInfraredResolution infraredResolution = StructureCoreInfraredResolution::Default;

        /** @brief The target resolution for streamed visible frames. @see StructureCoreVisibleResolution
            Non-default infrared and visible resolutions are currently unavailable.
        */
        StructureCoreVisibleResolution visibleResolution = StructureCoreVisibleResolution::Default;

        /** @brief The demosaicking method to use for color-enabled Structure Core visible frames.
            Will do nothing for monochrome visible frames.
        */
        StructureCoreDemosaicMethod demosaicMethod = StructureCoreDemosaicMethod::Default;

        /** @brief Set to true to apply gamma correction to incoming visible frames. */
        bool visibleApplyGammaCorrection = true;

        /** @brief Enable auto-exposure for infrared frames. */
        bool infraredAutoExposureEnabled = false;

        /** @brief Specifies how to stream the infrared frames. @see StructureCoreInfraredMode */
        StructureCoreInfraredMode infraredMode = StructureCoreInfraredMode::Default;

        /** @brief The target stream rate for IMU data. (gyro and accel) */
        StructureCoreIMUUpdateRate imuUpdateRate = StructureCoreIMUUpdateRate::Default;

        /** @brief Debugging boot mode for firmware builds. */
        StructureCoreBootMode bootMode = StructureCoreBootMode::Default;

        /** @brief Serial number of sensor to stream. If null, the first connected sensor will be used. */
        const char* sensorSerial = nullptr;

        /** @brief Debugging path for firmware builds. */
        const char* firmwareBootPath = nullptr;

        /** @brief Maximum amount of time (in milliseconds) to wait for a sensor to connect before throwing a timeout error. */
        int sensorInitializationTimeout = 6000;

        /** @brief The target framerate for the infrared camera. If the value is not supported, the default is 30. */
        float infraredFramerate = 30.f;

        /** @brief The target framerate for the depth sensor. If the value is not supported, the default is 30. */
        float depthFramerate    = 30.f;

        /** @brief The target framerate for the visible camera. If the value is not supported, the default is 30. */
        float visibleFramerate  = 30.f;

        /** @brief The initial visible exposure to start streaming with (milliseconds, but set in seconds). */
        float initialVisibleExposure = 0.016f;

        /** @brief The initial visible gain to start streaming with. Can be any number between 1 and 8. */
        float initialVisibleGain = 2.0f;

        /** @brief The initial infrared exposure to start streaming with. */
        float initialInfraredExposure = 0.0146f;

        /** @brief The initial infrared gain to start streaming with. Can be 0, 1, 2, or 3. */
        int initialInfraredGain = 3;

        /** @brief Setting this to true will eliminate saturation issues, but might result in sparser depth. */
        bool disableInfraredIntensityBalance = true;

        /** @brief Setting this to true will reduce latency, but might drop more frame */
        bool latencyReducerEnabled = true;

        /** @brief The dynamic calibration mode to use during depth streaming. */
        StructureCoreDynamicCalibrationMode dynamicCalibrationMode = StructureCoreDynamicCalibrationMode::Default;
    }
    structureCore;

    /** @brief OCC specific settings. */
    struct ST_API OCCPlaybackSettings
    {
        /** @brief The path to the OCC file. */
        const char* path = nullptr;

        /** @brief Set to true to enable streaming early color frames. */
        bool earlyColorEnabled = false;

        /** @brief Set to true to restart the OCC from the beginning, running forever until it is stopped. */
        bool autoReplay   = true;

        /** @brief The mode of playback for OCC streaming. @see CaptureSessionOCCPlaybackMode */
        CaptureSessionOCCPlaybackMode playbackMode = CaptureSessionOCCPlaybackMode::Default;

        /** @brief For non-realtime modes of OCC streaming, this will limit the FPS of streaming. */
        float rateLimit    = 30.f;
    }
    occ;

    /** @brief The type of dispatch service to use internally for data delivery. For advanced users only. */
    CaptureSessionDispatcherId dispatcher = CaptureSessionDispatcherId::Default;

    /** @brief The android looper will be auto-created for the thread calling startStreaming, when unspecified. Android-only. */
    ALooper* looper = nullptr;

    /** @brief Attempts to read presaved settings from "[Documents]/CaptureSessionSettings.json". */
    void readSavedSettings();

    /** @brief Writes the current state of the struct into "[Documents]CaptureSessionSettings.json". */
    void persistCaptureSettings();

    /** @brief Returns the estimated minimum and maximum depth values of the input depth range mode, in millimeters. */
    static void minMaxDepthInMmOfDepthRangeMode(StructureCoreDepthRangeMode mode, float& min, float& max);
};

//------------------------------------------------------------------------------

} // ST namespace
