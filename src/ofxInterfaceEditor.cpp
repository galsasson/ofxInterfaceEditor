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
	configJson = Json::objectValue;
	configJson["width"] = 700;
	configJson["lines"] = 20;
	configJson["pad"][0] = 6;
	configJson["pad"][1] = 0;
	configJson["background-color"] = "#222322 100%";
	configJson["border-width"] = 2;
	configJson["border-color"] = "#ffffff 100%";
	configJson["border-corner"] = 10;
	configJson["font"] = "Inconsolata-Regular.ttf";
	configJson["font-color"] = "#ffffff 100%";
	configJson["font-size"] = 22;
	configJson["line-numbers"] = true;
	configJson["line-numbers-color"] = "#ffffff 100%";
	configJson["line-numbers-bg-color"] = "#434445 100%";
	configJson["selection-color"] = "#aaaaaa 50%";
	configJson["special-enter"] = false;
	configJson["initial-text"] = "Write here...";

	// setup nanovg
	ofxNanoVG::one().setup();

	// register touch events
	ofAddListener(eventTouchDown, this, &ofxInterfaceEditor::onTouchDown);
	ofAddListener(eventTouchMove, this, &ofxInterfaceEditor::onTouchMove);
	ofAddListener(eventTouchUp, this, &ofxInterfaceEditor::onTouchUp);

	// init variables
	fboPad =			256;
	bDirty =			false;
	font =				NULL;
	state.caret.line =	0;
	state.caret.chr =	0;
	caretBlink =		0;
	bShiftPressed = false;
	view = state.targetView =	ofRectangle(0, 0, 0, 0);
	textLines.push_back("");

	setConfig(configJson);
}

void ofxInterfaceEditor::setConfig(const Json::Value& conf)
{
	ofxJsonParser::objectMerge(configJson, conf);

	// setup font
	string fontName = configJson["font"].asString();
	if ((font = ofxNanoVG::one().getFont(fontName)) == NULL) {
		font = ofxNanoVG::one().addFont(fontName, fontName);
	}

	// cache config value
	config.width = ofxJsonParser::parseFloat(configJson["width"], 500);
	config.lines = ofxJsonParser::parseInt(configJson["lines"], 1);
	config.borderWidth = ofxJsonParser::parseFloat(configJson["border-width"]);
	config.borderCorner = ofxJsonParser::parseFloat(configJson["border-corner"]);
	config.fontSize = ofxJsonParser::parseFloat(configJson["font-size"]);
	config.pad = ofxJsonParser::parseVector(configJson["pad"]);
	config.bLineNumbers = ofxJsonParser::parseBool(configJson["line-numbers"]);
	config.lineNumbersColor = ofxJsonParser::parseColor(configJson["line-numbers-color"]);
	config.lineNumbersBGColor = ofxJsonParser::parseColor(configJson["line-numbers-bg-color"]);
	config.fontColor = ofxJsonParser::parseColor(configJson["font-color"]);
	config.bgColor = ofxJsonParser::parseColor(configJson["background-color"]);
	config.borderColor = ofxJsonParser::parseColor(configJson["border-color"]);
	config.selectionColor = ofxJsonParser::parseColor(configJson["selection-color"]);
	float lw = ofxNanoVG::one().getTextBounds(font, 0, 0, "ab", config.fontSize).width - ofxNanoVG::one().getTextBounds(font, 0, 0, "b", config.fontSize).width;
	config.letterSize = ofVec2f(lw, config.fontSize);
	config.bSpecialEnter = ofxJsonParser::parseBool(configJson["special-enter"]);


	// set size
	setSize(config.width, config.fontSize*config.lines+config.borderWidth);
	state.targetView = view = ofRectangle(0, 0, getWidth(), getHeight());

	bDirty = true;
}

void ofxInterfaceEditor::update(float dt)
{
	ofVec2f offset = (state.targetView.getPosition()-view.getPosition());
	view.translate(0.5f*offset);

	// can be optimized (calc only on change);
	config.lineNumbersWidth = getLineNumberWidth();

//	if (bDirty) {
		renderToFbo(lastRender);
		bDirty = false;
//	}
	caretBlink += 10*dt;
}

