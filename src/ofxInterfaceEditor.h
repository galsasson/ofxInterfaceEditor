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
	~ofxInterfaceEditor();
	ofxInterfaceEditor();

	void setConfig(const Json::Value& config);
	void setText(const string& text);
	string getText();
	void loadTextFile(const string& file);
	void keyPressed(int key);
	void keyReleased(int key);
	void addChar(char ch);

private:
	Json::Value configJson;
	ofFbo lastRender;
	vector<string> textLines;
	bool bDirty;
	int fboPad;
	ofxNanoVG::Font* font;
	ofRectangle view;
	bool bShiftPressed;
	bool bCommandKeyPressed;
	bool bControlKeyPressed;
	string pasteboard;
	float caretBlink;

	struct caret_t {
		int line;
		int chr;
	};

	struct configcache_t {
		float width;
		int lines;
		float borderWidth;
		float borderCorner;
		float fontSize;
		ofVec2f pad;
		bool bLineNumbers;
		float lineNumbersWidth;
		ofColor fontColor;
		ofColor bgColor;
		ofColor borderColor;
		ofColor selectionColor;
		ofColor lineNumbersColor;
		ofColor lineNumbersBGColor;
		ofVec2f letterSize;
		bool bSpecialEnter;
	} config;

	struct selection_t {
		caret_t begin;
		caret_t end;
		bool active;
	};

	struct editor_state_t {
		string text;
		caret_t caret;
		selection_t selection;
		ofRectangle targetView;			// for animated scroll
	} state;							// current state
	stack<editor_state_t> undoStates;	// undo states
	stack<editor_state_t> redoStates;	// redos states

	void update(float dt);
	void draw();

	void renderToFbo(ofFbo& fbo);
	void allocateFbo(ofFbo& fbo);
	caret_t toCaret(ofVec2f p);
	ofVec2f toNode(const caret_t& caret);
	ofVec2f toNode(int line, int chr);
	size_t toTextPos(const caret_t& c);
	caret_t toCaret(size_t textPos);
	void onTouchDown(TouchEvent& event);
	void onTouchMove(TouchEvent& event);
	void onTouchUp(TouchEvent& event);
	void clearSelection();
	float getLineNumberWidth();
	void refreshView();
	string getSelectedText(size_t* bpos=NULL, size_t* epos=NULL);
	void pushUndoState();
	void popUndoState();
	void pushRedoState();
	void popRedoState();
};

#endif /* ofxInterfaceEditor_h */
