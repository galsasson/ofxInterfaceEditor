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
	// default config
	config = Json::objectValue;
	config["width"] = 500;
	config["height"] = 300;
	config["pad"][0] = 6;
	config["pad"][1] = 0;
	config["background-color"] = "#111213 100%";
	config["border-width"] = 16;
	config["border-color"] = "#ffffff 100%";
	config["border-corner"] = 10;
	config["font"] = "Inconsolata-Regular.ttf";
	config["font-color"] = "#ffffff 100%";
	config["font-size"] = 18;
	config["line-numbers"] = true;
	config["selection-color"] = "#aaaaaa 100%";
	config["initial-text"] = "Write here...";

	// setup nanovg
	ofxNanoVG::one().setup();

	// register touch events
	ofAddListener(eventTouchDown, this, &ofxInterfaceEditor::onTouchDown);
	ofAddListener(eventTouchMove, this, &ofxInterfaceEditor::onTouchMove);
	ofAddListener(eventTouchUp, this, &ofxInterfaceEditor::onTouchUp);

	// init variables
	fboPad = 256;
	bDirty = false;
	font = NULL;
	topY = 0;
	textLines.push_back("");
	caret.line = 0;
	caret.chr = 0;
	caretBlink = 0;

	loadConfig(config);
}

void ofxInterfaceEditor::loadConfig(const Json::Value& conf)
{
	ofxJsonParser::objectMerge(config, conf);
	setSize(config["width"].asFloat(), config["height"].asFloat());

	// setup font
	string fontName = config["font"].asString();
	if ((font = ofxNanoVG::one().getFont(fontName)) == NULL) {
		font = ofxNanoVG::one().addFont(fontName, fontName);
	}

	// cache config value
	cache.borderWidth = ofxJsonParser::parseFloat(config["border-width"]);
	cache.fontSize = ofxJsonParser::parseFloat(config["font-size"]);
	cache.pad = ofxJsonParser::parseVector(config["pad"]);
	cache.bLineNumbers = ofxJsonParser::parseBool(config["line-numbers"]);
	ofxNanoVG::one().resetMatrix();
	cache.lineNumbersWidth = cache.bLineNumbers?ofxNanoVG::one().getTextBounds(font, 0, 0, ofToString(textLines.size()), cache.fontSize).x+20:0;
	cache.fontColor = ofxJsonParser::parseColor(config["font-color"]);
	cache.bgColor = ofxJsonParser::parseColor(config["background-color"]);
	cache.borderColor = ofxJsonParser::parseColor(config["border-color"]);
	cache.selectionColor = ofxJsonParser::parseColor(config["selection-color"]);
	cache.letterSize = ofVec2f(0.5*cache.fontSize, cache.fontSize);

	bDirty = true;
}

void ofxInterfaceEditor::update(float dt)
{
//	if (bDirty) {
		renderToFbo(lastRender);
		bDirty = false;
//	}
	caretBlink += 6*dt;
}

void ofxInterfaceEditor::draw()
{
	ofSetColor(cache.bgColor);
	ofDrawRectangle(0, 0, getWidth(), getHeight());

	if (selection.active) {
		// draw selection
		ofSetColor(cache.selectionColor);
		caret_t sc = selection.begin;
		caret_t	ec = selection.end;
		if (sc.line > ec.line) {
			std::swap(sc, ec);
		}
		ofVec2f sPos = toNode(sc);
		ofVec2f ePos = toNode(ec);
		if (sc.line == ec.line) {
			ofDrawRectangle(sPos.x, sPos.y, ePos.x-sPos.x, cache.fontSize);
		}
		else {
			float rightEdge = getWidth()-0.5*cache.borderWidth;
			float leftEdge = 0.5*cache.borderWidth+cache.lineNumbersWidth+cache.pad.x;
			// first line
			ofDrawRectangle(sPos.x, sPos.y, rightEdge-sPos.x, cache.fontSize);
			for (int l=sc.line+1; l<ec.line; l++) {
				ofVec2f lpos = toNode(l, 0);
				ofDrawRectangle(leftEdge, lpos.y, rightEdge-leftEdge, cache.fontSize);
			}
			// last line
			ofDrawRectangle(leftEdge, ePos.y, ePos.x-leftEdge, cache.fontSize);
		}
	}
	else {
		// draw caret
		ofVec2f cPos = toNode(caret);
		float val = cos(caretBlink);
		if (val>0) {
			ofSetColor(255);
			ofDrawRectangle(cPos.x, cPos.y, 1, cache.fontSize);
		}
	}

	ofSetColor(255);
	lastRender.draw(-fboPad/2, -fboPad/2);

}

void ofxInterfaceEditor::setText(const string &text)
{
	textLines = ofSplitString(text, "\n", false, false);
}

void ofxInterfaceEditor::keyPressed(int key)
{
	switch (key) {
		case OF_KEY_LEFT:
			caret.chr--;
			break;
		case OF_KEY_UP:
			caret.line--;
			break;
		case OF_KEY_RIGHT:
			caret.chr++;
			break;
		case OF_KEY_DOWN:
			caret.line++;
			break;
	}
	// clamp caret
	caret.line = caret.line<0?0:caret.line>textLines.size()-1?textLines.size()-1:caret.line;
	string& line = textLines[caret.line];
	caret.chr = caret.chr<0?0:caret.chr>line.length()?line.length():caret.chr;

	if (key >= 32 && key <= 126) {
		line.insert(caret.chr, ofToString((char)key));
		caret.chr++;
	}
	else if (key == 127) {			// Delete
		if (selection.active) {
			clearSelection();
		}
		else if (caret.chr>0) {
			line.erase(line.begin()+caret.chr-1);
			caret.chr--;
		}
		else if (caret.line > 0) {
			textLines.erase(textLines.begin()+caret.line);
			caret.line--;
			line = textLines[caret.line];
			caret.chr = line.length();
		}
	}
	else if (key == 13) {			// Enter
		textLines.insert(textLines.begin()+caret.line+1, "");
		caret.line++;
		caret.chr=0;
	}

	caretBlink=0;

}

