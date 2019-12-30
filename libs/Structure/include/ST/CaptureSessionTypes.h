/*
    CaptureSessionTypes.h

    Copyright © 2017 Occipital, Inc. All rights reserved.
    This file is part of the Structure SDK.
    Unauthorized copying of this file, via any medium is strictly prohibited.
    Proprietary and confidential.

    http://structure.io
*/

#pragma once

#include <ST/Macros.h>
#include <ST/IMUEvents.h>
#include <ST/CameraFrames.h>
#include <ST/CaptureSessionSettings.h>

#include <string.h>

namespace ST
{

// forward declarations
struct CaptureSession;
struct CaptureSessionSample;

//------------------------------------------------------------------------------

/** @brief Events describing the internal state changes of the CaptureSession. */
enum class CaptureSessionEventId
{
    /** Hopefully, shouldn't ever get used. */
    Unknown,

    /** Sensor is connected. */
    Connected,

    /** Sensor is booting up. */
    Booting,

    /** Sensor is ready to stream. */
    Ready,

    /** Sensor was disconnected. */
    Disconnected,

    /** An error happened internally. */
    Error,

    /** A USB error happened internally. */
    UsbError,

    /** Sensor has entered low power mode */
    LowPowerMode,

    /** Sensor has entered recovery mode. (for firmware flashing, etc.) */
    RecoveryMode,

    /** Sensor has corrupt production line data. Please contact Occipital if this happens. */
    ProdDataCorrupt,

    /** Calibration for the sensor is either missing or invalid. */
    CalibrationMissingOrInvalid,

    /** Sensor has outdated firmware. */
    FWVersionMismatch,

    /** Sensor is attempting to update firmware from a BIN file. */
    FWUpdate,

    /** The sensor firmware update completed successfully. */
    FWUpdateComplete,

    /** The sensor failed to update firmware. */
    FWUpdateFailed,

    /** The sensor has corrupt firmware. Please contact Occipital if this happens. */
    FWCorrupt,

    /** End of file has been reached when replaying data. */
    EndOfFile,

    /** The USB driver for Structure Core was not detected. Should only appear on Windows. */
    USBDriverNotInstalled,

    /** The sensor was configured correctly and is streaming data successfully. */
    Streaming,
};

/** @brief Describes the USB version and speed by which a Structure Core sensor is connected. */
enum class CaptureSessionUSBVersion
{
    /** Unknown USB interface version. */
    Unknown,

    /** USB 1.x interface (Low Speed 1.5 Mbps or Full Speed 12 Mbps). Streaming is unlikely to function in this mode. */
    USB1,

    /** USB 2.0 interface (High Speed 480 Mbps). Not all streams can be used simultaneously at full sample rate in this mode. It is possible to use either a subset of streams at full sample rate or all streams at reduced sample rate. Sample latency may be higher than in USB3 mode. */
    USB2,

    /** USB 3.x interface (SuperSpeed 5+ Gbps or better). Preferred. All streams are supported at full sample rate in this mode, provided the host USB interface has sufficient bandwidth available for bulk transfers. */
    USB3,
};

/** @brief Information about the Structure Core sensor currently in use by a CaptureSession. See CaptureSession::sensorInfo. */
struct ST_API CaptureSessionSensorInfo
{
    CaptureSessionSensorInfo()
    {
        strcpy(serialNumber, "unknown");
    }

    struct FirmwareVersion
    {
        bool valid = false;
        uint32_t major    = 0;
        uint32_t minor    = 0;
        uint32_t revision = 0;
    };
    struct
    {
        FirmwareVersion driverFirmwareVersion;
        FirmwareVersion sensorFirmwareVersion;
    } structureCore;

    char serialNumber[256];
};

//------------------------------------------------------------------------------

struct ST_API CaptureSessionSample
{
    /** @brief CaptureSessionSample::type values. */
    enum class Type
    {
        Invalid = -1,

        /** @brief Structure Core accelerometer sample. When this type is received, the @ref accelerometerEvent field is valid. */
        AccelerometerEvent,

        /** @brief Structure Core gyroscope sample. When this type is received, the @ref gyroscopeEvent field is valid. */
        GyroscopeEvent,

        /** @brief Structure Core infrared frame. When this type is received, the @ref infraredFrame field is valid. */
        InfraredFrame,

        /** @brief Structure Core depth frame. When this type is received, the @ref depthFrame field is valid. */
        DepthFrame,

        /** @brief Structure Core monochrome or color visible camera frame. When this type is received, the @ref visibleFrame field is valid. */
        VisibleFrame,

        /** @brief External device color camera frame. Not applicable to Structure Core. When this type is received, the @ref externalColorFrame field is valid. */
        ExternalColorFrame,

        /** @brief Synchronized Structure Core frames. When this type is received, at least two of the @ref depthFrame, @ref visibleFrame, and @ref infraredFrame fields are valid, depending on streaming configuration. */
        SynchronizedFrames,

        /** @brief Set of images from multiple cameras. Not applicable to Structure Core. When this type is received, the @ref multiCameraColorFrame field is valid. */
        MultiCameraColorFrame,

        HowMany
    };

    /** @brief The type of the sample; dictates which field(s) are valid. */
    Type type = Type::Invalid;

    /** @brief Structure Core gyroscope sample. Valid if @ref type is Type::GyroscopeEvent. */
    GyroscopeEvent gyroscopeEvent;

    /** @brief Structure Core accelerometer sample. Valid if @ref type is Type::AccelerometerEvent. */
    AccelerometerEvent accelerometerEvent;

    /** @brief Structure Core depth frame. Valid if @ref type is Type::DepthFrame. May be valid if @ref type is Type::SynchronizedFrames, depending on streaming configuration. */
    DepthFrame depthFrame;

    /** @brief Structure Core left, right, or stereo infrared frame. Valid if @ref type is Type::InfraredFrame. May be valid if @ref type is Type::SynchronizedFrames, depending on streaming configuration. */
    InfraredFrame infraredFrame;

    /** @brief Structure Core monochrome or color visible camera frame, depending on sensor model. Valid if @ref type is Type::VisibleFrame. May be valid if @ref type is Type::SynchronizedFrames, depending on streaming configuration. */
    ColorFrame visibleFrame;

    /** @brief External device color camera frame. Not applicable to Structure Core. Valid if @ref type is Type::ColorFrame. */
    ColorFrame externalColorFrame;

    /** @brief Set of images from multiple cameras. Not applicable to Structure Core. Valid if @ref type is Type::MultiCameraColorFrame. */
    MultiCameraColorFrame multiCameraColorFrame;

    /** Convert the enum to a readable string. @see CaptureSessionSample::Type */
    static const char* toString(CaptureSessionSample::Type type);

    /** @brief Convert the enum to a readable string. @see CaptureSessionEventId */
    static const char* toString(CaptureSessionEventId state);

    /** @brief True if the sample represents image data or synchronized images. False for IMU samples. */
    bool isImage() const;

    /** @brief Construct an empty CaptureSessionSample. */
    CaptureSessionSample();
    ~CaptureSessionSample();

    /** @brief Construct a CaptureSessionSample as a shallow copy of another CaptureSessionSample. */
    CaptureSessionSample(const CaptureSessionSample& other);

    void* reserved;
};

//------------------------------------------------------------------------------

} // ST namespace