void ofxInterfaceEditor::draw()
{
	ofSetColor(255);
	lastRender.draw(-fboPad/2, -fboPad/2);
}

void ofxInterfaceEditor::setText(const string &text)
{
	textLines = ofSplitString(text, "\n", false, false);
	if (textLines.empty()) {
		textLines.push_back("");
	}

	// clamp caret
	state.caret.line = state.caret.line<0?0:state.caret.line>textLines.size()-1?textLines.size()-1:state.caret.line;
	state.caret.chr = state.caret.chr<0?0:state.caret.chr>textLines[state.caret.line].length()?textLines[state.caret.line].length():state.caret.chr;

	refreshView();
}

string ofxInterfaceEditor::getText()
{
	string str;
	for (int i=0; i<textLines.size(); i++) {
		str += textLines[i];
		if (i<textLines.size()-1) {
			str += "\n";
		}
	}
	return str;
}

void ofxInterfaceEditor::loadTextFile(const string &filename)
{
	ofFile file(filename);
	if (!file.exists()) {
		ofLogError("ofxInterfaceEditor") << "No such file: "<<filename;
		return;
	}

	setText(file.readToBuffer().getText());
}

void ofxInterfaceEditor::keyPressed(int key)
{
	ofLog() << "Key Pressed = "<<key;

	if (key == OF_KEY_SHIFT) {
		bShiftPressed=true;
		if (!state.selection.active) {
			state.selection.begin = state.caret;
		}
	}
	else if (key == OF_KEY_COMMAND) {
		bCommandKeyPressed=true;
	}
	else if (key == OF_KEY_CONTROL) {
		bControlKeyPressed=true;
	}
	else if (key >= OF_KEY_LEFT && key <= OF_KEY_END) {
		switch (key) {
			case OF_KEY_LEFT:
				state.caret.chr--;
				if (state.caret.chr < 0) {
					if (state.caret.line>0) {
						state.caret.line--;
						state.caret.chr = textLines[state.caret.line].size();
					}
				}
				break;
			case OF_KEY_UP:
				state.caret.line--;
				break;
			case OF_KEY_RIGHT:
				state.caret.chr++;
				if (state.caret.chr > textLines[state.caret.line].size()) {
					if (state.caret.line < textLines.size()-1) {
						state.caret.chr=0;
						state.caret.line++;
					}
				}
				break;
			case OF_KEY_DOWN:
				state.caret.line++;
				break;
			case OF_KEY_HOME:
				state.caret.chr = 0;
				break;
			case OF_KEY_END:
				state.caret.chr = textLines[state.caret.line].size();
				break;
		}
		// clamp caret
		state.caret.line = state.caret.line<0?0:state.caret.line>textLines.size()-1?textLines.size()-1:state.caret.line;
		state.caret.chr = state.caret.chr<0?0:state.caret.chr>textLines[state.caret.line].length()?textLines[state.caret.line].length():state.caret.chr;

		if (bShiftPressed) {
			state.selection.end = state.caret;
			state.selection.active = true;
		}
		else {
			state.selection.active = false;
		}
	}
	else if (bControlKeyPressed || bCommandKeyPressed) {
		// Special commands
		key += bControlKeyPressed?32:96;
		// command/control with character
		if (key == 'x') {					// Cut
			if (state.selection.active) {
				pushUndoState();
				string str = getText();
				caret_t sb = state.selection.begin;
				caret_t se = state.selection.end;
				if (((sb.line == se.line) && (sb.chr > se.chr)) ||
					sb.line > se.line) {
					swap(sb, se);
				}
				size_t bpos = toTextPos(sb);
				size_t size = toTextPos(se)-bpos;
				pasteboard = str.substr(bpos, size);
				str.erase(bpos, size);
				setText(str);
				state.selection.active = false;
				state.caret = sb;
			}
		}
		else if (key == 'c') {				// Copy
			if (state.selection.active) {
				pasteboard = getSelectedText();
			}
			else {
				pasteboard = textLines[state.caret.line] + "\n";
			}
		}
		else if (key == 'v') {				// Paste
			if (pasteboard.size()>0) {
				pushUndoState();
				if (state.selection.active) {
					// paste in selection place
					string str = getText();
					caret_t sb = state.selection.begin;
					caret_t se = state.selection.end;
					if (((sb.line == se.line) && (sb.chr > se.chr)) ||
						sb.line > se.line) {
						swap(sb, se);
					}
					size_t bpos = toTextPos(sb);
					size_t epos = toTextPos(se);
					stringstream newstr;
					newstr << str.substr(0, bpos);
					newstr << pasteboard;
					newstr << str.substr(epos, str.size()-epos);
					setText(newstr.str());
					state.caret = toCaret(bpos+pasteboard.size());
					state.selection.begin = toCaret(bpos);
					state.selection.end = toCaret(bpos+pasteboard.size());
				}
				else {
					// paste in caret position
					string str = getText();
					size_t cpos = toTextPos(state.caret);
					stringstream newText;
					newText << str.substr(0, cpos);
					newText << pasteboard;
					newText << str.substr(cpos, str.size()-cpos);
					setText(newText.str());
					state.caret = toCaret(cpos+pasteboard.size());
				}
			}
		}
		else if (key == 'z') {				// Undo/Redo
			if (bShiftPressed) {			// Redo
				popRedoState();
			}
			else {							// Undo
				popUndoState();
			}
		}
	}
	else if (key >= 32 && key <= 126) {
		// This is normal letter typing
		pushUndoState();
		if (state.selection.active) {
			clearSelection();
		}
		textLines[state.caret.line].insert(state.caret.chr, ofToString((char)key));
		state.caret.chr++;
	}
	else if (key == OF_KEY_BACKSPACE) {			// Delete
		pushUndoState();
		if (state.selection.active) {
			clearSelection();
		}
		else if (state.caret.chr>0) {
			textLines[state.caret.line].erase(textLines[state.caret.line].begin()+state.caret.chr-1);
			state.caret.chr--;
		}
		else if (state.caret.line > 0) {
			int chr = textLines[state.caret.line-1].size();
			// append this line to previous
			textLines[state.caret.line-1].append(textLines[state.caret.line]);
			// remove this line
			textLines.erase(textLines.begin()+state.caret.line);
			state.caret.line--;
			state.caret.chr = chr;
		}
	}
	else if (key == OF_KEY_DEL) {
		pushUndoState();
		if (state.selection.active) {
			clearSelection();
		}
		else {
			if (textLines[state.caret.line].size() == state.caret.chr) {
				// delete at end of line
				if (state.caret.line < textLines.size()-1) {
					textLines[state.caret.line].append(textLines[state.caret.line+1]);
					textLines.erase(textLines.begin()+state.caret.line+1);
				}
			}
			else {
				// delete anywhere else
				textLines[state.caret.line].erase(state.caret.chr, 1);
			}
		}
	}
	else if (key == OF_KEY_RETURN) {			// Enter
		pushUndoState();
		if (config.bSpecialEnter) {
			if (state.caret.chr == textLines[state.caret.line].size()) {
				// Enter at end of line
				textLines.insert(textLines.begin()+state.caret.line+1, "");
				state.caret.line++;
				state.caret.chr=0;
			}
			else {
				// Enter in middle of line
			}
		}
		else {
			if (state.selection.active) {
				clearSelection();
			}
			// Normal enter behavior
			string afterCaret = textLines[state.caret.line].substr(state.caret.chr, textLines[state.caret.line].size());
			textLines[state.caret.line].erase(state.caret.chr, textLines[state.caret.line].size()-state.caret.chr);
			textLines.insert(textLines.begin()+state.caret.line+1, afterCaret);
			state.caret.line++;
			state.caret.chr=0;
		}
	}

	refreshView();
	caretBlink=0;
}

