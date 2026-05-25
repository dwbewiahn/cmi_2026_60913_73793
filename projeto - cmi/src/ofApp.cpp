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

    if (cube.isLoading) {
        // Bottom-of-screen progress bar with status text.
        float barW = ofGetWidth() - 80;
        float barH = 18;
        float barX = 40;
        float barY = ofGetHeight() - 60;
        float p = cube.getLoadingProgress();

        ofSetColor(0, 0, 0, 180);
        ofDrawRectangle(0, barY - 36, ofGetWidth(), 96);

        ofSetColor(60);
        ofNoFill(); ofSetLineWidth(1);
        ofDrawRectangle(barX, barY, barW, barH);
        ofFill();
        ofSetColor(120, 200, 255);
        ofDrawRectangle(barX, barY, barW * p, barH);

        ofSetColor(230);
        ofDrawBitmapString(cube.loadingStatus, barX, barY - 8);
        ofDrawBitmapString(ofToString(int(p * 100)) + "%", barX + barW + 10, barY + 13);
    } else {
        ofSetColor(180);
        ofDrawBitmapString("I: feature inspector   [ / ]: prev/next photo", 20, ofGetHeight() - 20);
    }

    if (inspectorOn) drawInspector();
}

void ofApp::drawInspector() {
    if (cube.media.empty()) return;
    inspectorIdx = ((inspectorIdx % (int)cube.media.size()) + (int)cube.media.size()) % (int)cube.media.size();
    const MediaFrame& m = cube.media[inspectorIdx];
    const FeatureVector& f = m.features;

    float panelW = 360;
    float panelH = 320;
    float panelX = ofGetWidth() - panelW - 20;
    float panelY = 20;

    ofSetColor(0, 0, 0, 220);
    ofDrawRectangle(panelX, panelY, panelW, panelH);
    ofSetColor(120, 200, 255);
    ofNoFill(); ofSetLineWidth(1);
    ofDrawRectangle(panelX, panelY, panelW, panelH);
    ofFill();

    // Thumbnail
    if (m.loaded && m.thumbnail.isAllocated()) {
        float thumbMax = 140.f;
        float iw = m.thumbnail.getWidth();
        float ih = m.thumbnail.getHeight();
        float s = thumbMax / std::max(iw, ih);
        ofSetColor(255);
        m.thumbnail.draw(panelX + 12, panelY + 36, iw * s, ih * s);
    }

    ofSetColor(230);
    ofDrawBitmapString("Feature Inspector  [" + ofToString(inspectorIdx + 1) +
                       "/" + ofToString((int)cube.media.size()) + "]",
                       panelX + 12, panelY + 22);

    auto bar = [&](float x, float y, float w, float h, float v01, const std::string& label, float raw = -1.f) {
        ofSetColor(50);
        ofDrawRectangle(x, y, w, h);
        ofSetColor(120, 200, 255);
        ofDrawRectangle(x, y, w * ofClamp(v01, 0.f, 1.f), h);
        ofSetColor(230);
        std::string txt = label + ": " + ofToString(raw >= 0 ? raw : v01, 4);
        ofDrawBitmapString(txt, x, y - 4);
    };

    float tx = panelX + 165;
    float ty = panelY + 50;
    float tw = panelW - (tx - panelX) - 20;

    ofSetColor(230);
    ofDrawBitmapString(f.filename, panelX + 12, panelY + panelH - 90);

    if (!f.valid) {
        ofSetColor(255, 120, 120);
        ofDrawBitmapString("(features not loaded yet)", tx, ty);
    } else {
        bar(tx, ty +  10, tw, 10, f.meanLum,                 "mean luminance");
        bar(tx, ty +  40, tw, 10, f.varLum * 4.f,            "luminance var", f.varLum);
        bar(tx, ty +  70, tw, 10, f.varHue * 4.f,            "hue variance",  f.varHue);
        bar(tx, ty + 100, tw, 10, f.edgeDensity * 5.f,       "edge density",  f.edgeDensity);
        bar(tx, ty + 130, tw, 10, f.textureVar * 4.f,        "texture (RMS)", f.textureVar);

        ofSetColor(230);
        ofDrawBitmapString("ORB keypoints: " + ofToString(f.orbNumKeypoints) +
                           "   desc bytes: " + ofToString((int)f.orbDescriptors.size()),
                           panelX + 12, panelY + panelH - 60);
        ofDrawBitmapString("XML: bin/data/features.xml", panelX + 12, panelY + panelH - 40);
        ofDrawBitmapString("[ / ] cycle    I toggle", panelX + 12, panelY + panelH - 20);
    }
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

        // Feature inspector
        case 'i': case 'I': inspectorOn = !inspectorOn; break;
        case '[': if (!cube.media.empty()) inspectorIdx--; break;
        case ']': if (!cube.media.empty()) inspectorIdx++; break;
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
