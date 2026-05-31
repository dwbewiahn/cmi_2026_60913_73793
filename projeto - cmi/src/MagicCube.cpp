#include "MagicCube.h"
#include "FeatureExtractor.h"
#ifdef _MSC_VER
#include <intrin.h>
#pragma intrinsic(__popcnt)
#endif
#include <algorithm>
#include <numeric>
#include <cmath>

namespace {
// Each face is assigned a different perception. The user faces FRONT initially
// and sees the Light face.
const FacePerceptionConfig kFaceConfigs[FACE_COUNT] = {
    { FACE_FRONT,  PERCEPTION_LIGHT,    "Light"    },
    { FACE_BACK,   PERCEPTION_KINSHIP,  "Kinship"  },
    { FACE_LEFT,   PERCEPTION_CONTRAST, "Contrast" },
    { FACE_RIGHT,  PERCEPTION_HUE,      "Hue"      },
    { FACE_TOP,    PERCEPTION_DETAIL,   "Detail"   },
    { FACE_BOTTOM, PERCEPTION_TEXTURE,  "Texture"  },
};
} // namespace

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
    sortedAssigned = false;
    loadingStatus = "Preparing...";

    cubieSize = cubeSize / 3.f;
    buildCubies();
}

void MagicCube::loadVideos(const std::string& videosDir) {
    ofLogNotice() << "MagicCube: scanning videos " << videosDir;
    ofDirectory dir(videosDir);
    dir.allowExt("mp4"); dir.allowExt("MP4");
    dir.allowExt("mov"); dir.allowExt("MOV");
    dir.allowExt("m4v"); dir.allowExt("M4V");

    int n = 0;
    try { dir.listDir(); dir.sort(); n = dir.size(); }
    catch (const std::exception& e) { ofLogError() << "videos dir: " << e.what(); n = 0; }
    ofLogNotice() << "Found " << n << " videos in " << videosDir;

    videos.clear();
    videos.resize(n); // resized once -> elements stay put (ofVideoPlayer is stable)

    bool dirty = false;
    for (int i = 0; i < n; i++) {
        std::string path  = dir.getPath(i);
        std::string fname = ofFilePath::getFileName(path);

        videos[i].loadVideo(path); // for playback

        const FeatureVector* cached = featureStore.get(fname);
        if (cached && cached->isVideo && cached->valid) {
            videos[i].features = *cached; // reuse cached motion/rhythm
        } else {
            FeatureVector fv = FeatureExtractor::computeFromVideoPath(path);
            if (fv.valid) {
                featureStore.put(fv);
                videos[i].features = fv;
                dirty = true;
            }
        }
    }
    if (dirty) featureStore.save(featuresXmlPath);
}

void MagicCube::buildCubies() {
    cubies.clear();
    cubies.reserve(27);

    // Initially build with no photos. Photos get assigned in
    // assignPhotosBySorting() once features are available.
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            for (int z = -1; z <= 1; z++) {
                Cubie c;
                c.logicalPos = glm::ivec3(x, y, z);
                c.homePos    = glm::ivec3(x, y, z);
                cubies.push_back(c);
            }
        }
    }
}