void ofxInterfaceEditor::keyReleased(int key)
{
	ofLog() << "Key Released = "<<key;

	switch (key) {
		case OF_KEY_SHIFT:
			bShiftPressed=false;
			ofLogVerbose() << "set bShiftPressed to false";
			break;
		case OF_KEY_COMMAND:
			bCommandKeyPressed = false;
			break;
		case OF_KEY_CONTROL:
			bControlKeyPressed = false;
			break;
	}
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
	ofClear(ofColor(config.bgColor, 0));
	glClear(GL_STENCIL_BUFFER_BIT);

	// render NanoVG elements
	ofxNanoVG::one().beginFrame(fbo.getWidth(), fbo.getHeight(), 1);
	ofPushMatrix();
	ofTranslate(fboPad/2, fboPad/2);
	ofxNanoVG::one().applyOFMatrix();

	//////////////////////////////
	// DRAW FRAME
	//////////////////////////////

	if (config.borderCorner>0.1) {
		// Rounded corners
		// draw background
		ofxNanoVG::one().fillRoundedRect(0, 0, getWidth(), getHeight(), config.borderCorner, config.bgColor);
		// draw frame
		ofxNanoVG::one().strokeRoundedRect(0, 0, getWidth(), getHeight(), config.borderCorner, config.borderColor, config.borderWidth);
	}
	else {
		// Sharp corners
		// draw background
		ofxNanoVG::one().fillRect(0, 0, getWidth(), getHeight(), config.bgColor);
		// draw frame
		ofxNanoVG::one().strokeRect(0, 0, getWidth(), getHeight(), config.borderColor, config.borderWidth);
	}

	// draw line numbers bar
	if (config.bLineNumbers) {
		if (config.borderCorner>0.1) {
		ofxNanoVG::one().fillRoundedRect(0.5*config.borderWidth, 0.5*config.borderWidth, config.lineNumbersWidth, getHeight()-config.borderWidth, config.borderCorner, 0, 0, config.borderCorner, config.lineNumbersBGColor);
		}
		else {
			ofxNanoVG::one().fillRect(0.5*config.borderWidth, 0.5*config.borderWidth, config.lineNumbersWidth, getHeight()-config.borderWidth, config.lineNumbersBGColor);
		}
	}

	//////////////////////////////
	// DRAW TEXT
	//////////////////////////////

	ofxNanoVG::one().setFillColor(config.fontColor);
	float y=0.5*config.borderWidth;

	// draw only visible part of text
	double firstLine;
	double frac = modf(view.y/config.fontSize, &firstLine);
	// figure out first and last lines
	caret_t first{(int)firstLine,0};
	caret_t last = toCaret(ofVec2f(0, getHeight()));
	ofxNanoVG::one().enableScissor(0.5*config.borderWidth, 0.5*config.borderWidth, getWidth()-config.borderWidth, getHeight()-config.borderWidth);
	ofPushMatrix();
	ofTranslate(-view.x, -frac*config.fontSize);
	ofxNanoVG::one().applyOFMatrix();
	for (int i=first.line; i<=last.line; i++) {
		string& line = textLines[i];
		if (config.bLineNumbers) {
			ofxNanoVG::one().setFillColor(config.lineNumbersColor);
			ofxNanoVG::one().setTextAlign(ofxNanoVG::NVG_ALIGN_RIGHT, ofxNanoVG::NVG_ALIGN_TOP);
			ofxNanoVG::one().drawText(font, 0.5*config.borderWidth+config.lineNumbersWidth, config.pad.y+y, ofToString(i+1), config.fontSize);
		}
		ofxNanoVG::one().setFillColor(config.fontColor);
		ofxNanoVG::one().setTextAlign(ofxNanoVG::NVG_ALIGN_LEFT, ofxNanoVG::NVG_ALIGN_TOP);
		ofxNanoVG::one().drawText(font, 0.5*config.borderWidth+config.lineNumbersWidth+config.pad.x, config.pad.y+y, line, config.fontSize);
		y += config.fontSize;
	}

	ofPopMatrix();
	ofxNanoVG::one().applyOFMatrix();

	///////////////////////////////
	// DRAW SELECTION
	///////////////////////////////

	if (state.selection.active) {
		// draw selection
		caret_t sc = state.selection.begin;
		caret_t	ec = state.selection.end;
		if (sc.line > ec.line) {
			std::swap(sc, ec);
		}
		ofVec2f sPos = toNode(sc);
		ofVec2f ePos = toNode(ec);
		if (sc.line == ec.line) {
			ofxNanoVG::one().fillRect(sPos.x, sPos.y, ePos.x-sPos.x, config.fontSize, config.selectionColor);
		}
		else {
			float rightEdge = getWidth()-0.5*config.borderWidth;
			float leftEdge = 0.5*config.borderWidth+config.lineNumbersWidth+config.pad.x;
			// first line
			ofxNanoVG::one().fillRect(sPos.x, sPos.y, rightEdge-sPos.x, config.fontSize, config.selectionColor);
			for (int l=sc.line+1; l<ec.line; l++) {
				ofVec2f lpos = toNode(l, 0);
				ofxNanoVG::one().fillRect(leftEdge, lpos.y, rightEdge-leftEdge, config.fontSize, config.selectionColor);
			}
			// last line
			ofxNanoVG::one().fillRect(leftEdge, ePos.y, ePos.x-leftEdge, config.fontSize, config.selectionColor);
		}
	}

	///////////////////////////////
	// DRAW SCROLL BARS
	///////////////////////////////

	float barStartY = config.borderCorner;
	float totalTextHeight = textLines.size()*config.fontSize;
	float totalBarHeight = getHeight()-config.borderCorner*2;
	float linesToHeightFactor = totalBarHeight / totalTextHeight;
	float fly = barStartY+view.y*linesToHeightFactor;
	float lly = barStartY+(view.y+view.height)*linesToHeightFactor;
	lly = ofClamp(lly, 0, getHeight()-config.borderCorner);
	ofxNanoVG::one().fillRect(getWidth()-0.5*config.borderWidth-4, fly, 2, lly-fly, config.borderColor);

	///////////////////////////////
	// DRAW CARET
	///////////////////////////////

	ofVec2f cPos = toNode(state.caret);
	float val = cos(caretBlink);
	if (val>0) {
		ofSetColor(255);
		ofxNanoVG::one().fillRect(cPos.x, cPos.y, 1, config.fontSize, config.fontColor);
	}
	
	ofxNanoVG::one().disableScissor();

	ofxNanoVG::one().endFrame();


	ofPopMatrix();
	fbo.end();
	fbo.getTexture().generateMipmap();
}

