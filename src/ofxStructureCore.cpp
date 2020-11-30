#include "ofxStructureCore.h"

ofxStructureCore::ofxStructureCore()
{
	//_captureSession.setDelegate( this );
	auto* dm = &ofx::structure::deviceManager();  // init
}

ofxStructureCore::~ofxStructureCore()
{
}

bool ofxStructureCore::setup( const Settings& settings )
{
	_settings      = settings;
	_isInit        = false;
	_streamOnReady = false;  // wait until user calls start() to startStreaming()
	auto serial    = settings.getSerial();

	if ( !serial.empty() ) ofx::structure::deviceManager().attach( serial, this );

	if ( _captureSession && _captureSession->startMonitoring( settings ) ) {
		_isInit = true;
		ofLogNotice( ofx_module() ) << "Sensor " << ( serial.empty() ? "" : "[" + serial + "] " ) << "session initialized.";
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
		if ( _captureSession ) _isReady = true;

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

		if ( _captureSession && _captureSession->startStreaming() ) {
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
	if ( _captureSession ) _captureSession->stopStreaming();
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

		if ( _settings.generatePointCloud ) {
			updatePointCloud();  // update point cloud vbo
		}
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

void ofxStructureCore::drawDepthRange( float mmMin, float mmMax, float x, float y, float w, float h )
{

	if ( ofIsGLProgrammableRenderer() && depthImg.isAllocated() ) {

		if ( w == 0.f ) w = depthImg.getWidth();
		if ( h == 0.f ) h = depthImg.getHeight();

		// load shader
		if ( !_rangeMapShader.isLoaded() ) {
			_rangeMapShader.setupShaderFromSource( GL_VERTEX_SHADER, ofx::structure::map_range_vert_shader );
			_rangeMapShader.setupShaderFromSource( GL_FRAGMENT_SHADER, ofx::structure::map_range_frag_shader );
			_rangeMapShader.bindDefaults();
			if ( _rangeMapShader.linkProgram() ) {
				ofLogVerbose( ofx_module() ) << "Loaded depth range map shader.";
			} else {
				ofLogError( ofx_module() ) << "Error loading depth range map shader!";
			}
		}

		if ( _rangeMapShader.isLoaded() && depthImg.isAllocated() ) {
			// map depth range
			_rangeMapShader.begin();
			_rangeMapShader.setUniform4f( "inMin", glm::vec4( mmMin, mmMin, mmMin, 0.f ) );
			_rangeMapShader.setUniform4f( "inMax", glm::vec4( mmMax, mmMax, mmMax, 1.f ) );
			_rangeMapShader.setUniform4f( "outMin", glm::vec4( 0.0 ) );
			_rangeMapShader.setUniform4f( "outMax", glm::vec4( 1.0 ) );
			depthImg.draw( x, y, w, h );  // binds to tex0
			_rangeMapShader.end();
		}
	}
}

void ofxStructureCore::drawDepth( float x, float y, float w, float h )
{
	float mmMin = 0.0;
	float mmMax = 1.0;
	_settings.minMaxDepthInMmOfDepthRangeMode( _settings.structureCore.depthRangeMode, mmMin, mmMax );
	drawDepthRange( mmMin, mmMax, x, y, w, h );
}

void ofxStructureCore::drawInfrared( float x, float y, float w, float h )
{
	if ( ofIsGLProgrammableRenderer() && irImg.isAllocated() ) {

		if ( w == 0.f ) w = irImg.getWidth();
		if ( h == 0.f ) h = irImg.getHeight();

		// load shader
		if ( !_rangeMapShader.isLoaded() ) {
			_rangeMapShader.setupShaderFromSource( GL_VERTEX_SHADER, ofx::structure::map_range_vert_shader );
			_rangeMapShader.setupShaderFromSource( GL_FRAGMENT_SHADER, ofx::structure::map_range_frag_shader );
			_rangeMapShader.bindDefaults();
			if ( _rangeMapShader.linkProgram() ) {
				ofLogVerbose( ofx_module() ) << "Loaded range map shader.";
			} else {
				ofLogError( ofx_module() ) << "Error loading range map shader!";
			}
		}

		if ( _rangeMapShader.isLoaded() && irImg.isAllocated() ) {
			// map depth range
			_rangeMapShader.begin();
			_rangeMapShader.setUniform4f( "inMin", glm::vec4( glm::vec3( 0.f ), 0.f ) );
			_rangeMapShader.setUniform4f( "inMax", glm::vec4( glm::vec3( 1023.f / 65535.f ), 1.f ) );
			_rangeMapShader.setUniform4f( "outMin", glm::vec4( glm::vec3( 0.f ), 1.f ) );
			_rangeMapShader.setUniform4f( "outMax", glm::vec4( glm::vec3( 1.f ), 1.f ) );
			irImg.draw( x, y, w, h );  // binds to tex0
			_rangeMapShader.end();
		}
	}
}

inline const glm::vec3 ofxStructureCore::getGyroRotationRate()
{
	std::unique_lock<std::mutex> lck( _frameLock );
	auto r = _gyroscopeEvent.rotationRate();
	return { r.x, r.y, r.z };
}

inline const glm::vec3 ofxStructureCore::getAcceleration()
{
	std::unique_lock<std::mutex> lck( _frameLock );
	auto a = _accelerometerEvent.acceleration();
	return { a.x, a.y, a.z };
}

ST::Intrinsics ofxStructureCore::getIntrinsics()
{
	return _depthIntrinsics;
}

bool ofxStructureCore::setIrExposureAndGain( float exposure, float gain )
{
	if ( _captureSession ) {
		_captureSession->setInfraredCamerasExposureAndGain( exposure, gain );  // no way to check if fail?
		return true;
	}
	return false;
}

glm::vec2 ofxStructureCore::getIrExposureAndGain()
{
	glm::vec2 r{ 0, 0 };
	if ( _captureSession ) {
		_captureSession->getInfraredCamerasExposureAndGain( &r.x, &r.y );
	}
	return r;
}

// static methods

std::vector<std::string> ofxStructureCore::listDevices( bool bLog )
{

	std::vector<std::string> serials = ofx::structure::deviceManager().listDetectedSerials();

	if ( bLog ) {
		std::stringstream ss;
		for ( int i = 0; i < serials.size(); ++i ) {
			ss << "\n\t" << i << ": "
			   << "serial [" << serials[i] << "]\n";
		}
		ofLogNotice( ofx_module() ) << "\nFound " << serials.size() << " Structure Core devices: " << ss.str();
	}
	return serials;
}

// protected callback handlers -- not to be called directly:

void ofxStructureCore::handleNewFrame( const Frame& frame )
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

void ofxStructureCore::handleSessionEvent( EventType evt )
{
	const std::string id = serial();
	switch ( evt ) {
		case ST::CaptureSessionEventId::Connected:
		case ST::CaptureSessionEventId::Ready:
			_isReady = true;
			if ( _streamOnReady ) {
				start( 0. );  // start streaming
			}
			break;
		case ST::CaptureSessionEventId::Streaming:
			_isStreaming = true;
			// enforce ir exposure and gain - seems to be issues with initial settings
			if ( _settings.structureCore.infraredEnabled && !_settings.structureCore.infraredAutoExposureEnabled ) {
				_captureSession->setInfraredCamerasExposureAndGain( _settings.structureCore.initialInfraredExposure, _settings.structureCore.initialInfraredGain );
			}
			break;
		case ST::CaptureSessionEventId::Disconnected:
			_isStreaming = false;
			break;
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
			settings.varyingsToCapture               = { "vPosition" };
			if ( _transformFbShader.setup( settings ) ) {
				ofLogVerbose( ofx_module() ) << "Loaded transform feedback shader.";
			} else {
				ofLogError( ofx_module() ) << "Error loading transform feedback shader!";
			}
		}

		// allocate transform input vbo (with blank vert data)
		if ( _transformFbVbo.getNumVertices() != nVerts ) {
			std::vector<glm::vec3> vts( nVerts );  // verts
			std::vector<glm::vec2> tcs( nVerts );  // tex coords
			for ( int i = 0; i < nVerts; ++i ) {
				tcs[i] = glm::vec2( i % pointcloud.width, i / pointcloud.width );                           // row, col
				vts[i] = glm::vec3( tcs[i] - glm::vec2( pointcloud.width, pointcloud.height ) * .5f, 0. );  //
			}
			_transformFbVbo.setVertexData( vts.data(), nVerts, GL_STATIC_DRAW );
			pointcloud.vbo.setVertexData( vts.data(), nVerts, GL_STATIC_DRAW );
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
