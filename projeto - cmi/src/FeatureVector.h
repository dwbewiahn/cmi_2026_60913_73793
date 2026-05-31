#pragma once

#include <string>
#include <vector>
#include <cstdint>

// All scalar features are normalized to roughly [0, 1] so they're directly
// comparable across faces of the cube.
struct FeatureVector {
    std::string filename;
    float meanLum     = 0.f; // 0..1
    float varLum      = 0.f; // 0..1 (std/255, squared)
    float varHue      = 0.f; // 0..1 (std/180, squared)
    float edgeDensity = 0.f; // 0..1 (fraction of edge pixels)
    float textureVar  = 0.f; // 0..1 (RMS of local variance / 255)
    int orbNumKeypoints = 0;
    std::vector<uint8_t> orbDescriptors; // flat: orbNumKeypoints * 32 bytes

    // ----- Video-only features --------------------------------------------
    bool  isVideo      = false;
    float motionEnergy = 0.f; // 0..1  mean frame-to-frame difference (how much movement)
    float videoRhythm  = 0.f; // 0..1  variability of that motion over time (how bursty)

    bool valid = false;
};
