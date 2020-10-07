/*
    CaptureSession.h

    Copyright © 2020 Occipital, Inc. All rights reserved.
    This file is part of the Structure SDK.
    Unauthorized copying of this file, via any medium is strictly prohibited.
    Proprietary and confidential.

    http://structure.io
*/

#pragma once

#include <ST/MathTypes.h>
#include <ST/CaptureSessionTypes.h>

namespace ST
{

//------------------------------------------------------------------------------

struct CaptureSession;
struct CaptureSessionSample;

struct BridgeEngineInternalState;

//------------------------------------------------------------------------------

/** @brief The interface for CaptureSession delegates (CaptureSession::setDelegate). */
struct ST_API CaptureSessionDelegate
{
    virtual ~CaptureSessionDelegate() {}

    /** @brief Gets called when any type of data is streamed from CaptureSession.
        @param session The parent CaptureSession this delegate is attached to.
        @param sample The streamed data. @see CaptureSessionSample
    */
    virtual void captureSessionDidOutputSample(CaptureSession* session, const CaptureSessionSample& sample) = 0;

    /** @brief Gets called when a lone visible frame gets sent through before tracking is running. This can be ignored for most users.
        @param session The parent CaptureSession this delegate is attached to.
        @param sample The streamed data. @see CaptureSessionSample
    */
    virtual void captureSessionDidOutputEarlySample(CaptureSession* session, const CaptureSessionSample& sample) {}

    /** @brief Gets called when an incomplete sample is dropped or streamed from CaptureSession. This can be ignored for most users.
        @param session The parent CaptureSession this delegate is attached to.
        @param sample The streamed data. @see CaptureSessionSample
    */
    virtual void captureSessionDidDropPartiallySynchronizedSample(CaptureSession* session, const CaptureSessionSample& sample) {}

    /** @brief Gets called when the state of streaming changes. @see CaptureSessionEventId
        @param session The parent CaptureSession this delegate is attached to.
        @param event The event identifier.
    */
    virtual void captureSessionEventDidOccur(CaptureSession* session, CaptureSessionEventId event) {}
};

//------------------------------------------------------------------------------

/** @brief The main context for connecting to and streaming data from one or multiple Structure Cores. 
           The sensor information and streaming data of each sensor can be accessed through its CaptureSession object 
           when receiving ST::CaptureSessionEventId::Detected in captureSessionEventDidOccur.

    Example initialization:
    @code{.cpp}

    PerDeviceData* SessionDelegate::getDeviceDataForSession(ST::CaptureSession* session)
    {
        for (PerDeviceData* d : _deviceData)
        {
            if (d->parent == session)
                return d;
        }
        return nullptr;
    }
    struct PerDeviceData
    {
        std::atomic<ST::CaptureSessionEventId> lastCaptureSessionEvent { ST::CaptureSessionEventId::Disconnected };
        std::mutex sampleLock;
        ST::InfraredFrame lastInfraredFrame;
        ST::DepthFrame lastDepthFrame;
        ST::ColorFrame lastVisibleOrColorFrame;
        
        ST::CaptureSession* parent;
        ST::OCCFileWriter occWriter;
    };

    void captureSessionEventDidOccur(CaptureSession* session, CaptureSessionEventId event) {
        switch (event) {
        case ST::CaptureSessionEventId::Detected:
            // store session for accessing data from each sensor
            PerDeviceData* data = new PerDeviceData();
            data->parent = session;
            _deviceData.push_back(data);
            std::cout<<"Added newly detected sensor! \n";
            break;
        case ST::CaptureSessionEventId::Connected:
            // a sensor was detected and connected successfully! we can start streaming now
            session->startStreaming();
            break;
        case ST::CaptureSessionEventId::Ready:
            // the sensor is in a wait state, usually when streaming is stopped after starting
            // here, we can stream or request information, such as the serial number, etc.
            break;
        case ST::CaptureSessionEventId::Disconnected:
            // a connected sensor was disconnected, any operations on the CaptureSession will
            // most likely fail. In the event of a reboot, this is expected.
            break;
        }
    }
    struct MyCaptureSessionDelegate : public ST::CaptureSessionDelegate {
        void captureSessionDidOutputSample (CaptureSession* session, const CaptureSessionSample& sample) override {
            PerDeviceData* data = getDeviceDataForSession(session);
            if (!data)
            {
                GuiSupport::log("getDeviceDataForSession returned nullptr.");
                return;
            }
            std::unique_lock<std::mutex> u(data->sampleLock);
            {
                case ST::CaptureSessionSample::Type::SynchronizedFrames:
                    // we have sync'd frames!
                    if (sample.depthFrame.isValid())
                    {
                        data->lastDepthFrame = sample.depthFrame;
                        data->depthRateMonitor.tick();
                    }

                    if (sample.infraredFrame.isValid())
                    {
                        data->lastInfraredFrame = sample.infraredFrame;
                        data->infraredRateMonitor.tick();
                    }

                    if (sample.visibleFrame.isValid())
                    {
                        data->lastVisibleOrColorFrame = sample.visibleFrame;
                    }
                    break;
                case ST::CaptureSessionSample::Type::AccelerometerEvent:
                    // we have accel data!
                    break;
                case ST::CaptureSessionSample::Type::GyroscopeEvent:
                    // we have gyro data!
                    break;
                };
            }
            u.unlock();
        }
    };

    // configure the options to stream everything needed for tracking
    ST::CaptureSessionSettings settings;
    settings.structureCore.depthEnabled         = true;
    settings.structureCore.visibleEnabled       = true;
    settings.structureCore.accelerometerEnabled = true;
    settings.structureCore.gyroscopeEnabled     = true;

    // create your delegate class
    MyCaptureSessionDelegate delegate;

    // attach the delegate to a CaptureSession
    ST::CaptureSession captureSession;
    captureSession.setDelegate(&delegate);

    // apply settings and start streaming!
    captureSession.startMonitoring(settings);
    captureSession.startStreaming();
    @endcode
*/
struct ST_API CaptureSession
{
    /** @brief Constructor. */
     CaptureSession();
    ~CaptureSession();

