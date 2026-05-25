#pragma once

#include "ofMain.h"
#include "MediaFrame.h"
#include "CubeFace.h"
#include "Cubie.h"

class MagicCube {
public:
    std::vector<MediaFrame> media;
    std::vector<Cubie> cubies; // 27 of them

    float cubeSize = 600.f;
    float cubieSize = 200.f; // cubeSize / 3

    // Slice rotation animation state
    bool rotating = false;
    int rotAxis = 1;        // 0=X, 1=Y, 2=Z
    int rotSlice = 0;       // -1, 0, +1
    float rotCurrentAngle = 0.f;
    float rotTargetAngle = 0.f;
    float rotSpeedDegPerSec = 270.f;

    void setup(const std::string& photosDir);
    void update();
    void draw();

    int getMediaCount() const { return (int)media.size(); }

    // Start a slice rotation. Ignored if already rotating.
    // axis: 0=X (M slice), 1=Y (E slice), 2=Z (S slice)
    // slice: -1, 0, +1 (typically 0 for "middle")
    void startSliceRotation(int axis, int slice, float degrees);

private:
    void commitSliceRotation();
    bool isInSlice(const glm::ivec3& p, int axis, int slice) const;
    glm::mat4 sliceMatrix(int axis, float degrees) const;
    glm::ivec3 rotateInt(const glm::ivec3& p, int axis, float degrees) const;
};