void MagicCube::update() {
    float dt = ofGetLastFrameTime();

    if (isLoading) processOneLoadingStep();

    if (rotating) {
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

    // ---------------------------------------------------------
    particles.update(dt);  // always update so existing particles fade out

    // Effects play only on a face that is currently in the correct sorted
    // order. A scrambled face is silent; the moment it clicks back into place
    // (e.g. you undo a move) the effects return automatically.
    bool faceSorted = isFaceSorted(activeFace);
    bool emit = effectsEnabled && sortedAssigned && !rotating && faceSorted;
    if (emit) {
        // Burst when the active face changes, or when it just snapped back
        // into the correct order after having been scrambled.
        if (activeFace != lastActiveFace || !lastFaceSorted) {
            emitFaceBurst(activeFace, 22);
        }
        lastActiveFace = activeFace;

        // Drift particles spawn at a random point along the active face's
        // perimeter, slightly in front of the face — so they trail "around"
        // the face rather than across the photos.
        driftTimer += dt;
        if (driftTimer >= 0.05f) {
            driftTimer = 0.f;
            int side = (int)ofRandom(4);
            float t = ofRandom(1.f);
            float hs = cubeSize / 2.f;
            glm::vec3 facePos;
            switch (side) {
                case 0: facePos = glm::vec3(-hs + t * cubeSize, -hs, 4.f); break;
                case 1: facePos = glm::vec3(+hs, -hs + t * cubeSize, 4.f); break;
                case 2: facePos = glm::vec3(-hs + t * cubeSize, +hs, 4.f); break;
                default: facePos = glm::vec3(-hs, -hs + t * cubeSize, 4.f); break;
            }
            glm::vec4 wp = faceLocalTransform(activeFace, cubeSize) *
                           glm::vec4(facePos, 1.f);
            Perception p = getFacePerception(activeFace);
            particles.emitDrift(glm::vec3(wp), faceNormalLocal(activeFace),
                                perceptionColor(p));
        }
    } else {
        // Reset the change tracker so when sorting comes back we don't burst
        // for whatever face the camera happens to be looking at.
        lastActiveFace = activeFace;
    }
    lastFaceSorted = faceSorted;

    // Advance only the 3 videos shown in immersive mode.
    if (insideMode) {
        for (int i : videoOrder) videos[i].update();
    }
}

void MagicCube::processOneLoadingStep() {
    if (loadingIndex >= loadingTotal) {
        if (cacheDirty) {
            featureStore.save(featuresXmlPath);
            cacheDirty = false;
        }
        isLoading = false;
        loadingStatus = "Sorting...";
        assignPhotosBySorting();
        loadingStatus = "Ready";
        return;
    }

    int i = loadingIndex;
    const std::string& path = pendingPaths[i];
    std::string fname = ofFilePath::getFileName(path);

    media[i].load(path, 384);

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
            featureStore.save(featuresXmlPath);
        } else {
            ofLogError() << "FeatureExtractor returned invalid for " << fname;
        }
    }

    loadingIndex++;
}

void MagicCube::draw() {
    // Immersive mode: hide the whole cube and just show the video triptych on a
    // flat, cube-coloured world (the background is set by ofApp) so it really
    // feels like being inside the cube rather than looking at one of its faces.
    if (insideMode) {
        drawInsideVideos();
        return;
    }

    glm::mat4 sliceR = rotating ? sliceMatrix(rotAxis, rotCurrentAngle) : glm::mat4(1.f);
    for (auto& c : cubies) {
        bool inSlice = rotating && isInSlice(c.logicalPos, rotAxis, rotSlice);
        c.draw(cubieSize, inSlice ? sliceR : glm::mat4(1.f));
    }

    bool showStaticEffects = effectsEnabled && sortedAssigned && isFaceSorted(activeFace);
    if (showStaticEffects) {
        drawActiveFaceSortPath();
        drawActiveFaceGlow();
    }
    // Always draw the particles themselves so the last burst can finish fading
    // after the cube is scrambled or effects are toggled off.
    if (effectsEnabled) particles.draw();
}

void MagicCube::startSliceRotation(int axis, int slice, float degrees) {
    if (rotating) return;
    rotating = true;
    rotAxis = axis;
    rotSlice = slice;
    rotCurrentAngle = 0.f;
    rotTargetAngle = degrees;
}

void MagicCube::resetToSolved() {
    if (rotating) return;
    buildCubies();
    assignPhotosBySorting();
}

