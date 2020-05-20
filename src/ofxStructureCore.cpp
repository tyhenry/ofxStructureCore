#include "ofxStructureCore.h"

ofxStructureCore::ofxStructureCore()
{
	_captureSession.setDelegate( this );
}

bool ofxStructureCore::setup( const Settings& settings )
{
	_settings      = settings;
	_isInit        = false;
	_streamOnReady = false;  // wait until user calls start() to startStreaming()
	if ( _captureSession.startMonitoring( settings ) ) {
		_isInit = true;
		ofLogNotice( ofx_module() ) << "Sensor " << ( serial().empty() ? "" : "[" + serial() + "]" ) << " session initialized.";
		// HACK: if we have requested a serial number, the Structure SDK needs this thread to sleep for a bit before we can call startStreaming()
		// very weird, but seems like the only fix right now
		std::this_thread::sleep_for( std::chrono::milliseconds( 250 ) );
		return true;
	} else {
		ofLogError( ofx_module() ) << "Sensor session failed to initialize!";
		return false;
	}
}

bool ofxStructureCore::start( float timeout )
{
	if ( !_isInit ) {
		ofLogError( ofx_module() ) << "Can't start stream, sensor [" << serial() << "] not initialized!  Did you call setup()?";
		return false;
	}

	if ( _isStreaming ) {
		ofLogWarning( ofx_module() ) << "Can't start(), sensor [" << serial() << "] already streaming.";
		return true;
	}

	if ( !_isReady ) {

		if ( timeout > 0. ) {

			// block until ready signal received or we timeout
			float start = ofGetElapsedTimef();
			while ( !_isReady ) {
				std::this_thread::sleep_for( std::chrono::milliseconds( 5 ) );
				if ( ofGetElapsedTimef() - start >= timeout ) {
					ofLogError( ofx_module() ) << "Sensor [" << serial() << "] didn't start! Timed out after " << timeout << " seconds.";
					return false;
				}
			}
		} else {
			ofLogVerbose( ofx_module() ) << "Sensor [" << serial() << "] will start streaming when Ready signal is received (call stop() to cancel)...";
			_streamOnReady = true;  // start stream when we get the ready signal
			return true;
		}
	}

	if ( _isReady ) {

		if ( _captureSession.startStreaming() ) {
			ofLogVerbose( ofx_module() ) << "Requested sensor [" << serial() << "] start streaming.";
			return true;

			// note: startStreaming() doesn't seem to ever return false, even on failure
			//	_isStreaming managed in the async delegate callback
		} else {
			ofLogError( ofx_module() ) << "Error while trying to start stream for sensor [" << serial() << "]!  Did you call setup()?";
			return false;
		}
	}
	return false;
}

void ofxStructureCore::stop()
{
	_captureSession.stopStreaming();
	_isStreaming   = false;
	_streamOnReady = false;
}

void ofxStructureCore::update()
{
	if ( !_isStreaming ) {
		return;
	}

	// wrap in ofEnableArbTex() to enforce rect tex coords internally
	bool wasUsingArbTex = ofGetUsingArbTex();
	ofEnableArbTex();

	_isFrameNew = _depthDirty || _irDirty || _visibleDirty;
	if ( _depthDirty ) {
		{
			float t = ofGetElapsedTimef();
			if ( _lastDepthUpdateT == 0. ) {
				_lastDepthUpdateT = t;
				_depthUpdateFps   = 0.;
			} else {
				float diff        = t - _lastDepthUpdateT;
				_depthUpdateFps   = 1. / diff;
				_lastDepthUpdateT = t;
				//ofLogNotice( ofx_module() ) << __FUNCTION__ << ": fps = " << ofToString( _depthUpdateFps, 1 ) << ", diff = " << ofToString( diff * 1000., 2 ) << "ms";
			}

			std::unique_lock<std::mutex> lck( _frameLock );
			depthImg.getPixels().setFromPixels( _depthFrame.depthInMillimeters(), _depthFrame.width(), _depthFrame.height(), 1 );
			_depthIntrinsics = _depthFrame.intrinsics();
		}
		depthImg.update();
		// update point cloud
		updatePointCloud();
		_depthDirty = false;
	}
	if ( _irDirty ) {
		{
			std::unique_lock<std::mutex> lck( _frameLock );
			irImg.getPixels().setFromPixels( _irFrame.data(), _irFrame.width(), _irFrame.height(), 1 );
		}
		irImg.update();
		_irDirty = false;
	}
	if ( _visibleDirty ) {
		{
			std::unique_lock<std::mutex> lck( _frameLock );
			visibleImg.getPixels().setFromPixels( _visibleFrame.rgbData(), _visibleFrame.width(), _visibleFrame.height(), 3 );
		}
		visibleImg.update();
		_visibleDirty = false;
	}

	// restore state
	if ( !wasUsingArbTex ) {
		ofDisableArbTex();
	}
}

