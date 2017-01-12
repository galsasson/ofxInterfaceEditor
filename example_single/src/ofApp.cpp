#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
	ofSetFrameRate(60);

	// setup the scene
	TouchManager::one().setup(&scene);			// TouchManager should know about the scene
	scene.setSize(ofGetWidth(), ofGetHeight());	// set scene size to window size

	// add the text editor to the scene
	scene.addChild(&editor);
	// set editor position at 50x50
	editor.setPosition(50, 50);

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
	ofSetColor(255);
	ofDrawBitmapString("Hit 'S' to change size and colors", 3, 11);
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
	if (key == 'S') {
		Json::Value conf;
		conf["width"] = int(ofRandom(10, 60));
		conf["lines"] = int(ofRandom(1,20));
		conf["background-color"][0] = ofRandom(1);
		conf["background-color"][1] = ofRandom(1);
		conf["background-color"][2] = ofRandom(1);
		conf["font-color"][0] = ofRandom(1);
		conf["font-color"][1] = ofRandom(1);
		conf["font-color"][2] = ofRandom(1);
		editor.setConfig(conf);
	}
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
