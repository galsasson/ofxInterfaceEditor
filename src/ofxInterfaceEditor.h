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

class ofxInterfaceEditor
{
public:
	~ofxInterfaceEditor();
	ofxInterfaceEditor();
	ofxInterfaceEditor(Json::Value& config);



};

#endif /* ofxInterfaceEditor_h */
