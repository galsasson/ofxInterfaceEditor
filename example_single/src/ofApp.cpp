#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
	bDebug = false;
	ofSetFrameRate(30);

	scene.setSize(ofGetWidth(), ofGetHeight());
	TouchManager::one().setup(&scene);

	editor.setPosition(50, 50);
	editor.loadTextFile("example.hpp");

	scene.addChild(&editor);

}

//--------------------------------------------------------------
void ofApp::update(){
	scene.updateSubtreePostOrder(1.0/60);
}

//--------------------------------------------------------------
void ofApp::draw(){
	ofBackground(0);

	scene.render();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
	if (key == 'R') {
		editor.loadTextFile("example.hpp");
	}
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
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
