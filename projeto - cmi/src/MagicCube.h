#pragma once

#include "ofMain.h"
#include "MediaFrame.h"
#include "CubeFace.h"
#include "Cubie.h"
#include "FeatureStore.h"

class MagicCube {
public:
    std::vector<MediaFrame> media;
    std::vector<Cubie> cubies; // 27

    float cubeSize = 600.f;
    float cubieSize = 200.f;

    // Slice rotation animation
    bool rotating = false;
    int rotAxis = 1;
    int rotSlice = 0;
    float rotCurrentAngle = 0.f;
    float rotTargetAngle = 0.f;
    float rotSpeedDegPerSec = 270.f;

    // Async media loading + feature extraction
    bool isLoading = false;
    int loadingIndex = 0;     // next media slot to process
    int loadingTotal = 0;
    std::string loadingStatus;
    bool cacheDirty = false;
    std::vector<std::string> pendingPaths;

    FeatureStore featureStore;
    std::string featuresXmlPath;

    void setup(const std::string& photosDir);
    void update();
    void draw();

    int getMediaCount() const { return (int)media.size(); }
    float getLoadingProgress() const {
        return loadingTotal > 0 ? (float)loadingIndex / (float)loadingTotal : 1.f;
    }

    void startSliceRotation(int axis, int slice, float degrees);

private:
    void processOneLoadingStep();
    void buildCubies();

    void commitSliceRotation();
    bool isInSlice(const glm::ivec3& p, int axis, int slice) const;
    glm::mat4 sliceMatrix(int axis, float degrees) const;
    glm::ivec3 rotateInt(const glm::ivec3& p, int axis, float degrees) const;
};
