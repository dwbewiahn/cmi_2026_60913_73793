#include "MagicCube.h"
#include "FeatureExtractor.h"

void MagicCube::setup(const std::string& photosDir) {
    ofLogNotice() << "MagicCube: scanning " << photosDir;
    ofDirectory dir(photosDir);
    dir.allowExt("jpg");
    dir.allowExt("jpeg");
    dir.allowExt("JPG");
    dir.allowExt("JPEG");
    dir.allowExt("png");
    dir.allowExt("PNG");

    int n = 0;
    try {
        dir.listDir();
        dir.sort();
        n = dir.size();
    } catch (const std::exception& e) {
        ofLogError() << "MagicCube: failed to list photos dir: " << e.what();
        loadingStatus = std::string("ERROR listing ") + photosDir + ": " + e.what();
        n = 0;
    }
    ofLogNotice() << "Found " << n << " photos in " << photosDir;

    featuresXmlPath = ofToDataPath("features.xml", true);
    ofLogNotice() << "MagicCube: features XML path = " << featuresXmlPath;
    bool cacheExisted = featureStore.load(featuresXmlPath);
    ofLogNotice() << "MagicCube: cache " << (cacheExisted ? "loaded" : "not found / will be created");

    media.clear();
    media.resize(n);
    pendingPaths.clear();
    pendingPaths.reserve(n);
    for (int i = 0; i < n; i++) pendingPaths.push_back(dir.getPath(i));

    loadingTotal = n;
    loadingIndex = 0;
    isLoading = (n > 0);
    cacheDirty = false;
    loadingStatus = "Preparing...";

    cubieSize = cubeSize / 3.f;
    buildCubies();
}

void MagicCube::buildCubies() {
    cubies.clear();
    cubies.reserve(27);

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
    if (isLoading) {
        processOneLoadingStep();
    }

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

void MagicCube::processOneLoadingStep() {
    if (loadingIndex >= loadingTotal) {
        if (cacheDirty) {
            featureStore.save(featuresXmlPath);
            cacheDirty = false;
        }
        isLoading = false;
        loadingStatus = "Ready";
        return;
    }

    int i = loadingIndex;
    const std::string& path = pendingPaths[i];
    std::string fname = ofFilePath::getFileName(path);

    // Load thumbnail (always — it's needed for rendering).
    media[i].load(path, 384);

    // Features: load from cache, or compute + cache.
    if (featureStore.has(fname)) {
        media[i].features = *featureStore.get(fname);
        loadingStatus = "Cached features [" + ofToString(i + 1) + "/" +
                        ofToString(loadingTotal) + "] " + fname;
    } else {
        loadingStatus = "Extracting features [" + ofToString(i + 1) + "/" +
                        ofToString(loadingTotal) + "] " + fname;
        ofLogNotice() << loadingStatus;
        FeatureVector fv = FeatureExtractor::computeFromPath(path);
        if (fv.valid) {
            featureStore.put(fv);
            media[i].features = fv;
            cacheDirty = true;
            // Save incrementally so partial progress is preserved.
            featureStore.save(featuresXmlPath);
        } else {
            ofLogError() << "FeatureExtractor returned invalid for " << fname;
        }
    }

    loadingIndex++;
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
    int q = (int)std::round(degrees / 90.f);
    q = ((q % 4) + 4) % 4;
    glm::ivec3 r = p;
    for (int i = 0; i < q; i++) {
        if (axis == 0) r = glm::ivec3(r.x, -r.z, r.y);
        else if (axis == 1) r = glm::ivec3(r.z, r.y, -r.x);
        else r = glm::ivec3(-r.y, r.x, r.z);
    }
    return r;
}
