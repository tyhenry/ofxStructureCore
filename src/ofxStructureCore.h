#include "ofMain.h"

#include "ST/CameraFrames.h"
#include "ST/CaptureSession.h"
#include "ST/IMUEvents.h"
#include "ST/OCCFileWriter.h"
#include "ST/Utilities.h"

#include "ofxStructureCoreSettings.h"

class ofxStructureCore : public ST::CaptureSessionDelegate
{
public:
	using Settings = ofx::structure::Settings;

	ofxStructureCore();

	bool setup( const Settings& settings );  // call to init device
	bool start();                            // start streaming (if not already)
	void stop();
	void update();

	const bool isFrameNew() const { return _isFrameNew; }
	const bool isStreaming() const { return _isStreaming; }
	const std::string serial() const { return _captureSession.sensorInfo().serialNumber; }

	const glm::vec3 getGyroRotationRate() const;
	const glm::vec3 getAcceleration() const;

	// static methods
	static std::vector<std::string> listDevices( bool bLog );
	static void setLogLevel( ofLogLevel lvl ) { ofSetLogLevel( ofx_module(), lvl ); }

	ofFloatImage depthImg;  // float data is in mm (0 - 65355)
	ofShortImage irImg;
	ofImage visibleImg;

protected:
	ST::CaptureSession _captureSession;

	// latest frames / events
	ST::DepthFrame _depthFrame;
	ST::InfraredFrame _irFrame;
	ST::ColorFrame _visibleFrame;
	ST::GyroscopeEvent _gyroscopeEvent;
	ST::AccelerometerEvent _accelerometerEvent;

	bool _isStreaming   = false;
	bool _streamOnReady = false;  // start streaming when Ready event received
	bool _isFrameNew    = false;
	bool _depthDirty, _irDirty, _visibleDirty = false;

	using Frame = ST::CaptureSessionSample;
	void handleNewFrame( const Frame& frame );

	using EventType = ST::CaptureSessionEventId;
	void handleSessionEvent( EventType evt );

	static inline const std::string& ofx_module()
	{
		static const std::string name = "ofxStructureCore";
		return name;
	}

public:
	// delegate functions overrides
	void captureSessionEventDidOccur( ST::CaptureSession* session, ST::CaptureSessionEventId evt ) override
	{
		if ( session == &_captureSession )
			handleSessionEvent( evt );
		else
			ofLogError( __FUNCTION__ ) << "Received capture session event for unknown capture session: " << ( int )session;
	}
	void captureSessionDidOutputSample( ST::CaptureSession*, const ST::CaptureSessionSample& sample ) override
	{
		handleNewFrame( sample );
	}
};
