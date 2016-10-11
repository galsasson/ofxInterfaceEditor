//
//  ofxInterfaceEditor.cpp
//  example_single
//
//  Created by Gal Sasson on 10/9/16.
//
//

#include "ofxInterfaceEditor.h"
#include "ofxJsonParser.h"

ofxInterfaceEditor::~ofxInterfaceEditor()
{

}

ofxInterfaceEditor::ofxInterfaceEditor()
{
	ofxNanoVG::one().setup();
	
	Json::Value conf = Json::objectValue;
	conf["width"] = 500;
	conf["height"] = 300;
	conf["background-color"] = "#000000 100%";
	conf["border-width"] = 2;
	conf["border-color"] = "#ffffff 100%";
	conf["border-corner"] = 10;
	conf["font-color"] = "#ffffff 100%";
	conf["font-size"] = 24;
	conf["initial-text"] = "Write here...";
	fboPad = 256;
	loadConfig(conf);
}

void ofxInterfaceEditor::loadConfig(const Json::Value& conf)
{
	config = conf;
	setSize(config["width"].asFloat(), config["height"].asFloat());
	bDirty = true;
}

void ofxInterfaceEditor::update(float dt)
{
//	if (bDirty) {
		renderToFbo(lastRender);
		bDirty = false;
//	}
}

void ofxInterfaceEditor::draw()
{
	ofSetColor(255);
	lastRender.draw(-fboPad/2, -fboPad/2);
}

void ofxInterfaceEditor::renderToFbo(ofFbo& fbo)
{
	allocateFbo(fbo);
	fbo.begin();
	ofDisableDepthTest();
	ofClear(255, 255, 255, 0);
	glClear(GL_STENCIL_BUFFER_BIT);

	// render NanoVG elements
	ofxNanoVG::one().beginFrame(fbo.getWidth(), fbo.getHeight(), 1);
	ofPushMatrix();
	ofTranslate(fboPad/2, fboPad/2);
	ofxNanoVG::one().applyOFMatrix();

	float w = ofxJsonParser::parseFloat(config["width"]);
	float h = ofxJsonParser::parseFloat(config["height"]);

	// draw frame
	ofxNanoVG::one().fillRect(0, 0, w, h, ofxJsonParser::parseColor(config["background-color"]));
	ofxNanoVG::one().strokeRect(0, 0, w, h, ofxJsonParser::parseColor(config["border-color"]), ofxJsonParser::parseFloat(config["border-width"]));

	// draw text


	ofxNanoVG::one().endFrame();


	ofPopMatrix();
	fbo.end();
	fbo.getTexture().generateMipmap();
}

void ofxInterfaceEditor::allocateFbo(ofFbo& fbo)
{
	int reqW = int(config["width"].asFloat()) + fboPad;
	int reqH = int(config["height"].asFloat()) + fboPad;
	if (fbo.getWidth() < reqW ||
		fbo.getHeight() < reqH ) {
		ofFbo::Settings s;
		s.width = reqW;
		s.height = reqH;
		s.internalformat = GL_RGBA;
		s.useStencil = true;
		s.useDepth = true;
		s.minFilter = GL_LINEAR_MIPMAP_LINEAR;
		s.maxFilter = GL_LINEAR;
		s.textureTarget = GL_TEXTURE_2D;
		fbo.allocate(s);
	}
}

