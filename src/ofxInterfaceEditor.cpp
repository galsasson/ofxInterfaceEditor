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
	config["width"] = 700;
	config["lines"] = 20;
	config["pad"][0] = 6;
	config["pad"][1] = 0;
	config["background-color"] = "#222322 100%";
	config["border-width"] = 2;
	config["border-color"] = "#ffffff 100%";
	config["border-corner"] = 10;
	config["font"] = "Inconsolata-Regular.ttf";
	config["font-color"] = "#ffffff 100%";
	config["font-size"] = 22;
	config["line-numbers"] = true;
	config["line-numbers-color"] = "#ffffff 100%";
	config["line-numbers-bg-color"] = "#434445 100%";
	config["selection-color"] = "#aaaaaa 50%";
	config["special-enter"] = false;
	config["initial-text"] = "Write here...";

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

	loadConfig(config);
}

void ofxInterfaceEditor::loadConfig(const Json::Value& conf)
{
	ofxJsonParser::objectMerge(config, conf);

	// setup font
	string fontName = config["font"].asString();
	if ((font = ofxNanoVG::one().getFont(fontName)) == NULL) {
		font = ofxNanoVG::one().addFont(fontName, fontName);
	}

	// cache config value
	cache.width = ofxJsonParser::parseFloat(config["width"], 500);
	cache.lines = ofxJsonParser::parseInt(config["lines"], 1);
	cache.borderWidth = ofxJsonParser::parseFloat(config["border-width"]);
	cache.borderCorner = ofxJsonParser::parseFloat(config["border-corner"]);
	cache.fontSize = ofxJsonParser::parseFloat(config["font-size"]);
	cache.pad = ofxJsonParser::parseVector(config["pad"]);
	cache.bLineNumbers = ofxJsonParser::parseBool(config["line-numbers"]);
	cache.lineNumbersColor = ofxJsonParser::parseColor(config["line-numbers-color"]);
	cache.lineNumbersBGColor = ofxJsonParser::parseColor(config["line-numbers-bg-color"]);
	cache.fontColor = ofxJsonParser::parseColor(config["font-color"]);
	cache.bgColor = ofxJsonParser::parseColor(config["background-color"]);
	cache.borderColor = ofxJsonParser::parseColor(config["border-color"]);
	cache.selectionColor = ofxJsonParser::parseColor(config["selection-color"]);
	float lw = ofxNanoVG::one().getTextBounds(font, 0, 0, "ab", cache.fontSize).width - ofxNanoVG::one().getTextBounds(font, 0, 0, "b", cache.fontSize).width;
	cache.letterSize = ofVec2f(lw, cache.fontSize);
	cache.bSpecialEnter = ofxJsonParser::parseBool(config["special-enter"]);


	// set size
	setSize(cache.width, cache.fontSize*cache.lines+cache.borderWidth);
	view = ofRectangle(0, 0, getWidth(), getHeight());

	bDirty = true;
}

