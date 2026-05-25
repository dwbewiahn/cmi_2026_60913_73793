#pragma once

#include "ofMain.h"

class MediaFrame {
public:
    std::string filename;
    std::string filepath;
    ofImage thumbnail;
    bool loaded = false;

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
};
