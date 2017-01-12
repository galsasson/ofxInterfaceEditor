//
//  ofxInterfaceTextEditor.cpp
//  example_single
//
//  Created by Gal Sasson on 10/9/16.
//
//

#include "ofxInterfaceTextEditor.h"
#include "ofxJsonParser.h"

ofxInterfaceTextEditor* ofxInterfaceTextEditor::focusedEditor = NULL;
vector<ofxInterfaceTextEditor*> ofxInterfaceTextEditor::allEditors;
string ofxInterfaceTextEditor::internalPasteboard = "";

ofxInterfaceTextEditor::~ofxInterfaceTextEditor()
{
	// remove this text editor
	allEditors.erase(std::remove(allEditors.begin(), allEditors.end(), this), allEditors.end());
	if (focusedEditor == this) {
		focusedEditor = NULL;
	}
}

ofxInterfaceTextEditor::ofxInterfaceTextEditor()
{
	// default config
	configJson = Json::objectValue;
	configJson["width"] =					80;	// in chars
	configJson["lines"] =					20;	// in lines
	configJson["max-lines"] =				-1;
	configJson["padx"] =					6;	// when text starts in a line
	configJson["background-color"] =		"#111111 100%";
	configJson["border-width"] =			2;
	configJson["border-color"] =			"#ffffff 100%";
	configJson["border-corner"] =			14;
	configJson["font"] =					"Inconsolata-Regular.ttf";
	configJson["font-color"] =				"#ffffff 100%";
	configJson["font-size"] =				22;
	configJson["line-numbers"] =			false;
	configJson["line-numbers-color"] =		"#ffffff 100%";
	configJson["line-numbers-bg-color"] =	"#434445 100%";
	configJson["selection-color"] =			"#aaaaaa 50%";
	configJson["draggable"] =				false;
	configJson["title"] =					false;
	configJson["title-text"] =				"Text Editor";
	configJson["tab-width"] =				2;
	configJson["focus-color"] =				"#ddaa00 100%";
	configJson["focus-width"] =				0;

	// setup nanovg
	ofxNanoVG::one().setup();

	// register touch events
	ofAddListener(eventTouchDown, this, &ofxInterfaceTextEditor::onTouchDown);
	ofAddListener(eventTouchMove, this, &ofxInterfaceTextEditor::onTouchMove);
	ofAddListener(eventTouchUp, this, &ofxInterfaceTextEditor::onTouchUp);

	// init variables
	fboPad =			256;
	bDirty =			0;
	font =				NULL;
	caretBlink =		0;
	bShiftPressed =		false;
	bControlKeyPressed =false;
	bCommandKeyPressed =false;
	bInDrag =			false;
	state.caret.line =	0;
	state.caret.chr =	0;
	state.desiredChr =	0;
	bCollapsed =		false;
	copyTimer =			0;
	view = state.targetView =	ofRectangle(0, 0, 0, 0);
	textLines.push_back("");
	state.selection.active = false;

	setConfig(configJson);
	// add this text editor
	allEditors.push_back(this);
	requestFocus();
}

ofxInterfaceTextEditor::ofxInterfaceTextEditor(const Json::Value& _config) : ofxInterfaceTextEditor()
{
	setConfig(_config);
}