void ofxInterfaceEditor::update(float dt)
{
	ofVec2f offset = (state.targetView.getPosition()-view.getPosition());
	view.translate(0.5f*offset);

	// can be optimized (calc only on change);
	cache.lineNumbersWidth = getLineNumberWidth();

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
		if (cache.bSpecialEnter) {
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
	ofClear(255, 255, 255, 0);
	glClear(GL_STENCIL_BUFFER_BIT);

	// render NanoVG elements
	ofxNanoVG::one().beginFrame(fbo.getWidth(), fbo.getHeight(), 1);
	ofPushMatrix();
	ofTranslate(fboPad/2, fboPad/2);
	ofxNanoVG::one().applyOFMatrix();

	if (cache.borderCorner>0.1) {
		// Rounded corners
		// draw background
		ofxNanoVG::one().fillRoundedRect(0, 0, getWidth(), getHeight(), cache.borderCorner, cache.bgColor);
		// draw frame
		ofxNanoVG::one().strokeRoundedRect(0, 0, getWidth(), getHeight(), cache.borderCorner, cache.borderColor, cache.borderWidth);
	}
	else {
		// Sharp corners
		// draw background
		ofxNanoVG::one().fillRect(0, 0, getWidth(), getHeight(), cache.bgColor);
		// draw frame
		ofxNanoVG::one().strokeRect(0, 0, getWidth(), getHeight(), cache.borderColor, cache.borderWidth);
	}

	// draw line numbers bar
	if (cache.bLineNumbers) {
		ofxNanoVG::one().fillRoundedRect(0.5*cache.borderWidth, 0.5*cache.borderWidth, cache.lineNumbersWidth, getHeight()-cache.borderWidth, cache.borderCorner, 0, 0, cache.borderCorner, cache.lineNumbersBGColor);
	}


	//////////////////////////////
	// DRAW TEXT
	//////////////////////////////

	ofxNanoVG::one().setFillColor(cache.fontColor);
	float y=0.5*cache.borderWidth;

	// draw only visible part of text
	double firstLine;
	double frac = modf(view.y/cache.fontSize, &firstLine);
	// figure out first and last lines
	caret_t first{(int)firstLine,0};
	caret_t last = toCaret(ofVec2f(0, getHeight()));
	ofxNanoVG::one().enableScissor(0.5*cache.borderWidth, 0.5*cache.borderWidth, getWidth()-cache.borderWidth, getHeight()-cache.borderWidth);
	ofPushMatrix();
	ofTranslate(-view.x, -frac*cache.fontSize);
	ofxNanoVG::one().applyOFMatrix();
	for (int i=first.line; i<=last.line; i++) {
		string& line = textLines[i];
		if (cache.bLineNumbers) {
			ofxNanoVG::one().setFillColor(cache.lineNumbersColor);
			ofxNanoVG::one().setTextAlign(ofxNanoVG::NVG_ALIGN_RIGHT, ofxNanoVG::NVG_ALIGN_TOP);
			ofxNanoVG::one().drawText(font, 0.5*cache.borderWidth+cache.lineNumbersWidth, cache.pad.y+y, ofToString(i+1), cache.fontSize);
		}
		ofxNanoVG::one().setFillColor(cache.fontColor);
		ofxNanoVG::one().setTextAlign(ofxNanoVG::NVG_ALIGN_LEFT, ofxNanoVG::NVG_ALIGN_TOP);
		ofxNanoVG::one().drawText(font, 0.5*cache.borderWidth+cache.lineNumbersWidth+cache.pad.x, cache.pad.y+y, line, cache.fontSize);
		y += cache.fontSize;
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
			ofxNanoVG::one().fillRect(sPos.x, sPos.y, ePos.x-sPos.x, cache.fontSize, cache.selectionColor);
		}
		else {
			float rightEdge = getWidth()-0.5*cache.borderWidth;
			float leftEdge = 0.5*cache.borderWidth+cache.lineNumbersWidth+cache.pad.x;
			// first line
			ofxNanoVG::one().fillRect(sPos.x, sPos.y, rightEdge-sPos.x, cache.fontSize, cache.selectionColor);
			for (int l=sc.line+1; l<ec.line; l++) {
				ofVec2f lpos = toNode(l, 0);
				ofxNanoVG::one().fillRect(leftEdge, lpos.y, rightEdge-leftEdge, cache.fontSize, cache.selectionColor);
			}
			// last line
			ofxNanoVG::one().fillRect(leftEdge, ePos.y, ePos.x-leftEdge, cache.fontSize, cache.selectionColor);
		}
	}

	///////////////////////////////
	// DRAW CARET
	///////////////////////////////

	ofVec2f cPos = toNode(state.caret);
	float val = cos(caretBlink);
	if (val>0) {
		ofSetColor(255);
		ofxNanoVG::one().fillRect(cPos.x, cPos.y, 1, cache.fontSize, cache.fontColor);
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

	if (cache.bLineNumbers) {
		p.x -= cache.lineNumbersWidth;
	}
	p.x -= cache.pad.x + 0.5*cache.borderWidth;
	p.y -= cache.pad.y + 0.5*cache.borderWidth;
	p.x += state.targetView.x;
	p.y += state.targetView.y;

	c.line = p.y/cache.fontSize;
	if (c.line < 0) {
		c.line = 0;
	}
	else if (c.line > textLines.size()-1) {
		c.line = textLines.size()-1;
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
	p.x -= state.targetView.x;
	p.y -= state.targetView.y;

	if (cache.bLineNumbers) {
		p.x += cache.lineNumbersWidth;
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
		w += cache.letterSize.x;
		linesNum/=10;
	}

	return w;
}

void ofxInterfaceEditor::refreshView()
{
	// Update view
	ofVec2f cPos = toNode(state.caret);
	while (cPos.y > getHeight()-0.5*cache.fontSize) {	// if caret goes outside of the view
		state.targetView.y += cache.fontSize;				// advance one line
		cPos = toNode(state.caret);
	}

	while (cPos.y < 0) {
		state.targetView.y -= cache.fontSize;
		if (state.targetView.y < 0) {
			state.targetView.y = 0;
		}
		cPos = toNode(state.caret);
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
