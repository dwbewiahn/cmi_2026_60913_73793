#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup() {
    ofLogToConsole();
    ofSetFrameRate(60);
    ofSetCircleResolution(120);
    ofBackgroundGradient(ofColor(10), ofColor(50));
    ofEnableAntiAliasing();
    ofEnableSmoothing();
    // Low-poly spheres so the particle system stays cheap.
    ofSetSphereResolution(6);

    cube.setup(ofToDataPath("photos", true));
    
    // Inicializa a webcam com 320x240 píxeis de resolução
    grabber.setup(320, 240);

    // Tenta carregar o ficheiro de inteligência de rostos
    faceCascadeLoaded = faceCascade.load(ofToDataPath("haarcascade_frontalface_default.xml", true));

}

//--------------------------------------------------------------
void ofApp::update() {
    grabber.update();

    static glm::quat targetRot = curRot;
    static bool isAnimating = false;
    static float cooldownTimer = 0.0f;
    static bool firstRun = true;

    static float baselineX = 160.0f;
    static float baselineY = 120.0f;

    if (firstRun) {
        targetRot = curRot;
        firstRun = false;
    }

    if (!isAnimating && ofGetMousePressed() == false) {
        targetRot = curRot;
    }

    float deltaTime = ofClamp(ofGetLastFrameTime(), 1.f / 5000.f, 1.f / 5.f);

    if (cooldownTimer > 0.0f) {
        cooldownTimer -= deltaTime;
    }

    faceDetectedNow = false;

    //  Só processa a câmara se o controlo por face estiver ligado (faceTrackingOn == true)
    if (faceTrackingOn && faceCascadeLoaded && grabber.isFrameNew()) {
        ofPixels& px = grabber.getPixels();

        cv::Mat rgb(px.getHeight(), px.getWidth(), CV_8UC3, px.getData());
        cv::Mat gray;
        cv::cvtColor(rgb, gray, cv::COLOR_RGB2GRAY);

        std::vector<cv::Rect> faces;
        faceCascade.detectMultiScale(gray, faces, 1.1, 4, 0, cv::Size(40, 40));

        if (!faces.empty()) {
            cv::Rect bestFace = faces[0];
            for (const auto& f : faces) {
                if (f.area() > bestFace.area()) bestFace = f;
            }

            if (bestFace.width > 40) {
                faceDetectedNow = true;
                trackedFaceRect = bestFace;

                float faceCenterX = bestFace.x + bestFace.width / 2.0f;
                float faceCenterY = bestFace.y + bestFace.height / 2.0f;

                float rawOffsetX = (faceCenterX - baselineX);
                float rawOffsetY = (faceCenterY - baselineY);

                if (std::abs(rawOffsetX) < 40.0f && std::abs(rawOffsetY) < 40.0f) {
                    baselineX = ofLerp(baselineX, faceCenterX, 0.05f);
                    baselineY = ofLerp(baselineY, faceCenterY, 0.05f);
                }

                float diffX = faceCenterX - baselineX;
                float diffY = faceCenterY - baselineY;

                float thresholdX = 18.0f;
                float thresholdY = 22.0f;

                if (!isAnimating && cooldownTimer <= 0.0f) {

                    if (std::abs(diffX) > thresholdX && std::abs(diffX) * 1.3f > std::abs(diffY)) {
                        if (diffX < -thresholdX) {
                            targetRot = glm::angleAxis(ofDegToRad(-90.f), glm::vec3(0, 1, 0)) * curRot;
                            isAnimating = true;
                            cooldownTimer = 1.0f;
                        }
                        else if (diffX > thresholdX) {
                            targetRot = glm::angleAxis(ofDegToRad(90.f), glm::vec3(0, 1, 0)) * curRot;
                            isAnimating = true;
                            cooldownTimer = 1.0f;
                        }
                    }
                    else if (std::abs(diffY) > thresholdY) {
                        if (diffY < -thresholdY) {
                            targetRot = glm::angleAxis(ofDegToRad(90.f), glm::vec3(-1, 0, 0)) * curRot;
                            isAnimating = true;
                            cooldownTimer = 1.0f;
                        }
                        else if (diffY > thresholdY) {
                            targetRot = glm::angleAxis(ofDegToRad(-90.f), glm::vec3(-1, 0, 0)) * curRot;
                            isAnimating = true;
                            cooldownTimer = 1.0f;
                        }
                    }
                }
            }
        }
    }

    if (isAnimating) {
        curRot = glm::slerp(curRot, targetRot, 0.18f);
        if (std::abs(glm::dot(curRot, targetRot)) > 0.999f) {
            curRot = targetRot;
            isAnimating = false;
            xspeed = 0.0f;
            yspeed = 0.0f;
        }
    }
    else {
        xspeed /= 1.f + deltaTime;
        yspeed /= 1.f + deltaTime;
        glm::quat yRot = glm::angleAxis(ofDegToRad(xspeed), glm::vec3(0, 1, 0));
        glm::quat xRot = glm::angleAxis(ofDegToRad(yspeed), glm::vec3(-1, 0, 0));
        curRot = xRot * yRot * curRot;
    }

    cube.activeFace = cube.getActiveFace(curRot);
    cube.update();
}

