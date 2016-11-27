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

	void loadConfig(const Json::Value& config);
	void update(float dt);
	void draw();

	void setText(const string& text);
	string getText();
	void loadTextFile(const string& file);
	void keyPressed(int key);
	void keyReleased(int key);
	void addChar(char ch);

private:
	Json::Value config;
	ofFbo lastRender;
	vector<string> textLines;
	bool bDirty;
	int fboPad;
	ofxNanoVG::Font* font;
	ofRectangle view;
	ofRectangle targetView;		// for animated scroll
	bool bShiftPressed;

	struct caret_t {
		int line;
		int chr;
	} caret;
	float caretBlink;

	struct configcache_t {
		float width;
		int lines;
		float borderWidth;
		float fontSize;
		ofVec2f pad;
		bool bLineNumbers;
		float lineNumbersWidth;
		ofColor fontColor;
		ofColor bgColor;
		ofColor borderColor;
		ofColor selectionColor;
		ofVec2f letterSize;
		bool bSpecialEnter;
	} cache;

	struct selection_t {
		caret_t begin;
		caret_t end;
		bool active;
	} selection;

	void renderToFbo(ofFbo& fbo);
	void allocateFbo(ofFbo& fbo);
	caret_t toCaret(ofVec2f p);
	ofVec2f toNode(const caret_t& caret);
	ofVec2f toNode(int line, int chr);
	void onTouchDown(TouchEvent& event);
	void onTouchMove(TouchEvent& event);
	void onTouchUp(TouchEvent& event);
	void clearSelection();
	float getLineNumberWidth();
};

#endif /* ofxInterfaceEditor_h */
