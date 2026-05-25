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
};