void ofxInterfaceTextEditor::setConfig(const Json::Value& conf)
{
	ofxJsonParser::objectMerge(configJson, conf);

	// setup font
	string fontName = configJson["font"].asString();
	if ((font = ofxNanoVG::one().getFont(fontName)) == NULL) {
		font = ofxNanoVG::one().addFont(fontName, fontName);
	}

	// cache config value
	config.width = ofxJsonParser::parseInt(configJson["width"], 80);
	config.lines = ofxJsonParser::parseInt(configJson["lines"], 1);
	config.maxLines = ofxJsonParser::parseInt(configJson["max-lines"], -1);
	config.borderWidth = ofxJsonParser::parseFloat(configJson["border-width"]);
	config.borderCorner = ofxJsonParser::parseFloat(configJson["border-corner"]);
	config.fontSize = ofxJsonParser::parseFloat(configJson["font-size"]);
	config.padx = ofxJsonParser::parseFloat(configJson["padx"]);
	config.bLineNumbers = ofxJsonParser::parseBool(configJson["line-numbers"]);
	config.lineNumbersColor = ofxJsonParser::parseColor(configJson["line-numbers-color"]);
	config.lineNumbersBGColor = ofxJsonParser::parseColor(configJson["line-numbers-bg-color"]);
	config.fontColor = ofxJsonParser::parseColor(configJson["font-color"]);
	config.bgColor = ofxJsonParser::parseColor(configJson["background-color"]);
	config.borderColor = ofxJsonParser::parseColor(configJson["border-color"]);
	config.selectionColor = ofxJsonParser::parseColor(configJson["selection-color"]);
	ofxNanoVG::one().resetMatrix();
	float lw = ofxNanoVG::one().getTextBounds(font, 0, 0, "aba", config.fontSize).width - ofxNanoVG::one().getTextBounds(font, 0, 0, "aa", config.fontSize).width;
	config.letterSize = ofVec2f(lw, config.fontSize);
	config.bDraggable = ofxJsonParser::parseBool(configJson["draggable"]);
	config.bTitle = ofxJsonParser::parseBool(configJson["title"]);
	if (config.bTitle) {
		config.titleString = configJson["title-text"].asString();
		config.titleBarHeight = config.fontSize;
		setName(config.titleString);
	}
	else {
		config.titleString = "";
		config.titleBarHeight = 0;
	}
	config.tabWidth = ofxJsonParser::parseInt(configJson["tab-width"]);
	config.tabString = "";
	for (int i=0; i<config.tabWidth; i++) {
		config.tabString = config.tabString+" ";
	}
	config.focusColor = ofxJsonParser::parseColor(configJson["focus-color"]);
	config.focusWidth = ofxJsonParser::parseFloat(configJson["focus-width"]);

	// set size
	state.targetView = view = ofRectangle(0, 0, config.width*lw, config.lines*config.fontSize);
	setSize(view.width+config.borderWidth*2, view.height+config.titleBarHeight+2*config.borderWidth);

	bDirty = 0.5;
}

void ofxInterfaceTextEditor::setTitle(const string& name)
{
	config.titleString = name;
	setName(name);
}

void ofxInterfaceTextEditor::loadConfig(const string& filename)
{
	if (!ofFile(filename).exists()) {
		ofLogError("ofxInterfaceTextEditor") << "could not find the config file: "<<filename;
		return;
	}
	ofxJSONElement json;
	if (!json.open(filename)) {
		ofLogError("ofxInterfaceTextEditor") << "could not parse the config file: "<<filename;
		return;
	}
	setConfig(json);
}

void ofxInterfaceTextEditor::update(float dt)
{
	ofVec2f offset = (state.targetView.getPosition()-view.getPosition());
	view.translate(0.5f*offset);

	// can be optimized (calc only on change);
	config.lineNumbersWidth = getLineNumberWidth();

	if (focusedEditor == this || bDirty>0) {
		renderToFbo(lastRender);
		bDirty -= dt;
	}

	caretBlink += 5*dt;
	if (copyTimer > 0) {
		copyTimer -= dt;
		if (copyTimer<0) {
			copyTimer = 0;
		}
	}
}

void ofxInterfaceTextEditor::draw()
{
	ofSetColor(255);
	lastRender.draw(-fboPad/2, -fboPad/2);
}

bool ofxInterfaceTextEditor::contains(const ofVec3f& global)
{
	if (bCollapsed) {
		ofVec2f local = toLocal(global);
		if (local.x < 0 || local.x > getWidth() || local.y < 0 || local.y > config.titleBarHeight) {
			return false;
		}
		return true;
	}
	else {
		return Node::contains(global);
	}
}

void ofxInterfaceTextEditor::requestFocus()
{
	if (focusedEditor != NULL) {
		focusedEditor->bDirty = 0.5;
	}

	focusedEditor = this;
}

void ofxInterfaceTextEditor::requestFocus(ofxInterfaceTextEditor* editor)
{
	if (focusedEditor != NULL) {
		focusedEditor->bDirty = 0.5;
	}

	focusedEditor = editor;
}

ofxInterfaceTextEditor* ofxInterfaceTextEditor::getFocused()
{
	return focusedEditor;
}

vector<ofxInterfaceTextEditor*>& ofxInterfaceTextEditor::getAllEditors()
{
	return allEditors;
}

