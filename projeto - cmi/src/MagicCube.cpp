#include "MagicCube.h"

void MagicCube::setup(const std::string& photosDir) {
    ofDirectory dir(photosDir);
    dir.allowExt("jpg");
    dir.allowExt("jpeg");
    dir.allowExt("JPG");
    dir.allowExt("JPEG");
    dir.allowExt("png");
    dir.allowExt("PNG");
    dir.listDir();
    dir.sort();

    int n = dir.size();
    ofLogNotice() << "Found " << n << " photos in " << photosDir;
    media.resize(n);

    for (int i = 0; i < n; i++) {
        std::string path = dir.getPath(i);
        ofLogNotice() << "Loading [" << (i+1) << "/" << n << "] " << ofFilePath::getFileName(path);
        media[i].load(path, 384);
    }

    cubieSize = cubeSize / 3.f;
    cubies.clear();
    cubies.reserve(27);

    // Build 27 cubies. Each cubie's outward-facing local faces get a photo
    // cycled from the media library (54 outward face slots in total).
    int idx = 0;
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            for (int z = -1; z <= 1; z++) {
                Cubie c;
                c.logicalPos = glm::ivec3(x, y, z);
                if (!media.empty()) {
                    auto pick = [&]() -> MediaFrame* {
                        MediaFrame* m = &media[idx % (int)media.size()];
                        idx++;
                        return m;
                    };
                    if (z == +1) c.photos[FACE_FRONT]  = pick();
                    if (z == -1) c.photos[FACE_BACK]   = pick();
                    if (x == +1) c.photos[FACE_RIGHT]  = pick();
                    if (x == -1) c.photos[FACE_LEFT]   = pick();
                    if (y == -1) c.photos[FACE_TOP]    = pick();
                    if (y == +1) c.photos[FACE_BOTTOM] = pick();
                }
                cubies.push_back(c);
            }
        }
    }
}

void MagicCube::update() {
    if (!rotating) return;

    float dt = ofGetLastFrameTime();
    float step = rotSpeedDegPerSec * dt;
    float remaining = rotTargetAngle - rotCurrentAngle;

    if (std::fabs(remaining) <= step) {
        rotCurrentAngle = rotTargetAngle;
        commitSliceRotation();
        rotating = false;
        rotCurrentAngle = 0.f;
        rotTargetAngle = 0.f;
    } else {
        rotCurrentAngle += (remaining > 0 ? step : -step);
    }
}

void MagicCube::draw() {
    glm::mat4 sliceR = rotating ? sliceMatrix(rotAxis, rotCurrentAngle) : glm::mat4(1.f);

    for (auto& c : cubies) {
        bool inSlice = rotating && isInSlice(c.logicalPos, rotAxis, rotSlice);
        c.draw(cubieSize, inSlice ? sliceR : glm::mat4(1.f));
    }
}

void MagicCube::startSliceRotation(int axis, int slice, float degrees) {
    if (rotating) return;
    rotating = true;
    rotAxis = axis;
    rotSlice = slice;
    rotCurrentAngle = 0.f;
    rotTargetAngle = degrees;
}

void MagicCube::commitSliceRotation() {
    glm::mat4 R = sliceMatrix(rotAxis, rotTargetAngle);
    for (auto& c : cubies) {
        if (!isInSlice(c.logicalPos, rotAxis, rotSlice)) continue;
        c.logicalPos = rotateInt(c.logicalPos, rotAxis, rotTargetAngle);
        c.orientation = R * c.orientation;
    }
}

bool MagicCube::isInSlice(const glm::ivec3& p, int axis, int slice) const {
    return p[axis] == slice;
}

glm::mat4 MagicCube::sliceMatrix(int axis, float degrees) const {
    glm::vec3 a(0.f);
    a[axis] = 1.f;
    return glm::rotate(glm::mat4(1.f), ofDegToRad(degrees), a);
}

glm::ivec3 MagicCube::rotateInt(const glm::ivec3& p, int axis, float degrees) const {
    // Snap to nearest 90°
    int q = (int)std::round(degrees / 90.f);
    q = ((q % 4) + 4) % 4; // 0..3
    glm::ivec3 r = p;
    for (int i = 0; i < q; i++) {
        // +90° around given axis
        if (axis == 0) {        // X: (x, y, z) -> (x, -z, y)
            r = glm::ivec3(r.x, -r.z, r.y);
        } else if (axis == 1) { // Y: (x, y, z) -> (z, y, -x)
            r = glm::ivec3(r.z, r.y, -r.x);
        } else {                // Z: (x, y, z) -> (-y, x, z)
            r = glm::ivec3(-r.y, r.x, r.z);
        }
    }
    return r;
}
