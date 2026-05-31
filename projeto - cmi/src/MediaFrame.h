#pragma once

#include "ofMain.h"
#include "FeatureVector.h"

class MediaFrame {
public:
    std::string filename;
    std::string filepath;
    ofImage thumbnail;
    bool loaded = false;
    bool isVideo = false;
    ofVideoPlayer video;          // only used when isVideo == true
    FeatureVector features;

    void load(const std::string& path, int maxDim = 512) {
        filepath = path;
        filename = ofFilePath::getFileName(path);
        ofImage src;
        if (!src.load(path)) {
            ofLogError() << "Failed to load " << path;
            return;
        }
        float w = src.getWidth();
        float h = src.getHeight();
        float scale = std::min(1.f, (float)maxDim / std::max(w, h));
        int tw = (int)(w * scale);
        int th = (int)(h * scale);
        thumbnail.allocate(tw, th, OF_IMAGE_COLOR);
        thumbnail.getPixels() = src.getPixels();
        thumbnail.resize(tw, th);
        thumbnail.update();
        loaded = true;
    }

    // Load a video clip for playback (features are extracted separately).
    void loadVideo(const std::string& path) {
        filepath = path;
        filename = ofFilePath::getFileName(path);
        isVideo = true;
        video.setLoopState(OF_LOOP_NORMAL);
        if (!video.load(path)) {
            ofLogError() << "Failed to load video " << path;
            return;
        }
        video.setVolume(0.f);
        loaded = true;
    }

    void update() { if (isVideo && loaded) video.update(); }
    void play()   { if (isVideo && loaded) { video.setLoopState(OF_LOOP_NORMAL); video.play(); } }
    void pause()  { if (isVideo && loaded) video.setPaused(true); }

    float aspect() const {
        if (isVideo && loaded && video.getHeight() > 0)
            return video.getWidth() / video.getHeight();
        if (thumbnail.isAllocated() && thumbnail.getHeight() > 0)
            return thumbnail.getWidth() / thumbnail.getHeight();
        return 1.f;
    }

    // Draw the current frame (video) or thumbnail into the given rect.
    void draw(float x, float y, float w, float h) const {
        if (isVideo && loaded && video.isInitialized()) {
            const_cast<ofVideoPlayer&>(video).draw(x, y, w, h);
        } else if (thumbnail.isAllocated()) {
            thumbnail.draw(x, y, w, h);
        }
    }
};