void ofxInterfaceTextEditor::setText(const string &text, bool clearUndo)
{
	if (clearUndo) {
		undoStates = std::stack<editor_state_t>();
		redoStates = std::stack<editor_state_t>();
	}

	textLines = ofSplitString(text, "\n", false, false);
	if (textLines.empty()) {
		textLines.push_back("");
	}
	if (config.maxLines>0 && textLines.size()>config.maxLines) {
		textLines.erase(textLines.begin()+1, textLines.end());
	}

	// clamp caret
	state.caret.line = state.caret.line<0?0:state.caret.line>textLines.size()-1?textLines.size()-1:state.caret.line;
	state.caret.chr = state.caret.chr<0?0:state.caret.chr>textLines[state.caret.line].length()?textLines[state.caret.line].length():state.caret.chr;

	// clear select state
	state.selection.active = false;

	bringViewToCaret();
	limitView();
}

string ofxInterfaceTextEditor::getText()
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

string ofxInterfaceTextEditor::getLine(size_t i)
{
	if (i<textLines.size()) {
		return textLines[i];
	}
	return "";
}

void ofxInterfaceTextEditor::loadFromFile(const string &filename)
{
	ofFile file(filename);
	if (!file.exists()) {
		ofLogError("ofxInterfaceTextEditor") << "No such file: "<<filename;
		return;
	}

	setText(file.readToBuffer().getText());
}

void ofxInterfaceTextEditor::saveToFile(const string &filename)
{
	ofFile file(filename, ofFile::WriteOnly);
	ofBuffer buffer;
	buffer.set(getText());
	file.writeFromBuffer(buffer);
}

void ofxInterfaceTextEditor::appendString(const string &str)
{
	string text = getText();
	size_t cPos = toTextPos(state.caret);
	bool caretAtEnd = cPos==text.size();
	setText(getText()+str);
	if (caretAtEnd) {
		state.caret = toCaret(cPos+str.size());
	}
}

void ofxInterfaceTextEditor::appendChar(char ch)
{
	string str;
	str += ch;
	appendString(str);
}

void ofxInterfaceTextEditor::keyPressed(int key)
{
	if (bCollapsed) {
		return;
	}

//	ofLog() << "Key Pressed = "<<key;
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
				if (bCommandKeyPressed) {
					state.caret.chr = 0;
					state.desiredChr=state.caret.chr;
					break;
				}
				state.caret.chr--;
				if (state.caret.chr < 0) {
					if (state.caret.line>0) {
						state.caret.line--;
						state.caret.chr = textLines[state.caret.line].size();
					}
				}
				state.desiredChr=state.caret.chr;
				break;
			case OF_KEY_UP:
				state.caret.line--;
				if (state.caret.chr < state.desiredChr) {
					state.caret.chr = state.desiredChr;
				}
				break;
			case OF_KEY_RIGHT:
				if (bCommandKeyPressed) {
					state.caret.chr = textLines[state.caret.line].size();
					state.desiredChr=state.caret.chr;
					break;
				}
				state.caret.chr++;
				if (state.caret.chr > textLines[state.caret.line].size()) {
					if (state.caret.line < textLines.size()-1) {
						state.caret.chr=0;
						state.caret.line++;
					}
				}
				state.desiredChr=state.caret.chr;
				break;
			case OF_KEY_DOWN:
				state.caret.line++;
				if (state.caret.chr < state.desiredChr) {
					state.caret.chr = state.desiredChr;
				}
				break;
			case OF_KEY_HOME:
				state.caret.chr = 0;
				state.desiredChr=state.caret.chr;
				break;
			case OF_KEY_END:
				state.caret.chr = textLines[state.caret.line].size();
				state.desiredChr=state.caret.chr;
				break;
			case OF_KEY_PAGE_UP:
				state.caret.line -= config.lines;
				break;
			case OF_KEY_PAGE_DOWN:
				state.caret.line += config.lines;
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
		bringViewToCaret();
	}
	else if (bControlKeyPressed || bCommandKeyPressed) {
		// Special commands
		if (key < 32) {
			key += bControlKeyPressed?32:96;
		}

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
				clipboardCopy(str.substr(bpos, size));
				str.erase(bpos, size);
				setText(str);
				state.selection.active = false;
				state.caret = sb;
			}
			bringViewToCaret();
		}
		else if (key == 'c') {				// Copy
			copyTimer += 0.2;
			if (state.selection.active) {
				clipboardCopy(getSelectedText());
			}
			else {
				clipboardCopy(textLines[state.caret.line] + "\n");
			}
			bringViewToCaret();
		}
		else if (key == 'v') {				// Paste
			string pasteboard = clipboardPaste();
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
				state.desiredChr=state.caret.chr;
				bringViewToCaret();
			}
		}
		else if (key == 'z') {				// Undo/Redo
			if (bShiftPressed) {			// Redo
				popRedoState();
			}
			else {							// Undo
				popUndoState();
			}
			bringViewToCaret();
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
		state.desiredChr=state.caret.chr;
		bringViewToCaret();
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
		state.desiredChr=state.caret.chr;
		bringViewToCaret();
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
		bringViewToCaret();
	}
	else if (key == OF_KEY_RETURN) {			// Enter
		if (!fireEvent(eventEnterDown)) {
			return;
		}

		if (state.caret.line==config.maxLines-1) {
			return;
		}

		pushUndoState();
		if (state.selection.active) {
			clearSelection();
		}
		// Normal enter behavior
		string afterCaret = textLines[state.caret.line].substr(state.caret.chr, textLines[state.caret.line].size());
		textLines[state.caret.line].erase(state.caret.chr, textLines[state.caret.line].size()-state.caret.chr);
		textLines.insert(textLines.begin()+state.caret.line+1, afterCaret);
		state.caret.line++;
		state.caret.chr=0;
		state.desiredChr=state.caret.chr;
		bringViewToCaret();
	}
	else if (key == OF_KEY_TAB) {
		if (!fireEvent(eventTabDown)) {
			return;
		}
		pushUndoState();
		if (state.selection.active) {
			clearSelection();
		}
		textLines[state.caret.line].insert(state.caret.chr, config.tabString);
		state.caret.chr+=config.tabString.size();
		state.desiredChr=state.caret.chr;
		bringViewToCaret();
	}

	limitView();
	caretBlink=0;
}

