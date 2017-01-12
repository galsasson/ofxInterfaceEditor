//
//  ofxInterfaceTextEditor.h
//  example_single
//
//  Created by Gal Sasson on 10/9/16.
//
//

#ifndef ofxInterfaceTextEditor_h
#define ofxInterfaceTextEditor_h

#include "ofxInterface.h"
#include "ofxJSON.h"
#include "ofxNanoVG.h"

using namespace ofxInterface;

class ofxInterfaceTextEditor : public Node
{
public:
	struct caret_t {
		int line;
		int chr;
	};

	~ofxInterfaceTextEditor();
	ofxInterfaceTextEditor();
	ofxInterfaceTextEditor(const Json::Value& config);

	void loadConfig(const string& filename);	// load config from file
	void setConfig(const Json::Value& config);	// set editor config object
	void setTitle(const string& name);

	void loadFromFile(const string& filename);	// load text from file
	void saveToFile(const string& filename);	// save text to file

	void setText(const string& text, bool clearUndo=false);		// set editor text
	string getText();							// get editor text
	string getLine(size_t i);
	void appendString(const string& str);		// add text to end of buffer
	void appendChar(char ch);					// add char to end of buffer
	string getSelectedText(size_t* bpos=NULL, size_t* epos=NULL);	// bpos and epos are optional arguments
	caret_t getCaret();
	size_t getCaretPos();
	void flashSelectedText(float time);

	// get editor events
	struct EventArgs {
		ofxInterfaceTextEditor* editor;
		bool continueNormalBehavior;
	};
	ofEvent<EventArgs> eventEnterDown;
	ofEvent<EventArgs> eventTabDown;

	void keyPressed(int key);	// use to pass keyboard events
	void keyReleased(int key);	// use to pass keyboard events
	void vscroll(float amount);	// use to pass vscroll events
	static void vscroll(float x, float y, float amount);	// static vscroll that checks if the mouse is above the component

	// Node orerrides
	void update(float dt) override;
	void draw() override;
	bool contains(const ofVec3f& global) override;

	// Focus handling
	static ofxInterfaceTextEditor* getFocused();	// use this to pass keyboard events when there is more than one text editor
	void requestFocus();
	static void requestFocus(ofxInterfaceTextEditor* editor);
	static vector<ofxInterfaceTextEditor*>& getAllEditors();


protected:
	Json::Value configJson;
	ofFbo lastRender;
	vector<string> textLines;
	float bDirty;
	int fboPad;
	ofxNanoVG::Font* font;
	ofRectangle view;			// current view rectangle
	bool bShiftPressed;
	bool bCommandKeyPressed;
	bool bControlKeyPressed;
	static string internalPasteboard;
	float caretBlink;
	bool bInDrag;
	bool bCollapsed;
	float copyTimer;

	struct configcache_t {
		float width;
		int lines;
		float borderWidth;
		float borderCorner;
		float fontSize;
		float padx;
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
		bool bDraggable;
		bool bTitle;
		string titleString;
		float titleBarHeight;
		ofColor titleColor;
		int maxLines;
		int tabWidth;
		string tabString;
		ofColor focusColor;
		float focusWidth;
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
		size_t desiredChr;
	} state;							// current state
	stack<editor_state_t> undoStates;	// undo states
	stack<editor_state_t> redoStates;	// redos states

	// to handle focus - register every text editor in this vector
	static vector<ofxInterfaceTextEditor*> allEditors;
	static ofxInterfaceTextEditor* focusedEditor;

	// can override to change the drawing code (currently using ofxNanoVG)
	virtual void renderToFbo(ofFbo& fbo);
	virtual void drawTextEditor();


	void allocateFbo(ofFbo& fbo);
	caret_t toCaret(ofVec2f p, ofRectangle& _view);
	ofVec2f toNode(const caret_t& caret, ofRectangle& _view);
	ofVec2f toNode(int line, int chr, ofRectangle& _view);
	size_t toTextPos(const caret_t& c);
	caret_t toCaret(size_t textPos);
	void onTouchDown(TouchEvent& event);
	void onTouchMove(TouchEvent& event);
	void onTouchUp(TouchEvent& event);
	void clearSelection();
	float getLineNumberWidth();
	void bringViewToCaret();		// make sure that the current caret is in the view
	void limitView();				// make sure the view is legal
	void pushUndoState();
	void popUndoState();
	void pushRedoState();
	void popRedoState();
	bool fireEvent(ofEvent<EventArgs>& event);
	void clipboardCopy(const string& content);
	string clipboardPaste();
};

#endif /* ofxInterfaceTextEditor_h */