void ofxInterfaceEditor::keyReleased(int key)
{

}

void ofxInterfaceEditor::addChar(char ch)
{

}

void ofxInterfaceEditor::renderToFbo(ofFbo& fbo)
{
	// render
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

	// draw frame
	ofxNanoVG::one().strokeRect(0, 0, getWidth(), getHeight(), cache.borderColor, cache.borderWidth);

	// draw text
	ofxNanoVG::one().setFillColor(cache.fontColor);
	float y=0.5*cache.borderWidth;

	if (cache.bLineNumbers) {
		ofxNanoVG::one().fillRect(0.5*cache.borderWidth, 0.5*cache.borderWidth, cache.lineNumbersWidth, getHeight()-cache.borderWidth, ofColor(55, 56, 57));
	}

	caret_t first = toCaret(ofVec2f(0, 0));
	caret_t last = toCaret(ofVec2f(0, getHeight()));
	ofxNanoVG::one().setFillColor(cache.fontColor);
	for (int i=first.line; i<last.line; i++) {
		string& line = textLines[i];
		if (cache.bLineNumbers) {
			ofxNanoVG::one().setTextAlign(ofxNanoVG::NVG_ALIGN_RIGHT, ofxNanoVG::NVG_ALIGN_TOP);
			ofxNanoVG::one().drawText(font, 0.5*cache.borderWidth+cache.lineNumbersWidth, cache.pad.y+y, ofToString(i+1), cache.fontSize);
		}
		ofxNanoVG::one().setTextAlign(ofxNanoVG::NVG_ALIGN_LEFT, ofxNanoVG::NVG_ALIGN_TOP);
		ofxNanoVG::one().drawText(font, 0.5*cache.borderWidth+cache.lineNumbersWidth+cache.pad.x, cache.pad.y+y, line, cache.fontSize);
		y += cache.fontSize;
	}

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

ofxInterfaceEditor::caret_t ofxInterfaceEditor::toCaret(ofVec2f p)
{
	struct caret_t c{
		0,
		0
	};

	if (cache.bLineNumbers) {
		p.x -= cache.lineNumbersWidth;
	}
	p.x -= cache.pad.x + 0.5*cache.borderWidth;
	p.y -= cache.pad.y + 0.5*cache.borderWidth;
	p.y += topY;

	c.line = p.y/cache.fontSize;
	if (c.line < 0) {
		c.line = 0;
	}
	else if (c.line > textLines.size()) {
		c.line = textLines.size();
	}
	string& line = textLines[c.line];

	c.chr = (p.x+0.5*cache.letterSize.x)/cache.letterSize.x;
	if (c.chr < 0) {
		c.chr = 0;
	}
	else if (c.chr > line.length()) {
		c.chr = line.length();
	}
	return c;
}

ofVec2f ofxInterfaceEditor::toNode(const ofxInterfaceEditor::caret_t& _caret)
{
	return toNode(_caret.line, _caret.chr);
}

ofVec2f ofxInterfaceEditor::toNode(int line, int chr)
{
	ofVec2f p(chr*cache.letterSize.x, line*cache.fontSize);
	p.x += 0.5*cache.borderWidth+cache.pad.x;
	p.y += 0.5*cache.borderWidth+cache.pad.y;

	if (cache.bLineNumbers) {
		p.x += cache.lineNumbersWidth;
	}

	return p;
}

void ofxInterfaceEditor::onTouchDown(TouchEvent& event)
{
	ofVec2f local = toLocal(event.position);
	selection.begin = selection.end = caret = toCaret(local);
	selection.active = false;
	caretBlink = 0;
	ofLogNotice("Caret") << caret.line << "x" << caret.chr;
}

void ofxInterfaceEditor::onTouchMove(TouchEvent& event)
{
	selection.end = toCaret(toLocal(event.position));
	selection.active = true;
}

void ofxInterfaceEditor::onTouchUp(TouchEvent& event)
{
	if (selection.begin.chr == selection.end.chr &&
		selection.begin.line == selection.end.line) {
		selection.active = false;
	}
}

void ofxInterfaceEditor::clearSelection()
{
	caret_t sc = selection.begin;
	caret_t	ec = selection.end;
	if (sc.line == ec.line) {
		if (sc.chr > ec.chr) {
			std::swap(sc, ec);
		}
		string& line = textLines[sc.line];
		line.erase(line.begin()+sc.chr, line.begin()+(ec.chr));
	}
	else {
		if (sc.line > ec.line) {
			std::swap(sc, ec);
		}

		// first line
		string& line = textLines[sc.line];
		line.erase(sc.chr, line.size());
		// last line
		line = textLines[ec.line];
		line.erase(0, ec.chr);
		// in between
		for (int l=ec.line-1; l>sc.line; l--) {
			textLines.erase(textLines.begin()+l);
		}
	}
	caret = sc;
	selection.active = false;
}
