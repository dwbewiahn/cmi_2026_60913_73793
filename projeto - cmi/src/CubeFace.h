#pragma once

#include "ofMain.h"
#include "MediaFrame.h"

enum FaceType {
    FACE_FRONT = 0,
    FACE_BACK,
    FACE_LEFT,
    FACE_RIGHT,
    FACE_TOP,
    FACE_BOTTOM,
    FACE_COUNT
};

class CubeFace {
public:
    FaceType type;
    std::string label;
    std::vector<MediaFrame*> frames;
    int rows = 6;
    int cols = 7;
    float faceSize = 600.f;
    glm::mat4 localTransform = glm::mat4(1.0f);

    void setup(FaceType t, float size, const std::string& lbl,
               const std::vector<MediaFrame*>& mediaPtrs) {
        type = t;
        faceSize = size;
        label = lbl;
        frames = mediaPtrs;
        computeTransform();
    }

    void computeTransform() {
        float h = faceSize / 2.f;
        glm::mat4 m(1.0f);
        switch (type) {
            case FACE_FRONT:
                m = glm::translate(glm::mat4(1.f), glm::vec3(0, 0, h));
                break;
            case FACE_BACK:
                m = glm::translate(glm::mat4(1.f), glm::vec3(0, 0, -h))
                  * glm::rotate(glm::mat4(1.f), glm::pi<float>(), glm::vec3(0, 1, 0));
                break;
            case FACE_RIGHT:
                m = glm::translate(glm::mat4(1.f), glm::vec3(h, 0, 0))
                  * glm::rotate(glm::mat4(1.f), glm::half_pi<float>(), glm::vec3(0, 1, 0));
                break;
            case FACE_LEFT:
                m = glm::translate(glm::mat4(1.f), glm::vec3(-h, 0, 0))
                  * glm::rotate(glm::mat4(1.f), -glm::half_pi<float>(), glm::vec3(0, 1, 0));
                break;
            case FACE_TOP:
                // In OF's default screen-coord 3D (Y down), the visible "top" of the cube
                // is at world Y = -h, with outward normal -Y. To get the +Z local normal
                // to point -Y we need +90° rotation around X (not -90°).
                m = glm::translate(glm::mat4(1.f), glm::vec3(0, -h, 0))
                  * glm::rotate(glm::mat4(1.f), glm::half_pi<float>(), glm::vec3(1, 0, 0));
                break;
            case FACE_BOTTOM:
                m = glm::translate(glm::mat4(1.f), glm::vec3(0, h, 0))
                  * glm::rotate(glm::mat4(1.f), -glm::half_pi<float>(), glm::vec3(1, 0, 0));
                break;
            default: break;
        }
        localTransform = m;
    }

    void draw() {
        ofPushMatrix();
        ofMultMatrix(localTransform);

        ofSetColor(20);
        ofDrawRectangle(-faceSize/2.f, -faceSize/2.f, -0.5f, faceSize, faceSize);

        float cellW = faceSize / cols;
        float cellH = faceSize / rows;
        float pad = std::min(cellW, cellH) * 0.08f;

        int n = std::min((int)frames.size(), rows * cols);
        for (int i = 0; i < n; i++) {
            int col = i % cols;
            int row = i / cols;
            float cx = -faceSize/2.f + col * cellW + cellW/2.f;
            float cy = -faceSize/2.f + row * cellH + cellH/2.f;

            float availW = cellW - pad * 2.f;
            float availH = cellH - pad * 2.f;

            MediaFrame* mf = frames[i];
            if (mf && mf->loaded && mf->thumbnail.isAllocated()) {
                float iw = mf->thumbnail.getWidth();
                float ih = mf->thumbnail.getHeight();
                float imgAR = iw / ih;
                float cellAR = availW / availH;
                float drawW, drawH;
                if (imgAR > cellAR) {
                    drawW = availW;
                    drawH = availW / imgAR;
                } else {
                    drawH = availH;
                    drawW = availH * imgAR;
                }
                ofSetColor(255);
                mf->thumbnail.draw(cx - drawW/2.f, cy - drawH/2.f, drawW, drawH);
            } else {
                ofSetColor(80);
                ofDrawRectangle(cx - availW/2.f, cy - availH/2.f, availW, availH);
            }
        }

        ofSetColor(200);
        ofNoFill();
        ofSetLineWidth(2);
        ofDrawRectangle(-faceSize/2.f, -faceSize/2.f, faceSize, faceSize);
        ofFill();

        ofPopMatrix();
    }
};
