#include "ST/CameraFrames.h"
#include "ST/CaptureSession.h"
#include "ST/DeviceManager.h"
#include "ST/IMUEvents.h"
#include "ST/OCCFileWriter.h"
#include "ST/Utilities.h"
#include "ofMain.h"
#include "ofxStructureCoreDelegate.h"
#include "ofxStructureCoreSettings.h"
#include "ofxStructureCoreUtils.h"

class ofxStructureCore
{
public:
	friend class ofx::structure::DeviceManager;
	using Settings = ofx::structure::Settings;

	ofxStructureCore();
	~ofxStructureCore();

	bool setup( const Settings& settings );  // call to init device
	bool start( float timeout = 0.f );       // start streaming (if not already), wait timeout sec for response or if timeout == 0, start async
	void stop();
	void update();

	void drawDepthRange( float mmMin, float mmMax, float x = 0.f, float y = 0.f, float w = 0.f, float h = 0.f );
	void drawDepth( float x = 0, float y = 0 ) { drawDepth( x, y, depthImg.getWidth(), depthImg.getHeight() ); }
	void drawDepth( float x, float y, float w, float h );

	void drawInfrared( float x = 0, float y = 0 ) { drawInfrared( x, y, irImg.getWidth(), irImg.getHeight() ); }
	void drawInfrared( float x, float y, float w, float h );

	const bool isFrameNew() const { return _isFrameNew; }
	const bool isInit() const { return _isInit; }            // setup() was called
	const bool isReady() const { return _isReady; }          // sensor is ready to start()
	const bool isStreaming() const { return _isStreaming; }  // sensor has started
	const std::string serial() const
	{
		auto serial = _captureSession ? std::string( &_captureSession->sensorInfo().serialNumber[0] ) : "";
		if ( serial.empty() ) {
			serial = _settings.getSerial();  // no sensor initialzed, fallback to settings
		}
		return serial;
	}

	const glm::vec3 getGyroRotationRate();
	const glm::vec3 getAcceleration();
	ST::Intrinsics getIntrinsics();
	const Settings& getSettings() { return _settings; }

	bool setIrExposureAndGain( float exposure, float gain );
	glm::vec2 getIrExposureAndGain();

	// static methods
	static std::vector<std::string> listDevices( bool bLog );
	static void setLogLevel( ofLogLevel lvl ) { ofSetLogLevel( ofx_module(), lvl ); }

	ofFloatImage depthImg;  // float data is in mm
	ofShortImage irImg;
	ofImage visibleImg;

	struct PointCloud
	{
		ofVbo vbo;
		int width, height;
		void draw()
		{
			vbo.draw( GL_POINTS, 0, vbo.getNumVertices() );
		}
	} pointcloud;

protected:
	ST::CaptureSession* _captureSession;
	Settings _settings;

	std::mutex _frameLock;  // delegate receives frames on background thread

	float _lastFrameT, _fps, _lastDepthUpdateT, _depthUpdateFps;

	// latest frames / events
	ST::DepthFrame _depthFrame;
	ST::InfraredFrame _irFrame;
	ST::ColorFrame _visibleFrame;
	ST::GyroscopeEvent _gyroscopeEvent;
	ST::AccelerometerEvent _accelerometerEvent;

	std::atomic<bool>
	    _isInit,               // called setup()
	    _isReady,              // got ready signal from SDK
	    _isStreaming = false;  // got streaming signal from SDK

	bool _streamOnReady,  // should call start() on ready signal from SDK
	    _isFrameNew,
	    _depthDirty,
	    _irDirty,
	    _visibleDirty = false;
	ST::Intrinsics _depthIntrinsics;
	ofShader _rangeMapShader;           // maps mm depth range to visible range 0-1
	ofShader _transformFbShader;        // converts depth image to point cloud
	ofBufferObject _transformFbBuffer;  // gpu buffer for point cloud
	ofVbo _transformFbVbo;              // static vbo for transform fb

	using Frame = ST::CaptureSessionSample;
	void handleNewFrame( const Frame& frame );

	using EventType = ST::CaptureSessionEventId;
	void handleSessionEvent( EventType evt );

	void updatePointCloud();

	static inline const std::string& ofx_module()
	{
		static const std::string name = "ofxStructureCore";
		return name;
	}
};
