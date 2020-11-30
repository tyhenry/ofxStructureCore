#include "ofxStructureCoreDelegate.h"

#include "ofxStructureCore.h"

namespace ofx {
namespace structure {

	ST::CaptureSessionSettings& defaultSettings()
	{
		static ST::CaptureSessionSettings settings;
		//static std::string dummySerial = "";
		static bool isInit             = false;
		if ( !isInit ) {
			isInit                            = true;
			settings.source                   = ST::CaptureSessionSourceId::StructureCore;
			settings.dispatcher               = ST::CaptureSessionDispatcherId::BackgroundThread;
			settings.applyExpensiveCorrection = true;

			settings.structureCore.depthEnabled    = true;
			settings.structureCore.depthResolution = ST::StructureCoreDepthResolution::VGA;
			settings.structureCore.depthFramerate  = 30.f;

			settings.structureCore.infraredEnabled   = true;
			settings.structureCore.infraredFramerate = 30.f;

			settings.structureCore.visibleEnabled   = false;
			settings.structureCore.visibleFramerate = 30.f;
			settings.structureCore.demosaicMethod   = ST::StructureCoreDemosaicMethod::Bilinear;

			settings.structureCore.accelerometerEnabled = false;
			settings.structureCore.gyroscopeEnabled     = false;

			settings.structureCore.depthRangeMode          = ST::StructureCoreDepthRangeMode::Medium;
			//settings.structureCore.initialInfraredExposure = 0.020f;
			//settings.structureCore.initialInfraredGain     = 1;

			//settings.structureCore.sensorSerial = dummySerial.c_str();

			settings.frameSyncEnabled = true;
		}

		return settings;
	}

	DeviceManager::DeviceManager()
	{
		_deviceManager.initialize( this );
		std::this_thread::sleep_for( std::chrono::milliseconds( 5000 ) );
	}

	DeviceManager::~DeviceManager()
	{
		_deviceManager.release();
	}

	void DeviceManager::captureSessionEventDidOccur( ST::CaptureSession* session, ST::CaptureSessionEventId evt )
	{
		//const std::string id  = serial();
		const auto& logModule = ofxStructureCore::ofx_module();
		auto& serial = _sessionSerials[session] = session->sensorInfo().serialNumber;

		switch ( evt ) {
			case ST::CaptureSessionEventId::Booting:
				ofLogVerbose( logModule ) << "Sensor [" << serial << "] is booting.";
				break;
			case ST::CaptureSessionEventId::Detected: {
				ofLogNotice( logModule ) << "Sensor [" << serial << "] detected.";
				break;
			}
			case ST::CaptureSessionEventId::Connected: {
				ofLogNotice( logModule ) << "Sensor [" << serial << "] is connected.";
				break;
			}
			case ST::CaptureSessionEventId::Streaming:
				ofLogNotice( logModule ) << "Sensor [" << serial << "] is streaming.";
				break;
			case ST::CaptureSessionEventId::Disconnected:
				ofLogError( logModule ) << "Sensor [" << serial << "] - Disconnected!";
				break;
			case ST::CaptureSessionEventId::Ready:
				ofLogNotice( logModule ) << "Sensor [" << serial << "] is ready.";
				break;
			case ST::CaptureSessionEventId::Error:
				ofLogError( logModule ) << "Sensor [" << serial << "] - Capture error!";
				break;
			default:
				ofLogWarning( logModule ) << "Sensor [" << serial << "] - Unhandled capture session event type: " << ofxStructureCore::Frame::toString( evt );
		}

		if ( !serial.empty() && serial != "unknown" ) {
			auto* device = _devices[serial];
			if ( device ) {
				device->handleSessionEvent( evt );
			}
		}
	}

	void DeviceManager::captureSessionDidOutputSample( ST::CaptureSession* session, const ST::CaptureSessionSample& sample )
	{
		const auto& logModule = ofxStructureCore::ofx_module();
		auto& serial          = _sessionSerials[session];
		if ( serial.empty() ) {
			serial = session->sensorInfo().serialNumber;
		}
		auto& device = _devices[serial];
		if ( device ) {
			device->handleNewFrame( sample );
		} else {
			ofLogWarning( logModule ) << __FUNCTION__ << ": Capture session is streaming, but no ofxStructureCore device is attached at serial number " << serial << "!";
		}
	}

	std::vector<std::string> DeviceManager::listDetectedSerials()
	{
		std::vector<std::string> serials;
		bool needsWait = false;
		for ( auto& ss : _sessionSerials ) {
			if ( ss.second.empty() || ss.second == "unknown" ) {
				ss.first->startMonitoring( defaultSettings() );
				needsWait = true;
			}
		}
		if ( needsWait ) std::this_thread::sleep_for( std::chrono::milliseconds( 5000 ) );
		for ( auto& ss : _sessionSerials ) {
			if ( !ss.second.empty() && ss.second != "unknown" ) {
				serials.push_back( ss.second );
			}
		}
		return serials;
	}

	void DeviceManager::attach( const std::string& serial, ofxStructureCore* device )
	{
		release( device );
		_devices[serial] = device;
		if ( device ) {
			// find an existing capture session with this serial
			for ( auto& kv : _sessionSerials ) {
				if ( kv.second == serial && kv.first ) {
					kv.first->stopStreaming();
					device->_captureSession = kv.first;  // assign capture session ptr
					break;
				}
			}
		}
	}

	void DeviceManager::release( ofxStructureCore* device )
	{
		if ( !device ) return;
		// lose any ref to ofxStructureCore device
		for ( auto& kv : _devices ) {
			if ( kv.second == device ) {
				kv.second = nullptr;
			}
		}
		auto serial = device->serial();
		if ( !serial.empty() ) {
			// find and close any capture session
			for ( auto& sessionSerial : _sessionSerials ) {
				if ( sessionSerial.second == serial ) {
					if ( sessionSerial.first ) sessionSerial.first->stopStreaming();
				}
			}
		}
	}

	DeviceManager& deviceManager()
	{
		static DeviceManager self;
		return self;
	}

}  // namespace structure
}  // namespace ofx
