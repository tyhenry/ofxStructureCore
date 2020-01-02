#include "ofMain.h"
#include "ofApp.h"

//========================================================================
int main( ){
	ofSetupOpenGL(1920,1080,OF_WINDOW);			// <-------- setup the GL context

	ofGLFWWindowSettings settings;
	settings.setGLVersion( 3, 3 );
	settings.setSize( 1280, 1024 );
	settings.windowMode = OF_WINDOW;
	//ofCreateWindow( settings );

	// this kicks off the running of my app
	// can be OF_WINDOW or OF_FULLSCREEN
	// pass in width and height too:
	ofRunApp(new ofApp());

}
