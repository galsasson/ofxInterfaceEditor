#include "ofMain.h"
#include "ofApp.h"

//========================================================================
int main( ){
	ofGLFWWindowSettings s;
	s.width = 600;
	s.height = 500;
	s.numSamples = 0;
	s.doubleBuffering = true;
	s.depthBits = 24;
	s.stencilBits = 8;
	s.setGLVersion(3, 2);
	shared_ptr<ofAppBaseWindow> win = ofCreateWindow(s);

	// this kicks off the running of my app
	// can be OF_WINDOW or OF_FULLSCREEN
	// pass in width and height too:
	shared_ptr<ofApp> app(new ofApp());


	ofRunApp(win, app);
	int ret = ofRunMainLoop();
	// this kicks off the running of my app
	// can be OF_WINDOW or OF_FULLSCREEN
	// pass in width and height too:
//	ofRunApp(new ofApp());

}