void ofxInterfaceEditor::allocateFbo(ofFbo& fbo)
{
	int reqW = int(getWidth()) + fboPad;
	int reqH = int(getHeight()) + fboPad;
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

	if (config.bLineNumbers) {
		p.x -= config.lineNumbersWidth;
	}
	p.x -= config.pad.x + 0.5*config.borderWidth;
	p.y -= config.pad.y + 0.5*config.borderWidth;
	p.x += state.targetView.x;
	p.y += state.targetView.y;

	c.line = p.y/config.fontSize;
	if (c.line < 0) {
		c.line = 0;
	}
	else if (c.line > textLines.size()-1) {
		c.line = textLines.size()-1;
	}
	string& line = textLines[c.line];

	c.chr = (p.x+0.5*config.letterSize.x)/config.letterSize.x;
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
	ofVec2f p(chr*config.letterSize.x, line*config.fontSize);
	p.x += 0.5*config.borderWidth+config.pad.x;
	p.y += 0.5*config.borderWidth+config.pad.y;
	p.x -= state.targetView.x;
	p.y -= state.targetView.y;

	if (config.bLineNumbers) {
		p.x += config.lineNumbersWidth;
	}

	return p;
}

size_t ofxInterfaceEditor::toTextPos(const caret_t& c)
{
	size_t pos=0;
	for (unsigned int l=0; l<c.line && l<textLines.size(); l++) {
		pos += textLines[l].size()+1;
	}
	pos += c.chr;
	return pos;
}