inline const glm::vec3 ofxStructureCore::getGyroRotationRate()
{
	std::unique_lock<std::mutex> lck( _frameLock );
	auto r = _gyroscopeEvent.rotationRate();
	return {r.x, r.y, r.z};
}

inline const glm::vec3 ofxStructureCore::getAcceleration()
{
	std::unique_lock<std::mutex> lck( _frameLock );
	auto a = _accelerometerEvent.acceleration();
	return {a.x, a.y, a.z};
}

// static methods

std::vector<std::string> ofxStructureCore::listDevices( bool bLog )
{

	std::vector<std::string> devices;
	const ST::ConnectedSensorInfo* sensors[]{nullptr, nullptr, nullptr};
	int count;
	ST::enumerateConnectedSensors( sensors, &count );
	std::stringstream devices_ss;
	for ( int i = 0; i < count; ++i ) {
		if ( sensors && sensors[i] ) {
			if ( bLog ) {
				devices_ss << "\n\t" << i << ": "
				           << "serial [" << sensors[i]->serial << "], "
				           << "product: " << sensors[i]->product << ", "
				           << std::boolalpha
				           << "available: " << sensors[i]->available << ", "
				           << "booted: " << sensors[i]->booted << std::endl;
			}
			devices.emplace_back( &( sensors[i]->serial[0] ) );
		}
	}
	if ( bLog ) {
		ofLogNotice( ofx_module() ) << "\nFound " << devices.size() << " Structure Core devices: " << devices_ss.str();
	}
	return devices;
}

// protected callback handlers -- not to be called directly:

inline void ofxStructureCore::handleNewFrame( const Frame& frame )
{
	if ( _lastFrameT == 0. ) {
		_lastFrameT = ofGetElapsedTimef();
		_fps        = 0.;
	} else {
		float t     = ofGetElapsedTimef();
		float diff  = t - _lastFrameT;
		_fps        = 1. / diff;
		_lastFrameT = t;
		//ofLogNotice( ofx_module() ) << __FUNCTION__ << ": fps = " << ofToString( _fps, 1 ) << ", diff = " << ofToString( diff * 1000., 2 ) << "ms";
	}

	switch ( frame.type ) {
		case Frame::Type::DepthFrame: {
			std::unique_lock<std::mutex> lck( _frameLock );
			_depthFrame = frame.depthFrame;
			_depthDirty = true;  // update the pix/tex in update() loop
		} break;

		case Frame::Type::VisibleFrame: {
			std::unique_lock<std::mutex> lck( _frameLock );
			_visibleFrame = frame.visibleFrame;
			_visibleDirty = true;
		} break;

		case Frame::Type::InfraredFrame: {
			std::unique_lock<std::mutex> lck( _frameLock );
			_irFrame = frame.infraredFrame;
			_irDirty = true;
		} break;

		case Frame::Type::SynchronizedFrames: {
			if ( frame.depthFrame.isValid() ) {
				std::unique_lock<std::mutex> lck( _frameLock );
				_depthFrame = frame.depthFrame;
				_depthDirty = true;
			}
			if ( frame.visibleFrame.isValid() ) {
				std::unique_lock<std::mutex> lck( _frameLock );
				_visibleFrame = frame.visibleFrame;
				_visibleDirty = true;
			}
			if ( frame.infraredFrame.isValid() ) {
				std::unique_lock<std::mutex> lck( _frameLock );
				_irFrame = frame.infraredFrame;
				_irDirty = true;
			}
		} break;

		case Frame::Type::AccelerometerEvent: {
			std::unique_lock<std::mutex> lck( _frameLock );
			_accelerometerEvent = frame.accelerometerEvent;
		} break;

		case Frame::Type::GyroscopeEvent: {
			std::unique_lock<std::mutex> lck( _frameLock );
			_gyroscopeEvent = frame.gyroscopeEvent;
		} break;

		default: {
			ofLogWarning( ofx_module() ) << "Unhandled frame type: " << Frame::toString( frame.type );
		} break;
	}
}

