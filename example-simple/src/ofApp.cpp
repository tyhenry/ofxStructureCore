#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup()
{
	ofxStructureCore::setLogLevel( OF_LOG_VERBOSE );
	ofxStructureCore::listDevices(true);
	structure.setup( ofxStructureCore::Settings() );
	structure.start();
}

//--------------------------------------------------------------
void ofApp::update()
{
	structure.update();
	ofSetWindowTitle( ofToString( ofGetFrameRate(), 2) + " FPS");
}

//--------------------------------------------------------------
void ofApp::draw()
{
	ofScale(.25);
	structure.depthImg.draw(0,0);
	structure.irImg.draw(0, structure.depthImg.getHeight());
	structure.visibleImg.draw(0, structure.depthImg.getHeight() + structure.irImg.getHeight());
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
