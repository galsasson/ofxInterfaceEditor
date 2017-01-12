#include "ofApp.h"

Json::Value ofApp::getEditorConfig1()
{
	Json::Value config = Json::objectValue;
	config["width"] =			40;
	config["lines"] =			10;
	config["font"] =			"Menlo-Regular.ttf";
	config["font-size"] =		22;
	config["border-color"] =	"#ffffff 100%";
	config["border-width"] =	20;
	config["border-corner"] =	0;
	config["pad"][0] =			20;
	config["pad"][1] =			0;
	config["background-color"] ="#222222 100%";
	config["title"] =			true;
	config["draggable"] =		true;
	config["line-numbers"] =	true;
	config["focus-color"] =		"#ddaa00 100%";
	config["focus-width"] =		2;
	return config;
}

Json::Value ofApp::getEditorConfig2()
{
	Json::Value config =		Json::objectValue;
	config["width"] =			40;
	config["lines"] =			10;
	config["font"] =			"Menlo-Regular.ttf";
	config["font-size"] =		22;
	config["border-color"] =	"#ffffff 100%";
	config["border-width"] =	0;
	config["border-corner"] =	0;
	config["pad"][0] =			4;
	config["pad"][1] =			0;
	config["background-color"] ="#222222 100%";
	config["title"] =			false;
	config["draggable"] =		false;
	config["focus-color"] =		"#ddaa00 100%";
	config["focus-width"] =		2;
	return config;
}

Json::Value ofApp::getEditorConfig3()
{
	Json::Value config =		Json::objectValue;
	config["width"] =			40;
	config["lines"] =			10;
	config["font-size"] =		30;
	config["border-color"] =	"#ffffff 100%";
	config["border-width"] =	10;
	config["border-corner"] =	10;
	config["pad"][0] =			4;
	config["pad"][1] =			0;
	config["background-color"] ="#222222 100%";
	config["title"] =			true;
	config["draggable"] =		true;
	config["line-numbers"] =	true;
	config["focus-color"] =		"#ddaa00 100%";
	config["focus-width"] =		2;
	return config;
}

Json::Value ofApp::getEditorConfig4()
{
	Json::Value config = Json::objectValue;
	config["width"] =			60;
	config["lines"] =			16;
	config["font"] =			"Menlo-Regular.ttf";
	config["font-size"] =		14;
	config["border-color"] =	"#ffffff 100%";
	config["border-width"] =	1;
	config["border-corner"] =	2;
	config["pad"][0] =			2;
	config["pad"][1] =			0;
	config["background-color"] ="#222222 100%";
	config["title"] =			true;
	config["draggable"] =		true;
	config["focus-color"] =		"#ddaa00 100%";
	config["focus-width"] =		2;
	return config;
}


//--------------------------------------------------------------
void ofApp::setup(){
	bDebug = false;
	ofSetFrameRate(60);

	// setup the scene
	TouchManager::one().setup(&scene);			// TouchManager should know about the scene
	scene.setSize(ofGetWidth(), ofGetHeight());	// set scene size to window size

	// add multiple editors to the scene
	scene.addChild(new ofxInterfaceTextEditor(getEditorConfig1()));
	scene.addChild(new ofxInterfaceTextEditor(getEditorConfig2()));
	scene.addChild(new ofxInterfaceTextEditor(getEditorConfig3()));
	scene.addChild(new ofxInterfaceTextEditor(getEditorConfig4()));

	ofxInterfaceTextEditor::getAllEditors()[0]->setPosition(10, 40);
	ofxInterfaceTextEditor::getAllEditors()[1]->setPosition(520, 80);
	ofxInterfaceTextEditor::getAllEditors()[2]->setPosition(0, 350);
	ofxInterfaceTextEditor::getAllEditors()[3]->setPosition(590, 360);


	// create save/load buttons
	BitmapTextButton* tb = new BitmapTextButton();
	tb->setup("load");
	tb->setPosition(10, 10);
	ofAddListener(tb->eventTouchDown, this, &ofApp::onLoadClicked);
	scene.addChild(tb);
	tb = new BitmapTextButton();
	tb->setup("save");
	tb->setPosition(60, 10);
	ofAddListener(tb->eventTouchDown, this, &ofApp::onSaveClicked);
	scene.addChild(tb);
}

//--------------------------------------------------------------
void ofApp::update(){
	// update scene
	scene.updateSubtreePostOrder(1.0/60);
}

//--------------------------------------------------------------
void ofApp::draw(){
	ofBackground(0);
	// render scene
	scene.render();
}


//--------------------------------------------------------------
void ofApp::onLoadClicked(ofxInterface::TouchEvent &event)
{
	ofLogNotice("ofApp") << "Loading from example.txt";
	if (ofxInterfaceTextEditor::getFocused()) {
		ofxInterfaceTextEditor::getFocused()->loadFromFile("example.txt");
		ofxInterfaceTextEditor::getFocused()->setTitle("example.txt");
	}
}

//--------------------------------------------------------------
void ofApp::onSaveClicked(ofxInterface::TouchEvent &event)
{
	if (ofxInterfaceTextEditor::getFocused()) {
		ofxInterfaceTextEditor::getFocused()->saveToFile("example.txt");
		ofxInterfaceTextEditor::getFocused()->setTitle("example.txt saved!");
	}
	ofLogNotice("ofApp") << "Saved to example.txt";
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
	if (ofxInterfaceTextEditor::getFocused()) {
		ofxInterfaceTextEditor::getFocused()->keyPressed(key);
	}
	else {
		ofLogNotice("ofApp") << "no text editor has focus";
	}
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){
	if (ofxInterfaceTextEditor::getFocused()) {
		ofxInterfaceTextEditor::getFocused()->keyReleased(key);
	}
	else {
		ofLogNotice("ofApp") << "no text editor has focus";
	}
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
	TouchManager::one().touchMove(button, ofVec2f(x, y));
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
	TouchManager::one().touchDown(button, ofVec2f(x, y));
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){
	TouchManager::one().touchUp(button, ofVec2f(x, y));
}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseScrolled(int x, int y, float scrollX, float scrollY)
{
	ofxInterfaceTextEditor::vscroll(x, y, scrollY);
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
