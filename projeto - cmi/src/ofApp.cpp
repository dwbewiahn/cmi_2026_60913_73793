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

    // Carrega os vídeos (e extrai motion energy + rhythm; fica em cache no XML).
    cube.loadVideos(ofToDataPath("videos", true));

    // --- DETEÇÃO DE CARA TEMPORARIAMENTE DESATIVADA ---------------------------
    // // Inicializa a webcam com 320x240 píxeis de resolução
    // grabber.setup(320, 240);
    // // Tenta carregar o ficheiro de inteligência de rostos
    // faceCascadeLoaded = faceCascade.load(ofToDataPath("haarcascade_frontalface_default.xml", true));
    // --------------------------------------------------------------------------
}

//--------------------------------------------------------------
void ofApp::update() {

    // SE HOUVER UMA FOTO EXPANDIDA, CONGELA O CUBO
    if (selectedPhoto != nullptr) return;

    // --- DETEÇÃO DE CARA TEMPORARIAMENTE DESATIVADA ---
    // grabber.update();

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

    /* --- DETEÇÃO DE CARA TEMPORARIAMENTE DESATIVADA ---------------------------
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
    --------------------------------------------------------------------------- */

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
    // Inside the cube: a flat background the colour of the cube, so the videos
    // feel like they're floating inside it (not sitting on one of its faces).
    if (cube.insideMode) ofBackground(24, 24, 28);
    else                 ofBackgroundGradient(ofColor(10), ofColor(50));

    glm::vec3 center(ofGetWidth()/2.f, ofGetHeight()/2.f, 0.f);

    ofEnableDepthTest();
    ofPushMatrix();
    ofTranslate(center);
    ofScale(cameraZoom); 
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
    string statusInside = cube.insideMode ? "ON" : "OFF";
    ofDrawBitmapString("V         : enter / exit cube (videos) [" + statusInside + "]", 20, 160);
    ofDrawBitmapString("D         : next video (when inside)", 20, 175);


    ofDrawBitmapString("Photos: " + ofToString(cube.getMediaCount()) +
        "   Videos: " + ofToString(cube.getVideoCount()) +
        "   Particles: " + ofToString(cube.particles.activeCount()) +
        "   FPS: " + ofToString(ofGetFrameRate(), 1), 20, 195);

    ofPopMatrix();

    // Active face indicator — top center.
    if (cube.sortedAssigned) {
        FaceType active = cube.getActiveFace(curRot);
        Perception p = cube.getFacePerception(active);
        bool sorted = cube.isFaceSorted(active);
        std::string label = sorted
            ? std::string("ACTIVE FACE: ") + MagicCube::getPerceptionLabel(p)
            : std::string("SCRAMBLED face  —  rotate it back or press R");
        float tw = label.length() * 8.f;
        float bx = ofGetWidth() / 2.f - tw / 2.f - 12;
        float by = 8;
        ofSetColor(0, 0, 0, 180);
        ofDrawRectangle(bx, by, tw + 24, 26);
        if (!sorted) {
            ofSetColor(255, 120, 90); // warm warning border
        } else {
            ofSetColor(120, 200, 255);
        }
        ofNoFill(); ofSetLineWidth(1);
        ofDrawRectangle(bx, by, tw + 24, 26);
        ofFill();
        ofSetColor(!sorted ? ofColor(220, 160, 140) : ofColor(255));
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

    // Overlay do modo imersivo: os 3 vídeos (mais -> menos energético).
    // Posicionado em baixo à esquerda, mesmo por cima da linha "I: feature".
    if (cube.insideMode && !cube.videoOrder.empty()) {
        int nv = (int)cube.videoOrder.size();
        int c  = cube.videoCenter % nv;
        float bx = 20;
        float boxW = 540;
        float boxH = 30 + 18 * nv;
        float boxTop = ofGetHeight() - 34 - boxH; // base mesmo acima de "I: feature"
        ofSetColor(0, 0, 0, 180);
        ofDrawRectangle(bx - 10, boxTop, boxW, boxH);
        ofSetColor(120, 200, 255);
        ofDrawBitmapString("INSIDE THE CUBE  -  3 videos most -> least energetic   (D: next)", bx, boxTop + 16);
        ofSetColor(235);
        for (int k = 0; k < nv; k++) {
            int vi = cube.videoOrder[k];
            const FeatureVector& f = cube.videos[vi].features;
            std::string mark = (k == c) ? "  <-- CENTER (active)" : "";
            std::string line = ofToString(k + 1) + ". " + cube.videos[vi].filename +
                "   motion=" + ofToString(f.motionEnergy, 3) +
                "   rhythm=" + ofToString(f.videoRhythm, 3) + mark;
            ofDrawBitmapString(line, bx, boxTop + 34 + k * 18);
        }
    }

    // --- CÂMARA / CAIXA DE DETEÇÃO TEMPORARIAMENTE DESATIVADAS ---------------
    // ofSetColor(255);
    // grabber.draw(ofGetWidth() - 180, ofGetHeight() - 140, 160, 120);
    // if (faceDetectedNow) {
    //     ofPushStyle();
    //     ofNoFill();
    //     ofSetColor(0, 255, 0);
    //     ofSetLineWidth(2.f);
    //     float renderX = (ofGetWidth() - 180) + (trackedFaceRect.x * 0.5f);
    //     float renderY = (ofGetHeight() - 140) + (trackedFaceRect.y * 0.5f);
    //     float renderW = trackedFaceRect.width * 0.5f;
    //     float renderH = trackedFaceRect.height * 0.5f;
    //     ofDrawRectangle(renderX, renderY, renderW, renderH);
    //     ofPopStyle();
    // }
    // ------------------------------------------------------------------------

    // --- DESENHO DA FOTO EXPANDIDA EM ECRÃ INTEIRO ---
    if (selectedPhoto != nullptr && selectedPhoto->loaded && selectedPhoto->thumbnail.isAllocated()) {
        ofPushStyle();
        ofDisableDepthTest(); // Garante que renderiza à frente do cubo 3D

        // Desenha um fundo escuro semi-transparente para focar a atenção na imagem
        ofSetColor(0, 0, 0, 230);
        ofDrawRectangle(0, 0, ofGetWidth(), ofGetHeight());

        // Calcula a escala ideal para a imagem ocupar até 80% do menor eixo do ecrã
        float maxScreenDim = std::min(ofGetWidth(), ofGetHeight()) * 0.8f;
        float iw = selectedPhoto->thumbnail.getWidth();
        float ih = selectedPhoto->thumbnail.getHeight();
        float s = maxScreenDim / std::max(iw, ih);

        float drawW = iw * s;
        float drawH = ih * s;
        float drawX = (ofGetWidth() - drawW) / 2.f;
        float drawY = (ofGetHeight() - drawH) / 2.f;

        // Desenha uma moldura branca fina à volta da imagem
        ofNoFill();
        ofSetColor(255);
        ofSetLineWidth(2.f);
        ofDrawRectangle(drawX - 2, drawY - 2, drawW + 4, drawH + 4);

        // Desenha a imagem centralizada
        ofFill();
        ofSetColor(255);
        selectedPhoto->thumbnail.draw(drawX, drawY, drawW, drawH);

        // Desenha uma instrução simples de fecho na parte inferior
        ofSetColor(200);
        string closeTxt = "Click anywhere to close - [" + selectedPhoto->features.filename + "]";
        ofDrawBitmapString(closeTxt, ofGetWidth() / 2.f - (closeTxt.length() * 4.f), drawY + drawH + 30);

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

        // DETEÇÃO DE CARA TEMPORARIAMENTE DESATIVADA
    // case 't': case 'T': faceTrackingOn = !faceTrackingOn; break;

        // Entrar / sair do cubo (experiência imersiva com os vídeos).
    case 'v': case 'V':
        cube.setInsideMode(!cube.insideMode);
        cameraZoom = cube.insideMode ? 1.5f : 1.0f; // aproxima a câmara para parecer que estamos dentro
        break;

        // Dentro do cubo: passar para o vídeo seguinte (carrossel).
    case 'd': case 'D':
        cube.cycleVideo();
        break;

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

    
    if (selectedPhoto != nullptr) {
        selectedPhoto = nullptr;
        return;
    }

    // Converte o clique do ecrã para o espaço local relativo ao centro do cubo
    float centerBx = ofGetWidth() / 2.f;
    float centerBy = ofGetHeight() / 2.f;
    float localX = (x - centerBx) / cameraZoom;
    float localY = (y - centerBy) / cameraZoom;

    float halfCube = cube.cubeSize / 2.f;

    // Verifica se o clique ocorreu dentro dos limites visuais do cubo
    if (localX >= -halfCube && localX <= halfCube && localY >= -halfCube && localY <= halfCube) {

        // Cria o ponto de clique 3D no plano frontal projetado para o utilizador
        glm::vec3 worldClick(localX, localY, halfCube);

        // Multiplica pelo inverso da rotação atual para neutralizar as rotações acumuladas
        glm::vec3 localClick = glm::inverse(curRot) * worldClick;

        // Lambda para converter a coordenada contínua para o índice lógico do bloco (-1, 0 ou 1)
        auto getLogicalCoord = [](float val, float halfSize) {
            float limit = halfSize / 3.f;
            if (val < -limit) return -1;
            if (val > limit) return 1;
            return 0;
            };

        int cx = getLogicalCoord(localClick.x, halfCube);
        int cy = getLogicalCoord(localClick.y, halfCube);
        int cz = getLogicalCoord(localClick.z, halfCube);
        glm::ivec3 targetCubiePos(cx, cy, cz);

        // Identifica qual é a face orientada para o ecrã e procura o bloco correspondente
        FaceType activeFace = cube.getActiveFace(curRot);
        for (auto& c : cube.cubies) {
            if (c.logicalPos == targetCubiePos) {
                selectedPhoto = c.photos[activeFace];
                break;
            }
        }
    }
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