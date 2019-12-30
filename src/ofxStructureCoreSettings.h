#pragma once
#include "ST/CameraFrames.h"
#include "ST/CaptureSession.h"
#include "ST/IMUEvents.h"
#include "ST/OCCFileWriter.h"
#include "ST/Utilities.h"

namespace ofx {
namespace structure {
	struct Settings : public ST::CaptureSessionSettings
	{
		// simpler, overloaded interface:

		const char*& deviceId = structureCore.sensorSerial;  // ref to the char array

		bool& enableDepth   = structureCore.depthEnabled;
		bool& enableVisible = structureCore.visibleEnabled;
		bool& enableIr      = structureCore.infraredEnabled;
		bool& enableAcc     = structureCore.accelerometerEnabled;
		bool& enableGyro    = structureCore.gyroscopeEnabled;

		float& depthFps   = structureCore.depthFramerate;
		float& irFps      = structureCore.infraredFramerate;
		float& visibleFps = structureCore.visibleFramerate;

		bool& irAutoExposure = structureCore.infraredAutoExposureEnabled;  // false
		float& irExposure    = structureCore.initialInfraredExposure;      // 0.0146f
		int& irGain          = structureCore.initialInfraredGain;          // 3

		// enum / modes:
		using DepthResolution = ST::StructureCoreDepthResolution;         // SXGA = _1280x960, VGA = _640x480 (default), QVGA = _320x240
		using DepthRangeMode  = ST::StructureCoreDepthRangeMode;          // Short, Medium, Long, Hybrid, etc.
		using IRMode          = ST::StructureCoreInfraredMode;            // LeftCameraOnly, RightCameraOnly, BothCameras (default, 2x1 img)
		using CalibrationMode = ST::StructureCoreDynamicCalibrationMode;  // Off (default), OneShotPersistent, ContinuousNonPersistent
		using IMURate         = ST::StructureCoreIMUUpdateRate;           // AccelAndGyro_800Hz (default), etc.

		DepthResolution& depthResolution = structureCore.depthResolution;
		DepthRangeMode& depthRangeMode   = structureCore.depthRangeMode;
		IRMode& irMode                   = structureCore.infraredMode;
		CalibrationMode& calibrationMode = structureCore.dynamicCalibrationMode;
		IMURate& imuRate                 = structureCore.imuUpdateRate;

		// convert depth range mode to est. min / max range in millimeters
		static void rangeToMM( DepthRangeMode mode, float& min, float& max )
		{
			ST::CaptureSessionSettings::minMaxDepthInMmOfDepthRangeMode( mode, min, max );
		}

		/* more options inside structureCore.
			{
				ST::StructureCoreDemosaicMethod demosaicMethod                 = ST::StructureCoreDemosaicMethod::Default;
				ST::StructureCoreBootMode bootMode                             = ST::StructureCoreBootMode::Default;
				const char* firmwareBootPath                                   = nullptr;
				int sensorInitializationTimeout                                = 6000;
				float initialVisibleExposure                                   = 0.016f;
				float initialVisibleGain                                       = 2.0f;
				bool visibleApplyGammaCorrection                               = true;
				bool disableInfraredIntensityBalance                           = true;
				bool latencyReducerEnabled                                     = true;
			}
			*/

		Settings()
		{
			// capture device
			source = ST::CaptureSessionSourceId::StructureCore;  // todo: stream from OCC file

			//deviceId = "001";  // set to serial (char*) to specify a device, otherwise first available

			// defaults:
			enableDepth     = true;
			enableVisible   = true;
			enableIr        = true;
			depthResolution = DepthResolution::_1280x960;  // SXGA
			depthRangeMode  = DepthRangeMode::Hybrid;
			enableAcc       = false;
			enableGyro      = false;

			//calibrationMode = CalibrationMode::OneShotPersistent;  // re-calibrate on device boot
		}

		// todo: addt'l settings?
	};
}  // namespace structure
}  // namespace ofx