//--------------------------------------------------------------
void ofApp::draw() {
    ofBackgroundGradient(ofColor(10), ofColor(50));

    glm::vec3 center(ofGetWidth()/2.f, ofGetHeight()/2.f, 0.f);

    ofEnableDepthTest();
    ofPushMatrix();
    ofTranslate(center);
    ofScale(cameraZoom); // Aplica o fator de zoom
    ofMultMatrix(glm::mat4(curRot));

    cube.draw();

    ofPopMatrix();
    ofDisableDepthTest();

    ofSetColor(230);

    ofPushMatrix();
    ofScale(2.0f, 2.0f);

    ofDrawBitmapString("Drag      : rotate whole cube", 20, 20);
    ofDrawBitmapString("Arrows    : middle slices", 20, 40);
    ofDrawBitmapString("Q / E     : top / bottom layer", 20, 60);
    ofDrawBitmapString("Z / X     : left / right column", 20, 80);
    ofDrawBitmapString("R         : reset (re-sort photos by face)", 20, 100);
    ofDrawBitmapString("F         : toggle effects", 20, 120);
    ofDrawBitmapString("Scroll    : zoom cube", 20, 140);
    string statusFace = faceTrackingOn ? "ON" : "OFF";
    ofDrawBitmapString("T         : toggle face tracking [" + statusFace + "]", 20, 160);

    // Coordenada Y alterada para 180 para corrigir o erro de sobreposição de texto
    ofDrawBitmapString("Photos: " + ofToString(cube.getMediaCount()) +
        "   Particles: " + ofToString(cube.particles.activeCount()) +
        "   FPS: " + ofToString(ofGetFrameRate(), 1), 20, 180);

    ofPopMatrix();

    // Active face indicator — top center.
    if (cube.sortedAssigned) {
        FaceType active = cube.getActiveFace(curRot);
        Perception p = cube.getFacePerception(active);
        std::string label = cube.isScrambled
            ? std::string("SCRAMBLED  —  press R to re-sort")
            : std::string("ACTIVE FACE: ") + MagicCube::getPerceptionLabel(p);
        float tw = label.length() * 8.f;
        float bx = ofGetWidth() / 2.f - tw / 2.f - 12;
        float by = 8;
        ofSetColor(0, 0, 0, 180);
        ofDrawRectangle(bx, by, tw + 24, 26);
        if (cube.isScrambled) {
            ofSetColor(255, 120, 90); // warm warning border
        } else {
            ofSetColor(120, 200, 255);
        }
        ofNoFill(); ofSetLineWidth(1);
        ofDrawRectangle(bx, by, tw + 24, 26);
        ofFill();
        ofSetColor(cube.isScrambled ? ofColor(220, 160, 140) : ofColor(255));
        ofDrawBitmapString(label, bx + 12, by + 17);
    }

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

    // Desenha a câmara no canto inferior direito (largura 160, altura 120)
    ofSetColor(255);
    grabber.draw(ofGetWidth() - 180, ofGetHeight() - 140, 160, 120);

    // Desenha a caixa verde de diagnóstico se o OpenCV estiver a detetar um rosto válido
    if (faceDetectedNow) {
        ofPushStyle();
        ofNoFill();
        ofSetColor(0, 255, 0); // Cor verde ativa
        ofSetLineWidth(2.f);

        // Converte as coordenadas do frame da câmara para a posição correspondente no ecrã
        float renderX = (ofGetWidth() - 180) + (trackedFaceRect.x * 0.5f);
        float renderY = (ofGetHeight() - 140) + (trackedFaceRect.y * 0.5f);
        float renderW = trackedFaceRect.width * 0.5f;
        float renderH = trackedFaceRect.height * 0.5f;

        ofDrawRectangle(renderX, renderY, renderW, renderH);
        ofPopStyle();
    }
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

        // Reset cube to solved state (un-scrambles slice rotations + re-sorts)
    case 'r': case 'R': cube.resetToSolved(); break;

        // Toggle Phase 4 effects (particles, glow, sort path).
    case 'f': case 'F': cube.effectsEnabled = !cube.effectsEnabled;
        if (!cube.effectsEnabled) cube.particles.clear();
        break;

        // CASO ADICIONADO: Inverte o estado do rastreio facial com a tecla T
    case 't': case 'T': faceTrackingOn = !faceTrackingOn; break;

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

void ofApp::mouseScrolled(ofMouseEventArgs& mouse) {
    // Incrementa ou decrementa o zoom com base na direção do scroll
    cameraZoom += mouse.scrollY * 0.05f;

    // Restringe o zoom entre 0.3x e 3.0x para não quebrar a cena
    cameraZoom = ofClamp(cameraZoom, 0.3f, 3.0f);
}