#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup() {
    ofSetFrameRate(60);
    ofSetCircleResolution(120);
    ofBackgroundGradient(ofColor(10), ofColor(50));
    ofEnableAntiAliasing();
    ofEnableSmoothing();

    cube.setup(ofToDataPath("photos", true));
}

//--------------------------------------------------------------
void ofApp::update() {
    float deltaTime = ofClamp(ofGetLastFrameTime(), 1.f/5000.f, 1.f/5.f);
    xspeed /= 1.f + deltaTime;
    yspeed /= 1.f + deltaTime;

    glm::quat yRot = glm::angleAxis(ofDegToRad(xspeed), glm::vec3(0, 1, 0));
    glm::quat xRot = glm::angleAxis(ofDegToRad(yspeed), glm::vec3(-1, 0, 0));
    curRot = xRot * yRot * curRot;

    cube.update();
}

//--------------------------------------------------------------
void ofApp::draw() {
    ofBackgroundGradient(ofColor(10), ofColor(50));

    glm::vec3 center(ofGetWidth()/2.f, ofGetHeight()/2.f, 0.f);

    ofEnableDepthTest();
    ofPushMatrix();
    ofTranslate(center);
    ofMultMatrix(glm::mat4(curRot));

    cube.draw();

    ofPopMatrix();
    ofDisableDepthTest();

    ofSetColor(230);
    ofDrawBitmapString("Drag      : rotate whole cube", 20, 20);
    ofDrawBitmapString("Arrows    : middle slices", 20, 40);
    ofDrawBitmapString("Q / E     : top / bottom layer", 20, 60);
    ofDrawBitmapString("Z / X     : left / right column", 20, 80);
    ofDrawBitmapString("Photos: " + ofToString(cube.getMediaCount()) + "   FPS: " + ofToString(ofGetFrameRate(), 1), 20, 100);
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {
    // Rubik-style slice rotations.
    // Arrows  -> middle slices.
    // Q / E   -> top / bottom horizontal layers (around Y).
    // Z / X   -> left / right vertical columns (around X).
    switch (key) {
        // Middle slices
        case OF_KEY_RIGHT: cube.startSliceRotation(1, 0, +90.f); break;
        case OF_KEY_LEFT:  cube.startSliceRotation(1, 0, -90.f); break;
        case OF_KEY_UP:    cube.startSliceRotation(0, 0, +90.f); break;
        case OF_KEY_DOWN:  cube.startSliceRotation(0, 0, -90.f); break;

        // Outer horizontal layers (Y axis: -1 = top, +1 = bottom in OF Y-down).
        case 'q': case 'Q': cube.startSliceRotation(1, -1, +90.f); break;
        case 'e': case 'E': cube.startSliceRotation(1, +1, +90.f); break;

        // Outer vertical columns (X axis: -1 = left, +1 = right).
        case 'z': case 'Z': cube.startSliceRotation(0, -1, +90.f); break;
        case 'x': case 'X': cube.startSliceRotation(0, +1, +90.f); break;
        default: break;
    }
}
void ofApp::keyReleased(int key) {}
void ofApp::mouseMoved(int x, int y) {}

void ofApp::mouseDragged(int x, int y, int button) {
    glm::vec2 mouse(x, y);
    xspeed = ofLerp(xspeed, (x - lastMouse.x) * dampen, 0.1f);
    yspeed = ofLerp(yspeed, (y - lastMouse.y) * dampen, 0.1f);
    lastMouse = mouse;
}

void ofApp::mousePressed(int x, int y, int button) {
    lastMouse = glm::vec2(x, y);
}

void ofApp::mouseReleased(int x, int y, int button) {}
void ofApp::mouseEntered(int x, int y) {}
void ofApp::mouseExited(int x, int y) {}
void ofApp::windowResized(int w, int h) {}
void ofApp::gotMessage(ofMessage msg) {}
void ofApp::dragEvent(ofDragInfo dragInfo) {}
