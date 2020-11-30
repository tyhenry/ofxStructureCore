#pragma once

#include "ofMain.h"
#include "ofxStructureCore.h"

class ofApp : public ofBaseApp
{

public:
	void setup();
	void update();
	void draw();
	void exit();

	void keyPressed( int key );
	void keyReleased( int key );
	void mouseMoved( int x, int y );
	void mouseDragged( int x, int y, int button );
	void mousePressed( int x, int y, int button );
	void mouseReleased( int x, int y, int button );
	void mouseEntered( int x, int y );
	void mouseExited( int x, int y );
	void windowResized( int w, int h );
	void dragEvent( ofDragInfo dragInfo );
	void gotMessage( ofMessage msg );

	ofxStructureCore structure;
	ofEasyCam cam;

	ofRectangle irGainSlider, irExposureSlider;
	ofParameter<float> irGain{ "ir gain", 1., 0., 3. };
	ofParameter<float> irExposure{ "ir exposure", 0.016f, 0., 0.03 };
	bool irSlidersActive = false;
	bool startedStream = false;
};
