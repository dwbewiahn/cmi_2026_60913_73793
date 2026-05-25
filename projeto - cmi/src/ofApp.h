#pragma once

#include "ofMain.h"
#include "MagicCube.h"

class ofApp : public ofBaseApp {
public:
    void setup();
    void update();
    void draw();

    void keyPressed(int key);
    void keyReleased(int key);
    void mouseMoved(int x, int y);
    void mouseDragged(int x, int y, int button);
    void mousePressed(int x, int y, int button);
    void mouseReleased(int x, int y, int button);
    void mouseEntered(int x, int y);
    void mouseExited(int x, int y);
    void windowResized(int w, int h);
    void dragEvent(ofDragInfo dragInfo);
    void gotMessage(ofMessage msg);

    MagicCube cube;

    glm::quat curRot;
    glm::vec2 lastMouse;
    float dampen = 0.4f;
    float xspeed = 0.f;
    float yspeed = 0.f;

    bool inspectorOn = false;
    int inspectorIdx = 0;

    void drawInspector();
};