ofxInterfaceEditor::caret_t ofxInterfaceEditor::toCaret(size_t textPos)
{
	caret_t c{0,0};
	while (textPos>0 && textLines.size()>c.line) {
		if (textLines[c.line].size() < textPos) {
			// textPos is beyond this line
			textPos -= textLines[c.line].size()+1;
			c.line++;
		}
		else {
			// text pos is in this line
			c.chr = textPos;
			break;
		}
	}
	return c;
}

void ofxInterfaceEditor::onTouchDown(TouchEvent& event)
{
	ofVec2f local = toLocal(event.position);
	state.selection.begin = state.selection.end = state.caret = toCaret(local);
	state.selection.active = false;
	caretBlink = 0;
}

void ofxInterfaceEditor::onTouchMove(TouchEvent& event)
{
	state.selection.end = toCaret(toLocal(event.position));
	state.selection.active = true;
}

void ofxInterfaceEditor::onTouchUp(TouchEvent& event)
{
	if (state.selection.begin.chr == state.selection.end.chr &&
		state.selection.begin.line == state.selection.end.line) {
		state.selection.active = false;
	}
}

void ofxInterfaceEditor::clearSelection()
{
	caret_t sc = state.selection.begin;
	caret_t	ec = state.selection.end;

	if (sc.line == ec.line) {
		if (sc.chr > ec.chr) {
			std::swap(sc, ec);
		}
		textLines[sc.line].erase(textLines[sc.line].begin()+sc.chr, textLines[sc.line].begin()+(ec.chr));
	}
	else {
		if (sc.line > ec.line) {
			std::swap(sc, ec);
		}

		// first line
		textLines[sc.line].erase(textLines[sc.line].begin()+sc.chr, textLines[sc.line].end());
		// last line
		textLines[ec.line].erase(textLines[ec.line].begin(), textLines[ec.line].begin()+ec.chr);
		// merge last line with first above
		textLines[sc.line].append(textLines[ec.line]);

		// in between
		for (int l=ec.line; l>sc.line; l--) {
			textLines.erase(textLines.begin()+l);
		}
	}

	state.caret = sc;
	state.selection.active = false;
}