void MagicCube::commitSliceRotation() {
    glm::mat4 R = sliceMatrix(rotAxis, rotTargetAngle);
    for (auto& c : cubies) {
        if (!isInSlice(c.logicalPos, rotAxis, rotSlice)) continue;
        c.logicalPos = rotateInt(c.logicalPos, rotAxis, rotTargetAngle);
        c.orientation = R * c.orientation;
    }
    // A slice rotation just shuffled photos away from their feature-sorted
    // positions. The grouping no longer matches the active face's perception,
    // so all the effects should go silent until the user presses R.
    isScrambled = true;
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

// ---- sort photos by perception and assign to faces -----------------

void MagicCube::assignPhotosBySorting() {
    if (media.empty()) return;

    // Wipe any existing photo assignments first.
    for (auto& c : cubies) {
        for (int f = 0; f < FACE_COUNT; f++) c.photos[f] = nullptr;
    }

    for (const auto& cfg : kFaceConfigs) {
        std::vector<int> sorted = sortedIndicesFor(cfg.perception);
        int n = std::min(9, (int)sorted.size());
        for (int cell = 0; cell < n; cell++) {
            int col = cell % 3;
            int row = cell / 3;
            glm::ivec3 pos = cellToCubiePos(cfg.face, col, row);
            Cubie* c = findCubie(pos);
            if (c) {
                c->photos[cfg.face] = &media[sorted[cell]];
            }
        }
    }

    sortedAssigned = true;
    isScrambled = false;
    lastActiveFace = activeFace; // suppress a spurious face-change burst

    // Grouping happened — burst particles on every face to signal it.
    if (effectsEnabled) {
        for (const auto& cfg : kFaceConfigs) emitFaceBurst(cfg.face, 16);
    }
}

std::vector<int> MagicCube::sortedIndicesFor(Perception p) const {
    std::vector<int> idx(media.size());
    std::iota(idx.begin(), idx.end(), 0);

    if (p == PERCEPTION_KINSHIP) {
        // Pick the photo with the smallest average ORB distance to all others
        // (the most "central" photo) as anchor, then sort the rest by distance
        // to it. Anchor goes in cell 0; closest neighbors fill the rest.
        if (idx.empty()) return idx;
        int anchor = 0;
        float bestAvg = std::numeric_limits<float>::infinity();
        for (int i = 0; i < (int)media.size(); i++) {
            float sum = 0.f; int cnt = 0;
            for (int j = 0; j < (int)media.size(); j++) {
                if (i == j) continue;
                sum += orbDistance(media[i].features, media[j].features);
                cnt++;
            }
            float avg = cnt > 0 ? sum / cnt : 0.f;
            if (avg < bestAvg) { bestAvg = avg; anchor = i; }
        }
        std::sort(idx.begin(), idx.end(), [&](int a, int b) {
            // Irreflexivity: comp(x, x) must be false.
            if (a == b) return false;
            if (a == anchor) return true;
            if (b == anchor) return false;
            return orbDistance(media[anchor].features, media[a].features) <
                   orbDistance(media[anchor].features, media[b].features);
        });
        return idx;
    }

    auto key = [&](int i) -> float {
        const FeatureVector& f = media[i].features;
        switch (p) {
            case PERCEPTION_LIGHT:    return f.meanLum;
            case PERCEPTION_CONTRAST: return f.varLum;
            case PERCEPTION_HUE:      return f.varHue;
            case PERCEPTION_DETAIL:   return f.edgeDensity;
            case PERCEPTION_TEXTURE:  return f.textureVar;
            default: return 0.f;
        }
    };
    std::sort(idx.begin(), idx.end(), [&](int a, int b) { return key(a) < key(b); });
    return idx;
}

// Map (face, col, row) → world-space cubie logical position.
// Derived from each face's transform (see Cubie.h::faceLocalTransform).
// col / row are 0..2, where row 0 is the visual top of the face when looking
// at it from outside the cube.
glm::ivec3 MagicCube::cellToCubiePos(FaceType f, int col, int row) {
    switch (f) {
        case FACE_FRONT:  return glm::ivec3(col - 1, row - 1, +1);
        case FACE_BACK:   return glm::ivec3(1 - col, row - 1, -1);
        case FACE_RIGHT:  return glm::ivec3(+1, row - 1, 1 - col);
        case FACE_LEFT:   return glm::ivec3(-1, row - 1, col - 1);
        case FACE_TOP:    return glm::ivec3(col - 1, -1, row - 1);
        case FACE_BOTTOM: return glm::ivec3(col - 1, +1, 1 - row);
        default:          return glm::ivec3(0, 0, 0);
    }
}

Cubie* MagicCube::findCubie(const glm::ivec3& pos) {
    for (auto& c : cubies) if (c.logicalPos == pos) return &c;
    return nullptr;
}

// Hamming-based average min distance between two ORB descriptor sets.
// Result is in [0, 256] (lower = more similar).
float MagicCube::orbDistance(const FeatureVector& a, const FeatureVector& b) const {
    if (a.orbDescriptors.empty() || b.orbDescriptors.empty()) return 256.f;
    const int dsize = 32;
    int nA = a.orbNumKeypoints;
    int nB = b.orbNumKeypoints;
    if (nA <= 0 || nB <= 0) return 256.f;

    float total = 0.f;
    for (int i = 0; i < nA; i++) {
        const uint8_t* da = &a.orbDescriptors[i * dsize];
        int bestDist = 257;
        for (int j = 0; j < nB; j++) {
            const uint8_t* db = &b.orbDescriptors[j * dsize];
            int dist = 0;
            for (int k = 0; k < dsize; k++) {
                #ifdef _MSC_VER
                    dist += __popcnt((unsigned)(da[k] ^ db[k]));
                #else
                    dist += __builtin_popcount((unsigned)(da[k] ^ db[k]));
                #endif  
            }
            if (dist < bestDist) bestDist = dist;
        }
        total += bestDist;
    }
    return total / nA;
}

namespace {
// A cubie is "un-rotated" when its accumulated orientation is (near) identity.
bool isIdentityOrient(const glm::mat4& m) {
    const glm::mat4 I(1.f);
    for (int c = 0; c < 4; ++c)
        for (int r = 0; r < 4; ++r)
            if (std::fabs(m[c][r] - I[c][r]) > 1e-3f) return false;
    return true;
}
} // namespace

bool MagicCube::isFaceSorted(FaceType f) {
    // Face f is correctly ordered when each of its 9 cells is occupied by the
    // cubie that belongs there (homePos == that cell) and is not rotated.
    for (int cell = 0; cell < 9; cell++) {
        glm::ivec3 pos = cellToCubiePos(f, cell % 3, cell / 3);
        Cubie* c = findCubie(pos);
        if (!c) return false;
        if (c->homePos != pos) return false;
        if (!isIdentityOrient(c->orientation)) return false;
    }
    return true;
}

FaceType MagicCube::getActiveFace(const glm::quat& cubeRotation) const {
    // Each face's outward normal in cube-local coords (Y is down in OF default).
    static const glm::vec3 baseNormals[FACE_COUNT] = {
        glm::vec3(0,  0, +1), // FRONT
        glm::vec3(0,  0, -1), // BACK
        glm::vec3(-1, 0,  0), // LEFT
        glm::vec3(+1, 0,  0), // RIGHT
        glm::vec3(0, -1,  0), // TOP
        glm::vec3(0, +1,  0), // BOTTOM
    };
    // After ofMultMatrix(curRot) is applied, the camera looks at faces whose
    // (rotated) normals point most strongly toward +Z (out of the screen).
    const glm::vec3 viewerDir(0, 0, +1);
    int best = 0;
    float bestDot = -2.f;
    for (int i = 0; i < FACE_COUNT; i++) {
        glm::vec3 n = cubeRotation * baseNormals[i];
        float d = glm::dot(n, viewerDir);
        if (d > bestDot) { bestDot = d; best = i; }
    }
    return (FaceType)best;
}

const char* MagicCube::getPerceptionLabel(Perception p) {
    switch (p) {
        case PERCEPTION_LIGHT:    return "Light (mean luminance)";
        case PERCEPTION_CONTRAST: return "Contrast (luminance variance)";
        case PERCEPTION_HUE:      return "Hue (color variance)";
        case PERCEPTION_DETAIL:   return "Detail (edge density)";
        case PERCEPTION_TEXTURE:  return "Texture (local roughness)";
        case PERCEPTION_KINSHIP:  return "Kinship (ORB similarity)";
        default:                  return "?";
    }
}

Perception MagicCube::getFacePerception(FaceType f) const {
    for (const auto& cfg : kFaceConfigs) {
        if (cfg.face == f) return cfg.perception;
    }
    return PERCEPTION_LIGHT;
}

glm::vec3 MagicCube::faceNormalLocal(FaceType f) {
    switch (f) {
        case FACE_FRONT:  return glm::vec3(0,  0, +1);
        case FACE_BACK:   return glm::vec3(0,  0, -1);
        case FACE_RIGHT:  return glm::vec3(+1, 0,  0);
        case FACE_LEFT:   return glm::vec3(-1, 0,  0);
        case FACE_TOP:    return glm::vec3(0, -1,  0);
        case FACE_BOTTOM: return glm::vec3(0, +1,  0);
        default:          return glm::vec3(0, 0, 1);
    }
}

ofFloatColor MagicCube::perceptionColor(Perception p) {
    switch (p) {
        case PERCEPTION_LIGHT:    return ofFloatColor(1.00f, 0.85f, 0.40f);
        case PERCEPTION_CONTRAST: return ofFloatColor(0.95f, 0.95f, 1.00f);
        case PERCEPTION_HUE:      return ofFloatColor(0.85f, 0.35f, 1.00f);
        case PERCEPTION_DETAIL:   return ofFloatColor(0.45f, 1.00f, 0.65f);
        case PERCEPTION_TEXTURE:  return ofFloatColor(0.95f, 0.70f, 0.45f);
        case PERCEPTION_KINSHIP:  return ofFloatColor(0.40f, 0.90f, 1.00f);
        default:                  return ofFloatColor(1, 1, 1);
    }
}

// World-space center of one cell on a face — used to spawn particles at the
// right position. Uses the face's transform, evaluated on the face-local
// 2D grid position of the cell.
glm::vec3 MagicCube::faceCellWorldCenter(FaceType f, int col, int row) const {
    float step = cubeSize / 3.f;
    float fx = -cubeSize / 2.f + col * step + step / 2.f;
    float fy = -cubeSize / 2.f + row * step + step / 2.f;
    glm::vec4 local(fx, fy, 0.f, 1.f);
    glm::mat4 m = faceLocalTransform(f, cubeSize);
    glm::vec4 w = m * local;
    return glm::vec3(w);
}

void MagicCube::emitFaceBurst(FaceType f, int particlesPerCell) {
    Perception p = getFacePerception(f);
    ofFloatColor color = perceptionColor(p);
    glm::vec3 outward = faceNormalLocal(f);
    // Spawn slightly OUTSIDE the face so particles never cover the photos —
    // they explode outward from there.
    float outOffset = cubeSize * 0.12f;
    for (int cell = 0; cell < 9; cell++) {
        int col = cell % 3;
        int row = cell / 3;
        glm::vec3 wp = faceCellWorldCenter(f, col, row) + outward * outOffset;
        particles.emitDirected(wp, outward, particlesPerCell, color,
                               160.f, 0.35f, 1.1f, 7.0f);
    }
}

void MagicCube::drawActiveFaceGlow() {
    Perception p = getFacePerception(activeFace);
    ofFloatColor c = perceptionColor(p);
    float pulse = 0.5f + 0.5f * std::sin(ofGetElapsedTimef() * 2.4f);

    ofPushStyle();
    ofPushMatrix();
    ofMultMatrix(faceLocalTransform(activeFace, cubeSize));

    // Thick pulsing outline just in front of the photo layer.
    float z = 2.f;
    float s = cubeSize / 2.f;
    ofSetLineWidth(5.f);
    ofNoFill();
    ofSetColor(c.r * 255, c.g * 255, c.b * 255, (int)(120 + 90 * pulse));
    ofPushMatrix();
    ofTranslate(0, 0, z);
    ofDrawRectangle(-s, -s, s * 2, s * 2);
    ofPopMatrix();

    // Soft inner halo — a slightly smaller, fainter rect.
    ofSetLineWidth(2.f);
    ofSetColor(c.r * 255, c.g * 255, c.b * 255, (int)(60 + 40 * pulse));
    ofPushMatrix();
    ofTranslate(0, 0, z + 0.5f);
    ofDrawRectangle(-s * 0.96f, -s * 0.96f, s * 1.92f, s * 1.92f);
    ofPopMatrix();

    ofFill();
    ofPopMatrix();
    ofPopStyle();
}

void MagicCube::drawActiveFaceSortPath() {
    Perception p = getFacePerception(activeFace);
    ofFloatColor c = perceptionColor(p);
    float step = cubeSize / 3.f;
    float baseAlpha = 70.f;
    float pulseAlpha = 35.f * (0.5f + 0.5f * std::sin(ofGetElapsedTimef() * 1.8f));

    ofPushStyle();
    ofPushMatrix();
    ofMultMatrix(faceLocalTransform(activeFace, cubeSize));

    // Draw faint line connecting cells 0..8 (sort order: top-left to bottom-right).
    ofSetLineWidth(2.f);
    ofSetColor(c.r * 255, c.g * 255, c.b * 255, (int)(baseAlpha + pulseAlpha));

    glm::vec3 prev(0);
    for (int cell = 0; cell < 9; cell++) {
        int col = cell % 3;
        int row = cell / 3;
        float fx = -cubeSize / 2.f + col * step + step / 2.f;
        float fy = -cubeSize / 2.f + row * step + step / 2.f;
        glm::vec3 cur(fx, fy, 1.5f);
        if (cell > 0) ofDrawLine(prev, cur);
        prev = cur;
    }

    // Small dots at each cell center to mark the sort sequence.
    ofSetColor(c.r * 255, c.g * 255, c.b * 255, (int)(140 + pulseAlpha));
    for (int cell = 0; cell < 9; cell++) {
        int col = cell % 3;
        int row = cell / 3;
        float fx = -cubeSize / 2.f + col * step + step / 2.f;
        float fy = -cubeSize / 2.f + row * step + step / 2.f;
        ofDrawSphere(fx, fy, 1.5f, 3.f);
    }

    ofPopMatrix();
    ofPopStyle();


}
// Pegar foto na célula (col, row) da face ativa. Retorna nullptr se a célula estiver vazia
MediaFrame* MagicCube::getPhotoOnActiveFace(int col, int row) {
    glm::ivec3 pos = cellToCubiePos(activeFace, col, row);
    Cubie* c = findCubie(pos);
    if (c) {
        return c->photos[activeFace];
    }
    return nullptr;
}

// ---- Immersive video mode ---------------------------------------------------

void MagicCube::setInsideMode(bool on) {
    insideMode = on;
    if (on) {
        // Keep only the 3 most energetic videos, ordered most -> least energetic.
        std::vector<int> idx(videos.size());
        std::iota(idx.begin(), idx.end(), 0);
        std::sort(idx.begin(), idx.end(), [&](int a, int b) {
            return videos[a].features.motionEnergy > videos[b].features.motionEnergy;
        });
        if ((int)idx.size() > 3) idx.resize(3);
        videoOrder = idx;
        videoCenter = 0;                       // most energetic starts centred
        for (auto& v : videos) v.pause();
        for (int i : videoOrder) videos[i].play();
    } else {
        for (auto& v : videos) v.pause();
    }
}

void MagicCube::cycleVideo() {
    if (!insideMode || videoOrder.empty()) return;
    // Next video becomes the centre; the current centre slides to the side.
    videoCenter = (videoCenter + 1) % (int)videoOrder.size();
}

FaceType MagicCube::oppositeFace(FaceType f) {
    switch (f) {
        case FACE_FRONT:  return FACE_BACK;
        case FACE_BACK:   return FACE_FRONT;
        case FACE_LEFT:   return FACE_RIGHT;
        case FACE_RIGHT:  return FACE_LEFT;
        case FACE_TOP:    return FACE_BOTTOM;
        case FACE_BOTTOM: return FACE_TOP;
        default:          return FACE_BACK;
    }
}

// Videos grouped by motion energy: calm clips first, energetic clips last.
std::vector<int> MagicCube::videosSortedByMotion() const {
    std::vector<int> idx(videos.size());
    std::iota(idx.begin(), idx.end(), 0);
    std::sort(idx.begin(), idx.end(), [&](int a, int b) {
        return videos[a].features.motionEnergy < videos[b].features.motionEnergy;
    });
    return idx;
}

// Draw the videos on the interior back wall, in a row sorted by motion energy.
// Each tile pulses with its rhythm; its frame colour goes from calm (cyan) to
// energetic (magenta) with motion energy.
void MagicCube::drawInsideVideos() {
    int nv = (int)videoOrder.size();
    if (nv == 0) return;

    int c       = videoCenter % nv;
    int centerV = videoOrder[c];
    int leftV   = videoOrder[(c + nv - 1) % nv];
    int rightV  = videoOrder[(c + 1) % nv];

    // Triptych laid out in the camera-facing face frame (so it always faces us).
    ofPushStyle();
    ofPushMatrix();
    ofMultMatrix(faceLocalTransform(activeFace, cubeSize));
    ofDisableDepthTest(); // centre panel always reads on top of the side ones

    float baseH  = cubeSize * 0.72f;
    float spread = cubeSize * 0.55f;
    float angle  = 50.f;

    // Side panels first (dimmer); centre (active) last so it sits on top.
    if (nv >= 2) drawVideoPanel(leftV,  -spread, +cubeSize * 0.05f, +angle, baseH * 0.9f, 150);
    if (nv >= 3) drawVideoPanel(rightV, +spread, +cubeSize * 0.05f, -angle, baseH * 0.9f, 150);
    drawVideoPanel(centerV, 0.f, -cubeSize * 0.18f, 0.f, baseH, 255);

    ofEnableDepthTest();
    ofPopMatrix();
    ofPopStyle();
}

void MagicCube::drawVideoPanel(int videoIndex, float x, float z, float angleY,
                               float height, int brightness) {
    MediaFrame& v = videos[videoIndex];
    const FeatureVector& f = v.features;

    // Rhythm gives the panel a gentle pulse.
    float rhythm = ofClamp(f.videoRhythm * 16.f, 0.f, 1.f);
    float pulse  = 1.f + 0.06f * rhythm * std::sin(ofGetElapsedTimef() * 3.f);

    // Speed (motion energy) -> outline colour: green (slow) -> yellow -> red (fast).
    float motion = ofClamp(f.motionEnergy * 12.f, 0.f, 1.f);
    ofColor outline;
    if (motion < 0.5f) outline.set((int)ofLerp(0, 255, motion / 0.5f), 255, 0);
    else               outline.set(255, (int)ofLerp(255, 0, (motion - 0.5f) / 0.5f), 0);

    float ar = v.aspect(); if (ar <= 0.01f) ar = 0.56f;
    float h  = height * pulse;
    float w  = h * ar;

    ofPushMatrix();
    ofTranslate(x, 0.f, z);
    ofRotateYDeg(angleY);

    ofSetColor(brightness);
    v.draw(-w * 0.5f, -h * 0.5f, w, h);

    // Outline: colour by speed, thicker the faster it is.
    ofNoFill();
    ofSetLineWidth(3.f + 5.f * motion);
    ofSetColor(outline, 235);
    ofDrawRectangle(-w * 0.5f, -h * 0.5f, w, h);
    ofFill();

    ofPopMatrix();
}