void ofxInterfaceTextEditor::keyReleased(int key)
{
	if (bCollapsed) {
		return;
	}

//	ofLog() << "Key Released = "<<key;
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

void ofxInterfaceTextEditor::vscroll(float amount)
{
	state.targetView.y -= 4*amount;
	limitView();
	bDirty = 1;
}

void ofxInterfaceTextEditor::vscroll(float x, float y, float amount)
{
	ofVec2f p(x, y);
	for (ofxInterfaceTextEditor* editor: allEditors) {
		if (editor->contains(p)) {
			editor->vscroll(amount);
		}
	}
}

void ofxInterfaceTextEditor::renderToFbo(ofFbo& fbo)
{
	// render
	allocateFbo(fbo);
	fbo.begin();
	ofDisableDepthTest();
	if (config.borderWidth>0.1) {
		ofClear(ofColor(config.borderColor, 0));
	}
	else {
		ofClear(ofColor(config.bgColor, 0));
	}
	glClear(GL_STENCIL_BUFFER_BIT);

	// begin frame and translate after the pad
	ofxNanoVG::one().beginFrame(fbo.getWidth(), fbo.getHeight(), 1);
	ofPushMatrix();
	ofTranslate(fboPad/2, fboPad/2);
	ofxNanoVG::one().applyOFMatrix();


	drawTextEditor();


	ofxNanoVG::one().endFrame();


	ofPopMatrix();
	fbo.end();
	fbo.getTexture().generateMipmap();
}

void ofxInterfaceTextEditor::drawTextEditor()
{
	float halfBW = 0.5*config.borderWidth;
	///////////////////////////////////
	// DRAW TITLE BAR (if draggable) //
	///////////////////////////////////

	if (config.bTitle) {
		if (config.borderCorner>0.1) {
			if (!bCollapsed) {
				ofxNanoVG::one().fillRoundedRect(0, 0, getWidth(), config.titleBarHeight, config.borderCorner, config.borderCorner, 0, 0, config.borderColor);
			}
			else {
				ofxNanoVG::one().fillRoundedRect(0, 0, getWidth(), config.titleBarHeight, config.borderCorner, config.borderColor);
			}
		}
		else {
			ofxNanoVG::one().fillRect(0, 0, getWidth(), config.titleBarHeight, config.borderColor);
		}
		ofxNanoVG::one().setTextAlign(ofxNanoVG::NVG_ALIGN_LEFT, ofxNanoVG::NVG_ALIGN_TOP);
		ofxNanoVG::one().setFillColor(config.bgColor);
		ofxNanoVG::one().drawText(font, 6, 0, config.titleString, config.fontSize);

		// Draw collapse button
		ofVec2f vec(0, -0.4*config.fontSize);
		if (bCollapsed) {
			// draw up arrow
			ofVec2f center(getWidth()-0.5*config.fontSize, 0.55*config.titleBarHeight);
			ofxNanoVG::one().beginPath();
			ofxNanoVG::one().moveTo(center+vec);
			ofxNanoVG::one().lineTo(center+vec.getRotated(120));
			ofxNanoVG::one().lineTo(center+vec.getRotated(-120));
			ofxNanoVG::one().lineTo(center+vec);
			ofxNanoVG::one().fillPath(config.bgColor);
			return;
		}
		else {
			// draw down arrow
			ofVec2f center(getWidth()-0.5*config.fontSize, 0.45*config.titleBarHeight);
			ofxNanoVG::one().beginPath();
			ofxNanoVG::one().moveTo(center+vec.getRotated(180));
			ofxNanoVG::one().lineTo(center+vec.getRotated(-60));
			ofxNanoVG::one().lineTo(center+vec.getRotated(60));
			ofxNanoVG::one().lineTo(center+vec.getRotated(180));
			ofxNanoVG::one().fillPath(config.bgColor);
		}
	}

	//////////////////////////////
	// DRAW BACKGROUND
	//////////////////////////////

	if (config.borderCorner>0.1) {
		// Rounded corners
		ofxNanoVG::one().fillRoundedRect(halfBW, config.titleBarHeight+halfBW, getWidth()-config.borderWidth, getHeight()-config.borderWidth-config.titleBarHeight, config.borderCorner, config.bgColor);
	}
	else {
		// Sharp corners
		ofxNanoVG::one().fillRect(halfBW, config.titleBarHeight+halfBW, getWidth()-config.borderWidth, getHeight()-config.borderWidth-config.titleBarHeight, config.bgColor);
	}

	ofxNanoVG::one().setFillColor(config.fontColor);
	// draw only visible part of text
	double firstLine;
	double frac = modf(view.y/config.fontSize, &firstLine);
	// figure out first and last lines
	caret_t first{(int)firstLine,0};
	caret_t last = toCaret(ofVec2f(0, getHeight()), view);

	//////////////////////////////
	// DRAW LINE NUMBERS BAR
	//////////////////////////////

	// draw line numbers bar
	if (config.bLineNumbers) {
		// fill bar
		if (config.borderCorner>0.1) {
			if (config.bTitle) {
				ofxNanoVG::one().fillRoundedRect(halfBW, config.titleBarHeight+halfBW, config.lineNumbersWidth+halfBW, getHeight()-config.borderWidth-config.titleBarHeight, 0, 0, 0, config.borderCorner, config.lineNumbersBGColor);
			}
			else {
				ofxNanoVG::one().fillRoundedRect(halfBW, config.titleBarHeight+halfBW, config.lineNumbersWidth+halfBW, getHeight()-config.borderWidth-config.titleBarHeight, config.borderCorner, 0, 0, config.borderCorner, config.lineNumbersBGColor);
			}
		}
		else {
			ofxNanoVG::one().fillRect(halfBW, config.titleBarHeight+halfBW, config.lineNumbersWidth+halfBW, getHeight()-config.borderWidth-config.titleBarHeight, config.lineNumbersBGColor);
		}

		// draw numbers
		ofxNanoVG::one().enableScissor(config.borderWidth, config.titleBarHeight+config.borderWidth, getWidth()-2*config.borderWidth, getHeight()-config.titleBarHeight-2*config.borderWidth);
		float y=config.titleBarHeight+config.borderWidth+0.5*config.fontSize-frac*config.fontSize;
		float x = config.borderWidth+config.lineNumbersWidth-2;
		for (int i=first.line; i<=last.line; i++) {
			ofxNanoVG::one().setFillColor(config.lineNumbersColor);
			ofxNanoVG::one().setTextAlign(ofxNanoVG::NVG_ALIGN_RIGHT, ofxNanoVG::NVG_ALIGN_MIDDLE);
			ofxNanoVG::one().drawText(font, x, y, ofToString(i+1), 0.8*config.fontSize);
			y += config.fontSize;
		}
		ofxNanoVG::one().disableScissor();
	}


	// enable scissors for the draw calls below, limiting to visible text area
	ofRectangle textWindow(config.borderWidth+config.lineNumbersWidth+config.padx, config.titleBarHeight+config.borderWidth, getWidth()-2*config.borderWidth-config.lineNumbersWidth-config.padx, getHeight()-2*config.borderWidth-config.titleBarHeight);
	ofxNanoVG::one().enableScissor(textWindow.x, textWindow.y, textWindow.width, textWindow.height);

	//////////////////////////////
	// DRAW TEXT
	//////////////////////////////
	float y = config.titleBarHeight + config.borderWidth;
	float x = config.borderWidth+config.lineNumbersWidth+config.padx;
	ofPushMatrix();
	ofTranslate(-view.x, -frac*config.fontSize);
	ofxNanoVG::one().applyOFMatrix();
	for (int i=first.line; i<=last.line; i++) {
		string& line = textLines[i];
		ofxNanoVG::one().setFillColor(config.fontColor);
		ofxNanoVG::one().setTextAlign(ofxNanoVG::NVG_ALIGN_LEFT, ofxNanoVG::NVG_ALIGN_TOP);
		ofxNanoVG::one().drawText(font, x, y, line, config.fontSize);
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
		ofVec2f sPos = toNode(sc, view);
		ofVec2f ePos = toNode(ec, view);
		ofColor c = config.selectionColor;
		if (copyTimer>0) {
			c.a += ofMap(copyTimer, 0, 0.2, 0, 100, true);
		}
		if (sc.line == ec.line) {
			ofxNanoVG::one().fillRect(sPos.x, sPos.y, ePos.x-sPos.x, config.fontSize, c);
		}
		else {
			float rightEdge = getWidth()-halfBW;
			float leftEdge = halfBW+config.lineNumbersWidth+config.padx;
			// first line
			ofxNanoVG::one().fillRect(sPos.x, sPos.y, rightEdge-sPos.x, config.fontSize, c);
			for (int l=sc.line+1; l<ec.line; l++) {
				ofVec2f lpos = toNode(l, 0, view);
				ofxNanoVG::one().fillRect(leftEdge, lpos.y, rightEdge-leftEdge, config.fontSize, c);
			}
			// last line
			ofxNanoVG::one().fillRect(leftEdge, ePos.y, ePos.x-leftEdge, config.fontSize, c);
		}
	}

	///////////////////////////////
	// DRAW SCROLL BARS
	///////////////////////////////

	// Vertical scroll bar
	float barStartY = config.titleBarHeight + config.borderWidth + config.borderCorner + 2;
	float totalTextHeight = textLines.size()*config.fontSize;
	float totalBarHeight = getHeight()-config.titleBarHeight-2*config.borderWidth-2*config.borderCorner-4;
	float linesToHeightFactor = totalBarHeight / totalTextHeight;
	float fly = barStartY+view.y*linesToHeightFactor;
	float lly = barStartY+(view.y+view.height)*linesToHeightFactor;
	lly = ofClamp(lly, 0, barStartY+totalBarHeight);
	ofxNanoVG::one().fillRect(getWidth()-config.borderWidth-4, fly, 2, lly-fly, config.borderColor);

	///////////////////////////////
	// DRAW CARET
	///////////////////////////////
	if (focusedEditor == this) {
		// only if we have focus
		ofVec2f cPos = toNode(state.caret, view);
		if (cos(caretBlink)>0) {
			ofSetColor(255);
			ofxNanoVG::one().fillRect(cPos.x-1, cPos.y, 2, config.fontSize, config.fontColor);
		}
	}

	// disable scissors
	ofxNanoVG::one().disableScissor();

	//////////////////////////////
	// DRAW BORDER
	//////////////////////////////
	if (config.borderWidth>0.01) {
		if (config.borderCorner>0.1) {
			// Rounded corners
			if (config.bTitle) {
				// with title bar so top should be sharp corners
				ofxNanoVG::one().strokeRoundedRect(halfBW, halfBW+config.titleBarHeight, getWidth()-config.borderWidth, getHeight()-config.borderWidth-config.titleBarHeight, 0, 0, config.borderCorner, config.borderCorner, config.borderColor, config.borderWidth);
			}
			else {
				// no title bar so all is round
				ofxNanoVG::one().strokeRoundedRect(halfBW, halfBW+config.titleBarHeight, getWidth()-config.borderWidth, getHeight()-config.borderWidth-config.titleBarHeight, config.borderCorner, config.borderColor, config.borderWidth);
			}
		}
		else {
			// Sharp corners
			ofxNanoVG::one().strokeRect(halfBW, halfBW+config.titleBarHeight, getWidth()-config.borderWidth, getHeight()-config.borderWidth-config.titleBarHeight, config.borderColor, config.borderWidth);
		}
	}

	//////////////////////////////
	// DRAW FOCUS
	//////////////////////////////
	if (config.focusWidth > 0.01 && focusedEditor == this && allEditors.size()>1) {
		if (config.borderCorner>0.1) {
			// no title bar so all is round
			ofxNanoVG::one().strokeRoundedRect(0.5*config.focusWidth, 0.5*config.focusWidth, getWidth()-config.focusWidth, getHeight()-config.focusWidth, config.borderCorner, config.focusColor, config.focusWidth);
		}
		else {
			// Sharp corners
			ofxNanoVG::one().strokeRect(0.5*config.focusWidth, 0.5*config.focusWidth, getWidth()-config.focusWidth, getHeight()-config.focusWidth, config.focusColor, config.focusWidth);
		}
	}
}

void ofxInterfaceTextEditor::allocateFbo(ofFbo& fbo)
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

ofxInterfaceTextEditor::caret_t ofxInterfaceTextEditor::toCaret(ofVec2f p, ofRectangle& _view)
{
	struct caret_t c{
		0,
		0
	};

	if (config.bLineNumbers) {
		p.x -= config.lineNumbersWidth;
	}
	p.x -= config.padx + config.borderWidth;
	p.y -= config.titleBarHeight + config.borderWidth;
	p.x += _view.x;
	p.y += _view.y;

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

ofVec2f ofxInterfaceTextEditor::toNode(const ofxInterfaceTextEditor::caret_t& _caret, ofRectangle& _view)
{
	return toNode(_caret.line, _caret.chr, _view);
}

ofVec2f ofxInterfaceTextEditor::toNode(int line, int chr, ofRectangle& _view)
{
	ofVec2f p(chr*config.letterSize.x, line*config.fontSize);
	p.x += config.borderWidth+config.padx;
	p.y += config.titleBarHeight + config.borderWidth;
	p.x -= _view.x;
	p.y -= _view.y;

	if (config.bLineNumbers) {
		p.x += config.lineNumbersWidth;
	}

	return p;
}

size_t ofxInterfaceTextEditor::toTextPos(const caret_t& c)
{
	size_t pos=0;
	for (unsigned int l=0; l<c.line && l<textLines.size(); l++) {
		pos += textLines[l].size()+1;
	}
	pos += c.chr;
	return pos;
}

ofxInterfaceTextEditor::caret_t ofxInterfaceTextEditor::toCaret(size_t textPos)
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

void ofxInterfaceTextEditor::onTouchDown(TouchEvent& event)
{
	requestFocus();

	ofVec2f local = toLocal(event.position);
	if (local.y < config.titleBarHeight) {
		if (local.x > getWidth()-config.fontSize) {
			// touch to collapse button
			bCollapsed = !bCollapsed;
			if (bCollapsed) {
				if (focusedEditor == this) {
					ofxInterfaceTextEditor::requestFocus(NULL);
				}
			}
		}
		else if (config.bDraggable) {
			// touch on title
			bInDrag = true;
		}
	}
	else if (!bCollapsed) {
		state.selection.begin = state.selection.end = state.caret = toCaret(local, view);
		state.selection.active = false;
		state.desiredChr = state.caret.chr;
		caretBlink = 0;
		bringViewToCaret();
	}
}

void ofxInterfaceTextEditor::onTouchMove(TouchEvent& event)
{
	ofVec2f local = toLocal(event.position);
	if (bInDrag) {
		ofVec3f m = event.position-event.prevPosition;
		move(m);
	}
	else if (!bCollapsed && local.y > config.titleBarHeight) {
		state.selection.end = toCaret(local, view);
		state.selection.active = true;
	}
}

void ofxInterfaceTextEditor::onTouchUp(TouchEvent& event)
{
	if (bInDrag) {
		bInDrag=false;
	}
	else if (state.selection.begin.chr == state.selection.end.chr &&
		state.selection.begin.line == state.selection.end.line) {
		state.selection.active = false;
	}
}

void ofxInterfaceTextEditor::clearSelection()
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

float ofxInterfaceTextEditor::getLineNumberWidth()
{
	if (!config.bLineNumbers) {
		return 0;
	}

	float w=5;
	int linesNum = textLines.size();
	while (linesNum>0) {
		w += 0.8*config.letterSize.x;
		linesNum/=10;
	}

	return w;
}

void ofxInterfaceTextEditor::bringViewToCaret()
{
	// Update view
	ofVec2f cPos = toNode(state.caret, state.targetView);

	// check y
	if (cPos.y < (config.titleBarHeight+config.borderWidth)) {
		state.targetView.y += cPos.y-(config.titleBarHeight+config.borderWidth);
	}
	else if (cPos.y > getHeight()-config.borderWidth-0.5*config.fontSize) {
		state.targetView.y += cPos.y-(getHeight()-config.borderWidth-config.fontSize);
	}

	// check x
	float rightLimit = getWidth()-config.borderWidth-2*config.letterSize.x;
	float leftLimit = config.borderWidth+config.lineNumbersWidth+config.padx+5*config.letterSize.x;
	if (cPos.x < leftLimit) {
		state.targetView.x += cPos.x-leftLimit;
		if (state.targetView.x<0) {
			state.targetView.x=0;
		}
	}
	else if (cPos.x > rightLimit) {
		state.targetView.x += cPos.x-rightLimit;
	}
}

void ofxInterfaceTextEditor::limitView()
{
	if (state.targetView.y < 0) {
		state.targetView.y = 0;
	}
	else {
		float totalTextHeight = textLines.size()*config.fontSize;
		if (totalTextHeight>=state.targetView.height) {
			if (state.targetView.y+state.targetView.height>=totalTextHeight) {
				state.targetView.y = totalTextHeight-state.targetView.height;
			}
		}
		else {
			state.targetView.y = 0;
		}
	}
}

string ofxInterfaceTextEditor::getSelectedText(size_t* _bpos, size_t* _epos)
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

ofxInterfaceTextEditor::caret_t ofxInterfaceTextEditor::getCaret()
{
	return state.caret;
}

size_t ofxInterfaceTextEditor::getCaretPos()
{
	return toTextPos(state.caret);
}

void ofxInterfaceTextEditor::flashSelectedText(float time)
{
	copyTimer = time;
}

void ofxInterfaceTextEditor::pushUndoState()
{
	state.text = getText();
	undoStates.push(state);
}

void ofxInterfaceTextEditor::popUndoState()
{
	if (undoStates.empty()) {
		return;
	}

	pushRedoState();

	state = undoStates.top();
	undoStates.pop();
	setText(state.text);
}

void ofxInterfaceTextEditor::pushRedoState()
{
	state.text = getText();
	redoStates.push(state);
}

void ofxInterfaceTextEditor::popRedoState()
{
	if (redoStates.empty()) {
		return;
	}

	pushUndoState();

	state = redoStates.top();
	redoStates.pop();
	setText(state.text);
}

bool ofxInterfaceTextEditor::fireEvent(ofEvent<ofxInterfaceTextEditor::EventArgs> &event)
{
	struct EventArgs args{this,true};
	ofNotifyEvent(event, args, this);
	return args.continueNormalBehavior;
}


//---------------------------
// Copy & Paste
//---------------------------

void ofxInterfaceTextEditor::clipboardCopy(const string & content) {
	ofAppGLFWWindow * window = (ofAppGLFWWindow *)ofGetWindowPtr();
	if (window) {
		glfwSetClipboardString(window->getGLFWWindow(), content.c_str());
	}
	else {
		internalPasteboard = content;
	}
}

string ofxInterfaceTextEditor::clipboardPaste() {
	ofAppGLFWWindow * window = (ofAppGLFWWindow *)ofGetWindowPtr();
	if (window) {
		return string(glfwGetClipboardString(window->getGLFWWindow()));
	}
	else {
		return internalPasteboard;
	}
}