    /** @brief Attach a CaptureSessionDelegate to send output samples to. The delegate must be set prior to configuring the CaptureSession (@ref startMonitoring). */
    void setDelegate(CaptureSessionDelegate* delegate);

    /** @brief Initializes the CaptureSession and prepares it to start streaming data. Must be called at least once before streaming is started.
        This function is non-blocking and runs asynchronously.
        Once successful, the captureSessionEventDidOccur delegate will receive CaptureSessionEventId::Connected.
        If an error occurs, the captureSessionEventDidOccur delegate will receive CaptureSessionEventId::Error or the specific error case enum.
        @param settings Settings for the capture session
        @return True, if able to initialize the session using the provided settings.
        @see CaptureSessionDelegate::captureSessionEventDidOccur
    */
    bool startMonitoring(const CaptureSessionSettings& settings);

    /** @brief Attempt to start streaming data from a specified source. Streaming will start soon after this function is called.
        This function is non-blocking and runs asynchronously.
        Sample data is delivered to the delegate via CaptureSessionDelegate::captureSessionDidOutputSample.
        Once successful, the captureSessionEventDidOccur delegate will receive CaptureSessionEventId::Streaming.
        If an error occurs, the captureSessionEventDidOccur delegate method will receive CaptureSessionEventId::Error or the specific error case enum.
        @return True, if able to initialize and a timeout was not hit. NOTE: This does not mean the device is streaming, not until the captureSessionEventDidOccur delegate receives CaptureSessionEventId::Streaming.
        @see CaptureSessionDelegate::captureSessionEventDidOccur
    */
    bool startStreaming();

    /** @brief Stop streaming from the specified source.
        Unlike the start functions, this function runs synchronously and will block until the device has successfully stopped streaming.
        Once successful, the captureSessionEventDidOccur delegate will receive CaptureSessionEventId::Ready.
        If an error occurs, the captureSessionEventDidOccur delegate will receive CaptureSessionEventId::Error or the specific error case enum.
    */
    void stopStreaming();

    /** @brief Re-initialize streaming from the specified source.
       This function is non-blocking and runs asynchronously.
        Note that a Structure Core will be disconnected entirely for a brief moment after.
        Once successful, the captureSessionEventDidOccur delegate will receive CaptureSessionEventId::Disconnected, then CaptureSessionEventId::Booting, and finally CaptureSessionEventId::Connected once the device has successfully rebooted.
        @return True, if able to reboot the source. Otherwise, false.
    */
    bool rebootCaptureSource();

    /** @brief Obtain information about the sensor this CaptureSession is using. */
    const CaptureSessionSensorInfo& sensorInfo() const;

    /** @brief Returns the extrinsic transformation between the IMU and the depth camera - convention of the returned matrix is "IMU_centroid from depth".
     	This function requires a Structure Core to be connected and initialized. @see initialize
    */
    const Matrix4 getImuFromDepthExtrinsics () const;

    /** @brief Returns the extrinsic transformation between the IMU and the visible camera - convention of the returned matrix is "IMU_centroid from visible".
    	This function requires a Structure Core to be connected and initialized. @see initialize
    */
    const Matrix4 getImuFromVisibleExtrinsics () const;


    /** @brief Return the USB connection version between the host and Structure Core. Full streaming functionality is only available under CaptureSessionUSBVersion::USB3. */
    CaptureSessionUSBVersion USBVersion() const;

    /** @brief Get the camera type of the Structure Core sensor. */
    ST::StructureCoreCameraType getCameraType();

    /** @brief Set the exposure and gain of the visible frames from a Structure Core. */
    void setVisibleCameraExposureAndGain(float exposure, float gain);

    /** @brief Get the exposure and gain of the visible frames from a Structure Core. */
    void getVisibleCameraExposureAndGain(float* exposure, float* gain) const;

    /** @brief Set the exposure and gain of the infrared frames from a Structure Core. */
    void setInfraredCamerasExposureAndGain(float exposure, float gain);

    /** @brief Get the exposure and gain of the infrared frames from a Structure Core. */
    void getInfraredCamerasExposureAndGain(float* exposure, float* gain) const;

    /** @brief Returns the settings provided during initialization. */
    const CaptureSessionSettings& settings() const;

    /** @brief Set numerical property. */
    void setNumericalProperty(CaptureSessionProperty property, bool value);

    /** @brief Get numerical property. */
    bool getNumericalProperty(CaptureSessionProperty property) const;

    void persistExposureAndGainSettings(bool persist_infrared);
    void clearSavedSettings();

    /** @brief When doing real-time OCC playback, return the offset between the original OCC timestamps and the real-time ones. */
    double getOccFromSystemTimeOffset() const;

    ST_DECLARE_OPAQUE_INTERNALS(CaptureSession);
};

//------------------------------------------------------------------------------

}
