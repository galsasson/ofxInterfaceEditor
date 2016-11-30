#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
	bDebug = false;
	ofSetFrameRate(60);

	// setup the scene
	TouchManager::one().setup(&scene);			// TouchManager should know about the scene
	scene.setSize(ofGetWidth(), ofGetHeight());	// set scene size to window size

	// configure the editor	(look at ofxInterfaceEditor constructor for the full list of config values)
	Json::Value config = Json::objectValue;
	config["width"] = 40;
	config["lines"] = 10;
	config["font-size"] = 16*2;
	config["border-corner"] = 4*2;
	editor.setConfig(config);

	// set editor position at 50x50
	editor.setPosition(50, 50);
	// add the text editor to the scene
	scene.addChild(&editor);

	// create save/load buttons
	BitmapTextButton* tb = new BitmapTextButton();
	tb->setup("load");
	tb->setPosition(50, editor.getY()+editor.getHeight()+4);
	ofAddListener(tb->eventTouchDown, this, &ofApp::onLoadClicked);
	scene.addChild(tb);
	tb = new BitmapTextButton();
	tb->setup("save");
	tb->setPosition(100, editor.getY()+editor.getHeight()+4);
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
	editor.loadFromFile("example.txt");
	editor.setTitle("example.txt");
}

//--------------------------------------------------------------
void ofApp::onSaveClicked(ofxInterface::TouchEvent &event)
{
	editor.saveToFile("example.txt");
	editor.setTitle("example.txt saved!");
	ofLogNotice("ofApp") << "Saved to example.txt";
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
	editor.keyPressed(key);
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){
	editor.keyReleased(key);
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
	editor.vscroll(scrollY);
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
