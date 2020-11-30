#pragma once
#include "ST/CameraFrames.h"
#include "ST/CaptureSession.h"
#include "ST/IMUEvents.h"
#include "ST/OCCFileWriter.h"
#include "ST/Utilities.h"
#include "ofJson.h"

namespace ofx {
namespace structure {

	struct Settings : public ST::CaptureSessionSettings
	{
	protected:
		std::string _serial;

	public:
		Settings( const Settings& other )
		    : ST::CaptureSessionSettings( other ), generatePointCloud( other.generatePointCloud )
		{
			setSerial( other._serial );
		}

		Settings& operator=( const Settings& other )
		{
			ST::CaptureSessionSettings::operator=( other );
			generatePointCloud                  = other.generatePointCloud;
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

		bool generatePointCloud = false;

		// todo: addt'l settings?
	};

	// SERIALIZATION HELPERS

#define OFXSTRUCTURECORE_SETTINGS_FIELDS                                    \
	X_BOOL( structureCore.depthEnabled )                                    \
	X_ENUM( structureCore.depthResolution, QVGA )                           \
	X_ENUM( structureCore.depthResolution, VGA )                            \
	X_ENUM( structureCore.depthResolution, SXGA )                           \
	X_FLOAT( structureCore.depthFramerate )                                 \
	X_ENUM( structureCore.depthRangeMode, VeryShort )                       \
	X_ENUM( structureCore.depthRangeMode, Short )                           \
	X_ENUM( structureCore.depthRangeMode, Medium )                          \
	X_ENUM( structureCore.depthRangeMode, Long )                            \
	X_ENUM( structureCore.depthRangeMode, VeryLong )                        \
	X_ENUM( structureCore.depthRangeMode, Hybrid )                          \
	X_ENUM( structureCore.depthRangeMode, Default )                         \
	X_BOOL( applyExpensiveCorrection )                                      \
	X_ENUM( structureCore.dynamicCalibrationMode, Off )                     \
	X_ENUM( structureCore.dynamicCalibrationMode, OneShotPersistent )       \
	X_ENUM( structureCore.dynamicCalibrationMode, ContinuousNonPersistent ) \
                                                                            \
	X_BOOL( structureCore.visibleEnabled )                                  \
	X_FLOAT( structureCore.visibleFramerate )                               \
	X_FLOAT( structureCore.initialVisibleExposure )                         \
	X_FLOAT( structureCore.initialVisibleGain )                             \
                                                                            \
	X_BOOL( structureCore.infraredEnabled )                                 \
	X_FLOAT( structureCore.infraredFramerate )                              \
	X_FLOAT( structureCore.initialInfraredExposure )                        \
	X_INT( structureCore.initialInfraredGain )                              \
	X_ENUM( structureCore.infraredMode, LeftCameraOnly )                    \
	X_ENUM( structureCore.infraredMode, RightCameraOnly )                   \
	X_ENUM( structureCore.infraredMode, BothCameras )                       \
                                                                            \
	X_BOOL( frameSyncEnabled )                                              \
                                                                            \
	X_BOOL( structureCore.accelerometerEnabled )                            \
	X_BOOL( structureCore.gyroscopeEnabled )                                \
	X_ENUM( structureCore.imuUpdateRate, AccelAndGyro_100Hz )               \
	X_ENUM( structureCore.imuUpdateRate, AccelAndGyro_200Hz )               \
	X_ENUM( structureCore.imuUpdateRate, AccelAndGyro_400Hz )               \
	X_ENUM( structureCore.imuUpdateRate, AccelAndGyro_1000Hz )              \
                                                                            \
	X_BOOL( generatePointCloud )

	namespace serialize {

		//// map settings types to string
		//template <typename T>
		//inline const std::map<T, std::string>& enum_to_str_map()
		//{
		//	static_assert( false, "No string map for this enum type!" );
		//	return {};
		//}

		//template <>
		//inline const std::map<ST::StructureCoreDepthResolution, std::string>& enum_to_str_map()
		//{
		//	static const std::map<ST::StructureCoreDepthResolution, std::string> names = {
		//	    { ST::StructureCoreDepthResolution::_320x240, "320x240" },
		//	    { ST::StructureCoreDepthResolution::_640x480, "640x480" },
		//	    { ST::StructureCoreDepthResolution::_1280x960, "1280x960" },
		//	    { ST::StructureCoreDepthResolution::QVGA, "QVGA" },
		//	    { ST::StructureCoreDepthResolution::VGA, "VGA" },
		//	    { ST::StructureCoreDepthResolution::SXGA, "SXGA" } };
		//	return names;
		//};

