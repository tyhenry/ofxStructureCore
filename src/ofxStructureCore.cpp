#include "ofxStructureCore.h"

ofxStructureCore::ofxStructureCore()
{
	_captureSession.setDelegate( this );
}

bool ofxStructureCore::setup( const Settings& settings )
{
	if ( _captureSession.startMonitoring( settings ) ) {
		ofLogNotice( ofx_module() ) << "Sensor " << serial() << " initialized.";
	} else {
		ofLogError( ofx_module() ) << "Sensor " << serial() << " failed to initialize.";
	}
}

bool ofxStructureCore::start()
{
	_isStreaming = _captureSession.startStreaming();

	if ( !_isStreaming && !_streamOnReady ) {
		ofLogWarning( ofx_module() ) << "Sensor " << serial() << " didn't start, will retry on Ready signal (call stop() to cancel)...";
		_streamOnReady = true;  // stream when we get the ready signal
	}
	return _isStreaming;
}

void ofxStructureCore::stop()
{
	_captureSession.stopStreaming();
	_isStreaming   = false;
	_streamOnReady = false;
}

void ofxStructureCore::update()
{
	_isFrameNew = _depthDirty || _irDirty || _visibleDirty;
	// todo: threadsafe access to internal frame data
	if ( _depthDirty ) {
		depthImg.getPixels().setFromPixels( _depthFrame.depthInMillimeters(), _depthFrame.width, _depthFrame.height, 1 );
		depthImg.update();
		_depthDirty = false;
	}
	if ( _irDirty ) {
		irImg.getPixels().setFromPixels( _irFrame.data(), _irFrame.width, _irFrame.height, 1);
		irImg.update();
		_irDirty = false;
	}
	if ( _visibleDirty ) {
		visibleImg.getPixels().setFromPixels(_visibleFrame.rgbData(), _irFrame.width, _irFrame.height, 3);
		visibleImg.update();
		_visibleDirty = false;
	}
}

inline const glm::vec3 ofxStructureCore::getGyroRotationRate() const
{
	auto r = _gyroscopeEvent.rotationRate();
	return {r.x, r.y, r.z};
}

inline const glm::vec3 ofxStructureCore::getAcceleration() const
{
	auto a = _accelerometerEvent.acceleration();
	return {a.x, a.y, a.z};
}

// static methods

std::vector<std::string> ofxStructureCore::listDevices( bool bLog )
{

	std::vector<std::string> devices;
	const ST::ConnectedSensorInfo* sensors[10];
	int count;
	ST::enumerateConnectedSensors( sensors, &count );
	std::stringstream devices_ss;
	for ( int i = 0; i < count; ++i ) {
		if ( sensors && sensors[i] ) {
			if ( bLog ) {
				devices_ss << "\n\t"
				           << "serial [" << sensors[i]->serial << "], "
				           << "product: " << sensors[i]->product << ", "
				           << std::boolalpha
				           << "available: " << sensors[i]->available << ", "
				           << "booted: " << sensors[i]->booted << std::endl;
			}
			devices.push_back( sensors[i]->serial );
		}
	}
	if ( bLog ) {
		ofLogNotice( ofx_module() ) << "Found " << devices.size() << " devices: " << devices_ss.str();
	}
	return devices;
}

// protected callback handlers -- not to be called directly:

inline void ofxStructureCore::handleNewFrame( const Frame& frame )
{
	std::stringstream data_ss;

	switch ( frame.type ) {
		case Frame::Type::DepthFrame: {
			_depthFrame = frame.depthFrame;
			_depthDirty = true;  // update the pix/tex in update() loop
		} break;

		case Frame::Type::VisibleFrame: {
			_visibleFrame = frame.visibleFrame;
			_visibleDirty = true;
		} break;

		case Frame::Type::InfraredFrame: {
			_irFrame = frame.infraredFrame;
			_irDirty = true;
		} break;

		case Frame::Type::SynchronizedFrames: {
			if ( frame.depthFrame.isValid() ) {
				_depthFrame = frame.depthFrame;
				_depthDirty = true;
			}
			if ( frame.visibleFrame.isValid() ) {
				_visibleFrame = frame.visibleFrame;
				_visibleDirty = true;
			}
			if ( frame.infraredFrame.isValid() ) {
				_irFrame = frame.infraredFrame;
				_irDirty = true;
			}
		} break;

		case Frame::Type::AccelerometerEvent:
			_accelerometerEvent = frame.accelerometerEvent;
			break;

		case Frame::Type::GyroscopeEvent:
			_gyroscopeEvent = frame.gyroscopeEvent;
			break;

		default:
			ofLogWarning( ofx_module() ) << "Unhandled frame type: " << Frame::toString( frame.type );
			return;
	}
}

inline void ofxStructureCore::handleSessionEvent( EventType evt )
{
	const std::string id = _captureSession.sensorInfo().serialNumber;
	switch ( evt ) {
		case ST::CaptureSessionEventId::Booting:
			ofLogVerbose( ofx_module() ) << "StructureCore is booting...";
			break;
		case ST::CaptureSessionEventId::Ready:
			ofLogNotice( ofx_module() ) << "Sensor " << id << " is ready.";
			if ( _streamOnReady ) {
				ofLogNotice( ofx_module() ) << "Sensor " << id << " is starting...";
				start();
			}
			break;
		case ST::CaptureSessionEventId::Connected:
			ofLogVerbose( ofx_module() ) << "Sensor " << id << " is connected.";
			break;
		case ST::CaptureSessionEventId::Streaming:
			ofLogVerbose( ofx_module() ) << "Sensor " << id << " is streaming.";
			_isStreaming = true;
			break;
		case ST::CaptureSessionEventId::Disconnected:
			ofLogError( ofx_module() ) << "Sensor " << id << " - Disconnected!";
			_isStreaming = false;
			break;
		case ST::CaptureSessionEventId::Error:
			ofLogError( ofx_module() ) << "Sensor " << id << " - Capture error!";
			break;
		default:
			ofLogWarning( ofx_module() ) << "Sensor " << id << " - Unhandled capture session event type: " << Frame::toString( evt );
	}
}
