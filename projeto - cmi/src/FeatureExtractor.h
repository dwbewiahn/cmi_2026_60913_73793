#pragma once

#include "FeatureVector.h"
#include <string>

class FeatureExtractor {
public:
    // Compute all features for the given image path. Returns a FeatureVector
    // with valid=false if the image couldn't be loaded.
    static FeatureVector computeFromPath(const std::string& imagePath,
                                         int maxOrbKeypoints = 100,
                                         int maxDim = 512);

    // Compute video features (motion energy + rhythm) by sampling frames with
    // cv::VideoCapture. Returns valid=false if the video couldn't be opened.
    //  - motionEnergy: mean frame-to-frame difference (overall amount of motion)
    //  - videoRhythm : std-dev of that per-frame difference signal (how bursty)
    static FeatureVector computeFromVideoPath(const std::string& videoPath,
                                              int maxSamples = 64,
                                              int maxDim = 192);
};
