#pragma once

#include "ofMain.h"
#include "MagicCube.h"
#include <opencv2/opencv.hpp>

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

    float cameraZoom = 1.0f; // Controla a escala do cubo
    ofVideoGrabber grabber;   // Objeto responsável por gerir a webcam

    //  Rastreio Facial
    cv::CascadeClassifier faceCascade;
    bool faceCascadeLoaded = false;

    // Variáveis para desenhar a caixa de rastreio no ecrã
    cv::Rect trackedFaceRect;
    bool faceDetectedNow = false;
    bool faceTrackingOn = true;

    // Armazena a foto expandida (nullptr se nenhuma estiver aberta)
    MediaFrame* selectedPhoto = nullptr; 

    void drawInspector();

    // Captura o scroll do rato
    void mouseScrolled(ofMouseEventArgs& mouse); 
};

