#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup()
{
	ofBackground( 0 );
	//ofDisableArbTex();

	ofSetVerticalSync( true );
	ofxStructureCore::setLogLevel( OF_LOG_VERBOSE );
	auto devices = ofxStructureCore::listDevices( true );

	auto json = ofLoadJson( "settings.json" );

	if ( devices.empty() ) {
		ofLogError() << "No Structure Core devices found!";

	} else {

		ofxStructureCore::Settings settings;
		settings.structureCore.depthResolution        = ofxStructureCore::Settings::DepthResolution::_1280x960;
		settings.structureCore.depthRangeMode         = ofxStructureCore::Settings::DepthRangeMode::Long;
		settings.structureCore.dynamicCalibrationMode = ofxStructureCore::Settings::CalibrationMode::OneShotPersistent;
		settings.applyExpensiveCorrection             = true;
		settings.generatePointCloud                   = true;  // don't auto-generate point cloud VBO
		settings.structureCore.visibleEnabled         = false;
		settings.structureCore.infraredEnabled        = true;

		try {
			settings = json;
		} catch ( const std::exception& e ) {
			ofLogError( __FUNCTION__ ) << "Error loading ofxStructureCore::Settings from JSON: " << e.what();
		}

		if ( structure.setup( settings ) ) {
			structure.start( 10 );  // wait 10 seconds for ready signal
		}

		irExposure.set( settings.structureCore.initialInfraredExposure );
		irGain.set( settings.structureCore.initialInfraredGain );
	}

	auto winDims     = ofGetWindowSize();
	irExposureSlider = ofRectangle( winDims.x - 250, 50, 200, 50 );
	irGainSlider     = ofRectangle( winDims.x - 250, 150, 200, 50 );
}

//--------------------------------------------------------------
void ofApp::update()
{
	structure.update();
	ofSetWindowTitle( ofToString( ofGetFrameRate(), 2 ) + " FPS" );

	if ( !startedStream && structure.isStreaming() ) {
		startedStream = true;
		auto exp_gain = structure.getIrExposureAndGain();
		irExposure.set( exp_gain.x );
		irGain.set( exp_gain.y );
	}
}

//--------------------------------------------------------------
void ofApp::draw()
{
	//if ( !structure.depthImg.isAllocated() ) {
	if ( !structure.isStreaming() ) {
		ofDrawBitmapStringHighlight( "waiting for structure sensor...", 20, 20 );
		return;
	}

	// scale to fit images vertically:
	float scale = ofGetHeight() /
	              ( structure.depthImg.getHeight() + structure.irImg.getHeight() + structure.visibleImg.getHeight() );
	ofPushMatrix();
	ofScale( scale );
	//structure.depthImg.draw( 0, 0 );
	//structure.irImg.draw( 0, structure.depthImg.getHeight() );
	structure.drawDepth( 0, 0 );
	structure.drawInfrared( 0, structure.depthImg.getHeight() );
	structure.visibleImg.draw( 0, structure.depthImg.getHeight() + structure.irImg.getHeight() );
	ofPopMatrix();

	// testing: if mouse over infrared image, show pixel value
	glm::vec2 mouse{ ofGetMouseX(), ofGetMouseY() };
	ofRectangle irBounds{ 0, structure.depthImg.getHeight() * scale, structure.irImg.getWidth() * scale, structure.irImg.getHeight() * scale };
	if ( irBounds.inside( mouse ) ) {
		std::stringstream ss;
		int x      = ofMap( mouse.x, irBounds.getLeft(), irBounds.getRight(), 0, structure.irImg.getWidth() - 1 );
		int y      = ofMap( mouse.y, irBounds.getTop(), irBounds.getBottom(), 0, structure.irImg.getHeight() - 1 );
		auto color = structure.irImg.getColor( x, y );
		ss << color;
		ofDrawBitmapStringHighlight( ss.str(), mouse );
	}

	ofSetColor( 255 );
	cam.begin();
	ofEnableDepthTest();
	ofDrawAxis( 100 );
	structure.pointcloud.draw();
	ofDisableDepthTest();
	cam.end();

	// exposure and gain sliders
	ofPushStyle();
	ofNoFill();
	ofDrawRectangle( irExposureSlider );
	ofDrawRectangle( irGainSlider );
	ofFill();
	ofSetColor( 100 );
	ofDrawRectangle( irExposureSlider.x + 2, irExposureSlider.y + 2, ofMap( irExposure.get(), irExposure.getMin(), irExposure.getMax(), 0, irExposureSlider.width - 4 ), irExposureSlider.height - 4 );
	ofDrawRectangle( irGainSlider.x + 2, irGainSlider.y + 2, ofMap( irGain.get(), irGain.getMin(), irGain.getMax(), 0, irGainSlider.width - 4 ), irGainSlider.height - 4 );
	ofSetColor( 255 );
	ofDrawBitmapStringHighlight( "IR Exposure: " + ofToString( irExposure.get() ), irExposureSlider.x, irExposureSlider.y );
	ofDrawBitmapStringHighlight( "IR Gain: " + ofToString( irGain.get() ), irGainSlider.x, irGainSlider.y );
	ofPopStyle();
}

//--------------------------------------------------------------
void ofApp::exit()
{
	structure.stop();
}

//--------------------------------------------------------------
void ofApp::keyPressed( int key )
{
}

//--------------------------------------------------------------
void ofApp::keyReleased( int key )
{
}

//--------------------------------------------------------------
void ofApp::mouseMoved( int x, int y )
{
}

//--------------------------------------------------------------
void ofApp::mouseDragged( int x, int y, int button )
{
	glm::vec2 mouse{ x, y };
	if ( irGainSlider.inside( mouse ) ) {
		irGain.set( ofMap( mouse.x, irGainSlider.getLeft(), irGainSlider.getRight(), irGain.getMin(), irGain.getMax() ) );
		irSlidersActive = true;
	} else if ( irExposureSlider.inside( mouse ) ) {
		irExposure.set( ofMap( mouse.x, irExposureSlider.getLeft(), irExposureSlider.getRight(), irExposure.getMin(), irExposure.getMax() ) );
		irSlidersActive = true;
	}
}

//--------------------------------------------------------------
void ofApp::mousePressed( int x, int y, int button )
{
}

//--------------------------------------------------------------
void ofApp::mouseReleased( int x, int y, int button )
{
	if ( irSlidersActive ) {
		ofLogNotice() << "IR exposure and gain currently: " << structure.getIrExposureAndGain() << "\n"
		              << "setting to: " << irExposure.get() << ", " << irGain.get() << "...";
		structure.setIrExposureAndGain( irExposure.get(), irGain.get() );
		std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
		auto eg = structure.getIrExposureAndGain();
		irExposure.set( eg.x );
		irGain.set( eg.y );
		irSlidersActive = false;
	}
}

//--------------------------------------------------------------
void ofApp::mouseEntered( int x, int y )
{
}

//--------------------------------------------------------------
void ofApp::mouseExited( int x, int y )
{
}

//--------------------------------------------------------------
void ofApp::windowResized( int w, int h )
{
}

//--------------------------------------------------------------
void ofApp::gotMessage( ofMessage msg )
{
}

//--------------------------------------------------------------
void ofApp::dragEvent( ofDragInfo dragInfo )
{
}
