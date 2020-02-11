#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup()
{
	ofBackground( 0 );
	//ofDisableArbTex();

	ofSetVerticalSync( true );
	ofxStructureCore::setLogLevel( OF_LOG_VERBOSE );
	auto devices = ofxStructureCore::listDevices( true );

	ofxStructureCore::Settings settings;
	settings.structureCore.depthResolution = ofxStructureCore::Settings::DepthResolution::_1280x960;
	settings.applyExpensiveCorrection      = true;
	settings.structureCore.depthRangeMode  = ofxStructureCore::Settings::DepthRangeMode::Hybrid;
	settings.setSerial( devices[0] );	// one option for specifying a specific camera by serial number

	if ( structure.setup( settings ) ) {
		structure.start();
	}
}

//--------------------------------------------------------------
void ofApp::update()
{
	structure.update();
	ofSetWindowTitle( ofToString( ofGetFrameRate(), 2 ) + " FPS" );
}

//--------------------------------------------------------------
void ofApp::draw()
{
	if ( !structure.depthImg.isAllocated() ) {
		ofDrawBitmapStringHighlight( "waiting for structure sensor...", 20, 20 );
		return;
	}

	// scale to fit images vertically:
	float scale = ofGetHeight() /
	              ( structure.depthImg.getHeight() + structure.irImg.getHeight() + structure.visibleImg.getHeight() );
	ofScale( scale );
	structure.depthImg.draw( 0, 0 );
	structure.irImg.draw( 0, structure.depthImg.getHeight() );
	structure.visibleImg.draw( 0, structure.depthImg.getHeight() + structure.irImg.getHeight() );

	cam.begin();
	ofEnableDepthTest();
	ofDrawAxis( 100 );
	structure.pointcloud.draw();
	ofDisableDepthTest();
	cam.end();
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
}

//--------------------------------------------------------------
void ofApp::mousePressed( int x, int y, int button )
{
}

//--------------------------------------------------------------
void ofApp::mouseReleased( int x, int y, int button )
{
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