		//template <>
		//inline const std::map<ST::StructureCoreDepthRangeMode, std::string>& enum_to_str_map()
		//{
		//	static const std::map<ST::StructureCoreDepthRangeMode, std::string> names = {
		//	    { ST::StructureCoreDepthRangeMode::VeryShort, "VeryShort" },  // 0.35 - 0.92m
		//	    { ST::StructureCoreDepthRangeMode::Short, "Short" },          // 0.41 - 1.36m
		//	    { ST::StructureCoreDepthRangeMode::Medium, "Medium" },        // 0.52 - 5.23m
		//	    { ST::StructureCoreDepthRangeMode::Long, "Long" },            // 0.58 - 8.0m
		//	    { ST::StructureCoreDepthRangeMode::VeryLong, "VeryLong" },    // 0.58 - 10.0m
		//	    { ST::StructureCoreDepthRangeMode::Hybrid, "Hybrid" } };      // 0.35 - 10.0m
		//	return names;
		//};

		//template <>
		//inline const std::map<ST::StructureCoreDynamicCalibrationMode, std::string>& enum_to_str_map()
		//{
		//	static const std::map<ST::StructureCoreDynamicCalibrationMode, std::string> names = {
		//	    { ST::StructureCoreDynamicCalibrationMode::Off, "Off" },
		//	    { ST::StructureCoreDynamicCalibrationMode::OneShotPersistent, "OneShotPersistent" },
		//	    { ST::StructureCoreDynamicCalibrationMode::ContinuousNonPersistent, "ContinuousNonPersisent" } };
		//	return names;
		//};

		//template <>
		//inline const std::map<ST::StructureCoreIMUUpdateRate, std::string>& enum_to_str_map()
		//{
		//	static const std::map<ST::StructureCoreIMUUpdateRate, std::string> names = {
		//	    { ST::StructureCoreIMUUpdateRate::AccelAndGyro_100Hz, "AccelAndGyro_100Hz" },
		//	    { ST::StructureCoreIMUUpdateRate::AccelAndGyro_200Hz, "AccelAndGyro_200Hz" },
		//	    { ST::StructureCoreIMUUpdateRate::AccelAndGyro_400Hz, "AccelAndGyro_400Hz" },
		//	    { ST::StructureCoreIMUUpdateRate::AccelAndGyro_1000Hz, "AccelAndGyro_1000Hz" } };
		//	return names;
		//};

		//template <typename T>
		//inline std::string to_string( T enum_ )
		//{
		//	try {
		//		return enum_to_str_map<T>().at( type );
		//	} catch ( ... ) {}
		//	return "";
		//}
		//template <typename T>
		//inline T to_enum( const std::string& str, const T& default_value )
		//{
		//	for ( auto& type : enum_to_str_map<T>() ) {
		//		if ( type.second == str ) {
		//			return type.first;
		//		}
		//		return default_value;
		//	}
		//}

		template <typename T>
		inline void loadEnumFromJSON( const ofJson& json, T& field, const std::string& key, const std::string& stringForEnumValue, T enumValue )
		{
			if ( json.find( key ) != json.end() && json[key].is_string() && json[key].get<std::string>() == stringForEnumValue ) {
				field = enumValue;
			}
		}

		inline void loadBoolFromJSON( const ofJson& json, bool& field, const std::string& key )
		{
			if ( json.find( key ) != json.end() && json[key].is_boolean() ) {
				field = json[key].get<bool>();
			}
		}

		inline void loadIntFromJSON( const ofJson& json, int& field, const std::string& key )
		{
			if ( json.find( key ) != json.end() && json[key].is_number() ) {
				field = json[key].get<int>();
			}
		}

		inline void loadFloatFromJSON( const ofJson& json, float& field, const std::string& key )
		{
			if ( json.find( key ) != json.end() && json[key].is_number() ) {
				field = json[key].get<float>();
			}
		}

		inline void loadStringFromJSON( const ofJson& json, std::string& field, const std::string& key )
		{
			if ( json.find( key ) != json.end() && json[key].is_string() ) {
				field = json[key].get<std::string>();
			}
		}

		template <typename T>
		inline void saveEnumToJSON( ofJson& json, const T& field, const std::string& key, const std::string& stringForEnumValue, T enumValue )
		{
			if ( field == enumValue ) {
				json[key] = stringForEnumValue;
			}
		}

