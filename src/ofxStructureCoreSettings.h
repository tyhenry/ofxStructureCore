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
		    { ST::StructureCoreDepthRangeMode::VeryShort, "VeryShort" },  // 0.35 - 0.92m
		    { ST::StructureCoreDepthRangeMode::Short, "Short" },          // 0.41 - 1.36m
		    { ST::StructureCoreDepthRangeMode::Medium, "Medium" },        // 0.52 - 5.23m
		    { ST::StructureCoreDepthRangeMode::Long, "Long" },            // 0.58 - 8.0m
		    { ST::StructureCoreDepthRangeMode::VeryLong, "VeryLong" },    // 0.58 - 10.0m
		    { ST::StructureCoreDepthRangeMode::Hybrid, "Hybrid" } };      // 0.35 - 10.0m
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
	protected:
		std::string _serial;

	public:
		Settings( const Settings& other )
		    : ST::CaptureSessionSettings( other )
		{
			setSerial( other._serial );
		}

		Settings& operator=( const Settings& other )
		{
			ST::CaptureSessionSettings::operator=( other );
			setSerial( other._serial );
			return *this;
		}

		void setSerial( const std::string& serial )
		{
			_serial = serial;
			if ( !_serial.empty() ) {
				structureCore.sensorSerial = _serial.c_str();
			} else {
				structureCore.sensorSerial = nullptr;  // first available
			}
		}
		std::string getSerial() const
		{
			return structureCore.sensorSerial ? std::string( structureCore.sensorSerial ) : "";
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

		Settings()
		{
			// capture device type
			source = ST::CaptureSessionSourceId::StructureCore;  // todo: stream from OCC file

			// defaults:

			structureCore.depthEnabled           = true;
			structureCore.visibleEnabled         = true;
			structureCore.infraredEnabled        = true;
			structureCore.depthResolution        = DepthResolution::_1280x960;  // SXGA
			structureCore.depthRangeMode         = DepthRangeMode::Medium;      // 0.52m - 5.23m
			structureCore.accelerometerEnabled   = false;
			structureCore.gyroscopeEnabled       = false;
			structureCore.dynamicCalibrationMode = CalibrationMode::OneShotPersistent;  // re-calibrate on device boot
			applyExpensiveCorrection             = true;                                // apply expensive depth correction to stream

			// more options:

			//structureCore.demosaicMethod                  = ST::StructureCoreDemosaicMethod::Default;  // ST::StructureCoreDemosaicMethod
			//structureCore.bootMode                        = ST::StructureCoreBootMode::Default;        // ST::StructureCoreBootMode
			//structureCore.firmwareBootPath                = nullptr;                                   // const char*
			//structureCore.sensorInitializationTimeout     = 6000;                                      // int
			//structureCore.initialVisibleExposure          = 0.016f;                                    // float
			//structureCore.initialVisibleGain              = 2.0f;                                      // float
			//structureCore.visibleApplyGammaCorrection     = true;                                      // bool
			//structureCore.disableInfraredIntensityBalance = true;                                      // bool
			//structureCore.latencyReducerEnabled           = true;                                      // bool
		}

		// todo: addt'l settings?
	};
}  // namespace structure
}  // namespace ofx
