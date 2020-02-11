#pragma once
#include "ST/CameraFrames.h"
#include "ST/CaptureSession.h"
#include "ST/IMUEvents.h"
#include "ST/OCCFileWriter.h"
#include "ST/Utilities.h"

namespace ofx {
namespace structure {

	// map settings types to string
	template <typename T>
	inline const std::map<T, std::string>& type_to_str_map()
	{
		return {};
	}

	template <>
	inline const std::map<ST::StructureCoreDepthResolution, std::string>& type_to_str_map()
	{
		static const std::map<ST::StructureCoreDepthResolution, std::string> names = {
		    { ST::StructureCoreDepthResolution::_320x240, "320x240" },
		    { ST::StructureCoreDepthResolution::_640x480, "640x480" },
		    { ST::StructureCoreDepthResolution::_1280x960, "1280x960" },
		    { ST::StructureCoreDepthResolution::VGA, "VGA" },
		    { ST::StructureCoreDepthResolution::SXGA, "SXGA" },
		    { ST::StructureCoreDepthResolution::QVGA, "QVGA" } };
		return names;
	};
	template <>
	inline const std::map<ST::StructureCoreDepthRangeMode, std::string>& type_to_str_map()
	{
		static const std::map<ST::StructureCoreDepthRangeMode, std::string> names = {
		    { ST::StructureCoreDepthRangeMode::Short, "Short" },
		    { ST::StructureCoreDepthRangeMode::Medium, "Medium" },
		    { ST::StructureCoreDepthRangeMode::Long, "Long" },
		    { ST::StructureCoreDepthRangeMode::Hybrid, "Hybrid" } };
		return names;
	};
	template <>
	inline const std::map<ST::StructureCoreDynamicCalibrationMode, std::string>& type_to_str_map()
	{
		static const std::map<ST::StructureCoreDynamicCalibrationMode, std::string> names = {
		    { ST::StructureCoreDynamicCalibrationMode::Off, "Off" },
		    { ST::StructureCoreDynamicCalibrationMode::OneShotPersistent, "OneShotPersistent" },
		    { ST::StructureCoreDynamicCalibrationMode::ContinuousNonPersistent, "ContinuousNonPersisent" } };
		return names;
	};

	template <typename T>
	inline std::string to_string( T type )
	{
		try {
			return type_to_str_map<T>().at( type );
		} catch ( ... ) {}
		return "";
	}
	template <typename T>
	inline T to_type( std::string str, const T& default_value )
	{
		for ( auto& type : type_to_str_map<T>() ) {
			if ( type.second == str ) {
				return type.first;
			}
			return default_value;
		}
	}

	struct Settings : public ST::CaptureSessionSettings
	{
		void setSerial(const std::string& serial) {
			auto len = serial.length();
			char * tmp_str = new char[len + 1];
			strncpy(tmp_str, serial.c_str(), len);
			tmp_str[len] = '\0'; // enfore terminate
			structureCore.sensorSerial = tmp_str;
		}

		// enum / modes:
		using DepthResolution = ST::StructureCoreDepthResolution;         // SXGA = _1280x960, VGA = _640x480 (default), QVGA = _320x240
		using DepthRangeMode  = ST::StructureCoreDepthRangeMode;          // Short, Medium, Long, Hybrid, etc.
		using IRMode          = ST::StructureCoreInfraredMode;            // LeftCameraOnly, RightCameraOnly, BothCameras (default, 2x1 img)
		using CalibrationMode = ST::StructureCoreDynamicCalibrationMode;  // Off (default), OneShotPersistent, ContinuousNonPersistent
		using IMURate         = ST::StructureCoreIMUUpdateRate;           // AccelAndGyro_800Hz (default), etc.

		// convert depth range mode to est. min / max range in millimeters
		static void rangeToMM( DepthRangeMode mode, float& min, float& max )
		{
			ST::CaptureSessionSettings::minMaxDepthInMmOfDepthRangeMode( mode, min, max );
		}

		// more options inside structureCore.
		//{
		//	ST::StructureCoreDemosaicMethod demosaicMethod = ST::StructureCoreDemosaicMethod::Default;
		//	ST::StructureCoreBootMode bootMode             = ST::StructureCoreBootMode::Default;
		//	const char* firmwareBootPath                   = nullptr;
		//	int sensorInitializationTimeout                = 6000;
		//	float initialVisibleExposure                   = 0.016f;
		//	float initialVisibleGain                       = 2.0f;
		//	bool visibleApplyGammaCorrection               = true;
		//	bool disableInfraredIntensityBalance           = true;
		//	bool latencyReducerEnabled                     = true;
		//}

		Settings()
		{
			// capture device type
			source = ST::CaptureSessionSourceId::StructureCore;  // todo: stream from OCC file

			//deviceId = "001";  // (char*) set to serial to specify a device, otherwise first available

			// defaults:
			structureCore.depthEnabled         = true;
			structureCore.visibleEnabled       = true;
			structureCore.infraredEnabled      = true;
			structureCore.depthResolution      = DepthResolution::_1280x960;  // SXGA
			structureCore.depthRangeMode       = DepthRangeMode::Medium;      // 0.52m - 5.23m
			structureCore.accelerometerEnabled = false;
			structureCore.gyroscopeEnabled     = false;
			applyExpensiveCorrection           = true;  // apply expensive depth correction to stream

			//calibrationMode = CalibrationMode::OneShotPersistent;  // re-calibrate on device boot
		}

		// todo: addt'l settings?
	};
}  // namespace structure
}  // namespace ofx
