#pragma once

#include "ofMain.h"
#include "MediaFrame.h"
#include "CubeFace.h" // for FaceType enum

inline glm::mat4 faceLocalTransform(FaceType f, float size) {
    float h = size / 2.f;
    glm::mat4 m(1.f);
    switch (f) {
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
            m = glm::translate(glm::mat4(1.f), glm::vec3(0, -h, 0))
              * glm::rotate(glm::mat4(1.f), glm::half_pi<float>(), glm::vec3(1, 0, 0));
            break;
        case FACE_BOTTOM:
            m = glm::translate(glm::mat4(1.f), glm::vec3(0, h, 0))
              * glm::rotate(glm::mat4(1.f), -glm::half_pi<float>(), glm::vec3(1, 0, 0));
            break;
        default: break;
    }
    return m;
}

class Cubie {
public:
    glm::ivec3 logicalPos = glm::ivec3(0);
    glm::mat4 orientation = glm::mat4(1.f);
    MediaFrame* photos[FACE_COUNT] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};

    void draw(float cubieSize, const glm::mat4& extraTransform = glm::mat4(1.f)) const {
        ofPushMatrix();
        ofMultMatrix(extraTransform);

        glm::vec3 worldPos = glm::vec3(logicalPos) * cubieSize;
        ofTranslate(worldPos);
        ofMultMatrix(orientation);

        float h = cubieSize / 2.f;
        float pad = cubieSize * 0.06f;

        for (int fi = 0; fi < FACE_COUNT; fi++) {
            FaceType f = (FaceType)fi;
            ofPushMatrix();
            ofMultMatrix(faceLocalTransform(f, cubieSize));

            // Dark backing for the face (helps when looking at an interior side)
            ofSetColor(12);
            ofDrawRectangle(-h, -h, -0.5f, cubieSize, cubieSize);

            MediaFrame* mf = photos[f];
            if (mf && mf->loaded && mf->thumbnail.isAllocated()) {
                float iw = mf->thumbnail.getWidth();
                float ih = mf->thumbnail.getHeight();
                float availW = cubieSize - pad * 2.f;
                float availH = cubieSize - pad * 2.f;
                float imgAR = iw / ih;
                float cellAR = availW / availH;
                float drawW, drawH;
                if (imgAR > cellAR) { drawW = availW; drawH = availW / imgAR; }
                else                { drawH = availH; drawW = availH * imgAR; }
                ofSetColor(255);
                mf->thumbnail.draw(-drawW/2.f, -drawH/2.f, drawW, drawH);
            }

            // Black border between cells
            ofSetColor(0);
            ofNoFill();
            ofSetLineWidth(2);
            ofDrawRectangle(-h, -h, cubieSize, cubieSize);
            ofFill();

            ofPopMatrix();
        }

        ofPopMatrix();
    }
};
