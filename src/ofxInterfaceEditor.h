//
//  ofxInterfaceEditor.h
//  example_single
//
//  Created by Gal Sasson on 10/9/16.
//
//

#ifndef ofxInterfaceEditor_h
#define ofxInterfaceEditor_h

#include "ofxInterface.h"
#include "ofxJSON.h"
#include "ofxNanoVG.h"

using namespace ofxInterface;

class ofxInterfaceEditor : public Node
{
public:
	Json::Value config;
	ofFbo lastRender;
	string text;

	~ofxInterfaceEditor();
	ofxInterfaceEditor();

	void loadConfig(const Json::Value& config);
	void update(float dt);
	void draw();


private:
	bool bDirty;
	int fboPad;
	void renderToFbo(ofFbo& fbo);
	void allocateFbo(ofFbo& fbo);
};

#endif /* ofxInterfaceEditor_h */