inline void ofxStructureCore::handleSessionEvent( EventType evt )
{
	const std::string id = serial();
	switch ( evt ) {
		case ST::CaptureSessionEventId::Booting:
			ofLogVerbose( ofx_module() ) << "StructureCore is booting...";
			break;
		case ST::CaptureSessionEventId::Ready:
			ofLogNotice( ofx_module() ) << "Sensor " << id << " is ready.";
			_isReady = true;
			if ( _streamOnReady ) {
				start( 0. );  // start streaming
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

void ofxStructureCore::updatePointCloud()
{

	int cols = depthImg.getWidth();
	int rows = depthImg.getHeight();

	float _fx = _depthIntrinsics.fx;
	float _fy = _depthIntrinsics.fy;
	float _cx = _depthIntrinsics.cx;
	float _cy = _depthIntrinsics.cy;

	size_t nVerts     = rows * cols;
	pointcloud.width  = cols;
	pointcloud.height = rows;

	if ( ofIsGLProgrammableRenderer() ) {
		// use tranfsorm feedback to calc point cloud on gpu

		// load shader
		if ( !_transformFbShader.isLoaded() ) {
			ofShader::TransformFeedbackSettings settings;
			settings.shaderSources[GL_VERTEX_SHADER] = ofx::structure::depth_to_points_vert_shader;
			settings.bindDefaults                    = false;
			settings.varyingsToCapture               = {"vPosition"};
			if ( _transformFbShader.setup( settings ) ) {
				ofLogVerbose( ofx_module() ) << "Loaded transform feedback shader.";
			} else {
				ofLogError( ofx_module() ) << "Error loading transform feedback shader!";
			}
		}

		// allocate transform input vbo (with blank vert data)
		if ( _transformFbVbo.getNumVertices() != nVerts ) {
			std::vector<glm::vec3> tmp( nVerts );
			_transformFbVbo.setVertexData( tmp.data(), nVerts, GL_STATIC_DRAW );

			// set static tex coord data here for point cloud vbo since we are updating the size anyway
			std::vector<glm::vec2> tcs( nVerts );
			for ( int i = 0; i < nVerts; ++i ) {
				tcs[i] = glm::vec2( i % pointcloud.width, i / pointcloud.width );  // todo: normalized tex coords?
			}
			pointcloud.vbo.setTexCoordData( tcs.data(), tcs.size(), GL_STATIC_DRAW );
		}

		// allocate transform output buffer
		size_t bufSz = nVerts * sizeof( glm::vec3 );
		if ( _transformFbBuffer.size() != bufSz ) {
			// todo: profile usage hints? GL_STREAM_READ is a guess, see: https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBufferData.xhtml
			_transformFbBuffer.allocate( bufSz, GL_STREAM_READ );
		}

		// perform transform feedback
		_transformFbShader.beginTransformFeedback( GL_POINTS, _transformFbBuffer );
		{
			_transformFbShader.setUniformTexture( "uDepthTex", depthImg.getTexture(), 1 );
			_transformFbShader.setUniform2i( "uDepthDims", cols, rows );
			_transformFbShader.setUniform2f( "uC", _depthIntrinsics.cx, _depthIntrinsics.cy );
			_transformFbShader.setUniform2f( "uF", _depthIntrinsics.fx, _depthIntrinsics.fy );
			_transformFbVbo.draw( GL_POINTS, 0, _transformFbVbo.getNumVertices() );
		}
		_transformFbShader.endTransformFeedback( _transformFbBuffer );

		// set vbo to use the output buffer
		pointcloud.vbo.setVertexBuffer( _transformFbBuffer, 3, sizeof( glm::vec3 ), 0 );

		// todo: map memory to point cloud vertices on CPU?
		// vbo.getVertexBuffer().map<glm::vec3>(GL_READ_ONLY);

	} else {

		// build point cloud on cpu
		auto& depths = depthImg.getPixels();
		std::vector<glm::vec3> verts( nVerts );
		for ( int r = 0; r < rows; r++ ) {
			for ( int c = 0; c < cols; c++ ) {
				int i       = r * cols + c;
				float depth = depths[i];  // millimeters
				// project depth image into metric space
				// see: http://nicolas.burrus.name/index.php/Research/KinectCalibration
				verts[i].x = depth * ( c - _cx ) / _fx * -1.;  // invert x axis for opengl
				verts[i].y = depth * ( r - _cy ) / _fy * -1.;  // invert y axis for opengl
				verts[i].z = depth;
			}
		}
		pointcloud.vbo.setVertexData( verts.data(), nVerts, GL_STREAM_DRAW );  // upload to GPU
	}
}