		inline void saveBoolToJSON( ofJson& json, const bool& field, const std::string& key )
		{
			json[key] = field;
		}

		inline void saveIntToJSON( ofJson& json, const int& field, const std::string& key )
		{
			json[key] = field;
		}

		inline void saveFloatToJSON( ofJson& json, const float& field, const std::string& key )
		{
			json[key] = field;
		}

		inline void saveStringToJSON( ofJson& json, const std::string& field, const std::string& key )
		{
			json[key] = field;
		}
	}  // namespace serialize

	// json serialization
	//void to_json( ofJson& json, const Settings& settings )
	//{
	//	//json = {
	//	//    { "serial", settings.getSerial() },
	//	//    { "generatePointCloud", settings.generatePointCloud },                    // bool
	//	//    { "visibleEnabled", settings.structureCore.visibleEnabled },              // bool
	//	//    { "infraredEnabled", settings.structureCore.infraredEnabled },            // bool
	//	//    { "depthEnabled", settings.structureCore.depthEnabled },                  // bool
	//	//    { "accelerometerEnabled", settings.structureCore.accelerometerEnabled },  // bool
	//	//    { "gyroscopeEnabled", settings.structureCore.gyroscopeEnabled },          // bool
	//	//    { "frameSyncEnabled", settings.frameSyncEnabled },                        // bool
	//	//    { "depthResolution", serialize::to_string( settings.structureCore.depthResolution ) },
	//	//    { "depthRangeMode", serialize::to_string( settings.structureCore.depthRangeMode ) },
	//	//    { "dynamicCalibrationMode", serialize::to_string( settings.structureCore.dynamicCalibrationMode ) },
	//	//    { "imuUpdateRate", serialize::to_string( settings.structureCore.imuUpdateRate ) },
	//	//    { "initialInfraredExposure", settings.structureCore.initialInfraredExposure },                 // float
	//	//    { "initialInfraredGain", settings.structureCore.initialInfraredGain },                         // float
	//	//    { "diableInfraredIntensityBalance", settings.structureCore.disableInfraredIntensityBalance },  // bool
	//	//    { "initialVisibleExposure", settings.structureCore.initialVisibleExposure },                   // float
	//	//    { "initialVisibleGain", settings.structureCore.initialVisibleGain },                           // float
	//	//    { "visibleApplyGammaCorrection", settings.structureCore.visibleApplyGammaCorrection },         // bool
	//	//    { "applyExpensiveCorrection", settings.applyExpensiveCorrection },                             // bool
	//	//    { "latencyReducerEnabled", settings.structureCore.latencyReducerEnabled },                     // bool
	//	//    { "sensorInitializationTimeout", settings.structureCore.sensorInitializationTimeout }          // int
	//	//};
	//}

	inline void from_json( const ofJson& json, Settings& settings )
	{
		using namespace serialize;
#define X_ENUM( field, enumValue ) loadEnumFromJSON( json, settings.field, #field, #enumValue, decltype( settings.field )::enumValue );
#define X_BOOL( field ) loadBoolFromJSON( json, settings.field, #field );
#define X_INT( field ) loadIntFromJSON( json, settings.field, #field );
#define X_FLOAT( field ) loadFloatFromJSON( json, settings.field, #field );
		OFXSTRUCTURECORE_SETTINGS_FIELDS
#undef X_ENUM
#undef X_BOOL
#undef X_INT
#undef X_FLOAT
		std::string serial = "";
		loadStringFromJSON( json, serial, "serial" );
		if ( !serial.empty() ) { settings.setSerial( serial ); }
	}

	inline void to_json( ofJson& json, const Settings& settings )
	{
		json = {};
		using namespace serialize;
#define X_ENUM( field, enumValue ) saveEnumToJSON( json, settings.field, #field, #enumValue, decltype( settings.field )::enumValue );
#define X_BOOL( field ) saveBoolToJSON( json, settings.field, #field );
#define X_INT( field ) saveIntToJSON( json, settings.field, #field );
#define X_FLOAT( field ) saveFloatToJSON( json, settings.field, #field );
		OFXSTRUCTURECORE_SETTINGS_FIELDS
#undef X_ENUM
#undef X_BOOL
#undef X_INT
#undef X_FLOAT
		json["serial"] = settings.getSerial();
	}

}  // namespace structure
}  // namespace ofx