float ofxInterfaceEditor::getLineNumberWidth()
{
	float w=0;
	int linesNum = textLines.size();
	while (linesNum>0) {
		w += config.letterSize.x;
		linesNum/=10;
	}

	return w;
}

void ofxInterfaceEditor::refreshView()
{
	// Update view

	ofVec2f cPos = toNode(state.caret);
	while (cPos.y > getHeight()-0.5*config.fontSize) {	// if caret goes below view height
		state.targetView.y += config.fontSize;				// advance one line
		cPos = toNode(state.caret);
	}

	while (cPos.y < 0) {								// if caret goes above view y
		state.targetView.y -= config.fontSize;
		if (state.targetView.y < 0) {
			state.targetView.y = 0;
		}
		cPos = toNode(state.caret);
	}

	caret_t c{(int)textLines.size()-1, 0};
	while(int(state.targetView.y/config.fontSize)>0 && toNode(c).y<getHeight()-1.5*config.fontSize) {												// if last line is above view height and first line is not 0
		state.targetView.y -= config.fontSize;
		if (state.targetView.y < 0) {
			state.targetView.y = 0;
		}
	}
}

string ofxInterfaceEditor::getSelectedText(size_t* _bpos, size_t* _epos)
{
	if (!state.selection.active) {
		return "";
	}

	string str = getText();
	caret_t sb = state.selection.begin;
	caret_t se = state.selection.end;
	if (((sb.line == se.line) && (sb.chr > se.chr)) ||
		sb.line > se.line) {
		swap(sb, se);
	}
	size_t bpos = toTextPos(sb);
	size_t epos = toTextPos(se);
	if (_bpos != NULL) {
		*_bpos = bpos;
	}
	if (_epos != NULL) {
		*_epos = epos;
	}
	return str.substr(bpos, epos-bpos);
}

void ofxInterfaceEditor::pushUndoState()
{
	state.text = getText();
	undoStates.push(state);
}

void ofxInterfaceEditor::popUndoState()
{
	if (undoStates.empty()) {
		return;
	}

	pushRedoState();

	state = undoStates.top();
	undoStates.pop();
	setText(state.text);
}

void ofxInterfaceEditor::pushRedoState()
{
	state.text = getText();
	redoStates.push(state);
}

void ofxInterfaceEditor::popRedoState()
{
	if (redoStates.empty()) {
		return;
	}

	pushUndoState();

	state = redoStates.top();
	redoStates.pop();
	setText(state.text);
}
