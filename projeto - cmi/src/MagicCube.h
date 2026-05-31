#pragma once

#include "ofMain.h"
#include "MediaFrame.h"
#include "CubeFace.h"
#include "Cubie.h"
#include "FeatureStore.h"
#include "Particles.h"

enum Perception {
    PERCEPTION_LIGHT = 0,    // sorted by meanLum ascending — shadow to sunshine
    PERCEPTION_CONTRAST,     // sorted by varLum ascending — soft to dramatic
    PERCEPTION_HUE,          // sorted by varHue ascending — single-color to rainbow
    PERCEPTION_DETAIL,       // sorted by edgeDensity ascending — simple to complex
    PERCEPTION_TEXTURE,      // sorted by textureVar ascending — smooth to rough
    PERCEPTION_KINSHIP,      // clustered by ORB similarity to an anchor photo
    PERCEPTION_COUNT
};

struct FacePerceptionConfig {
    FaceType face;
    Perception perception;
    const char* label;
};

class MagicCube {
public:
    std::vector<MediaFrame> media;
    std::vector<MediaFrame> videos;   // clips shown inside the cube (immersive)
    std::vector<Cubie> cubies;

    // Immersive "inside the cube" mode: the front face is hidden so the camera
    // appears to be inside, and the 3 most energetic videos are shown as a
    // triptych (active video centred, the other two angled at the sides).
    bool insideMode = false;
    int  videoCenter = 0;          // which of the 3 videos is centred (carousel)
    std::vector<int> videoOrder;   // up to 3 video indices, most -> least energetic

    float cubeSize = 600.f;
    float cubieSize = 200.f;

    // Slice rotation
    bool rotating = false;
    int rotAxis = 1;
    int rotSlice = 0;
    float rotCurrentAngle = 0.f;
    float rotTargetAngle = 0.f;
    float rotSpeedDegPerSec = 270.f;

    // Async loading
    bool isLoading = false;
    int loadingIndex = 0;
    int loadingTotal = 0;
    std::string loadingStatus;
    bool cacheDirty = false;
    std::vector<std::string> pendingPaths;

    FeatureStore featureStore;
    std::string featuresXmlPath;
    bool sortedAssigned = false;

    // effects
    ParticleSystem particles;
    FaceType activeFace = FACE_FRONT;
    FaceType lastActiveFace = FACE_FRONT;
    float driftTimer = 0.f;
    bool effectsEnabled = true;
    bool isScrambled = false;     // true once any slice rotation has committed
    bool lastFaceSorted = true;   // tracks active-face sort state across frames

    void setup(const std::string& photosDir);
    void loadVideos(const std::string& videosDir); // load + extract video features
    void update();
    void draw();

    // Immersive video mode.
    void setInsideMode(bool on);
    void cycleVideo();            // advance the carousel to the next video (key D)
    int  getVideoCount() const { return (int)videos.size(); }

    int getMediaCount() const { return (int)media.size(); }
    float getLoadingProgress() const {
        return loadingTotal > 0 ? (float)loadingIndex / (float)loadingTotal : 1.f;
    }

    void startSliceRotation(int axis, int slice, float degrees);
    void resetToSolved();

    // Which face is most facing the camera, given current overall cube rotation.
    FaceType getActiveFace(const glm::quat& cubeRotation) const;

    // True if face f currently shows its photos in the correct sorted order
    // (its 9 cubies are back home with an un-rotated orientation). Used to
    // re-enable the effects automatically once a face clicks back into place.
    bool isFaceSorted(FaceType f);

    // Retorna a foto presente numa determinada célula (coluna/linha) da face ativa
    MediaFrame* getPhotoOnActiveFace(int col, int row);

    static const char* getPerceptionLabel(Perception p);
    Perception getFacePerception(FaceType f) const;

    // Cube-local outward normal of a face (before any rotation).
    static glm::vec3 faceNormalLocal(FaceType f);

    // Per-perception color (used by particles, glow, and sort-sequence lines).
    static ofFloatColor perceptionColor(Perception p);

private:
    void processOneLoadingStep();
    void buildCubies();
    void assignPhotosBySorting();
    std::vector<int> sortedIndicesFor(Perception p) const;
    static glm::ivec3 cellToCubiePos(FaceType f, int col, int row);
    Cubie* findCubie(const glm::ivec3& pos);
    float orbDistance(const FeatureVector& a, const FeatureVector& b) const;

    void emitFaceBurst(FaceType f, int particlesPerCell = 18);
    void drawActiveFaceGlow();
    void drawActiveFaceSortPath();
    glm::vec3 faceCellWorldCenter(FaceType f, int col, int row) const;

    void commitSliceRotation();
    bool isInSlice(const glm::ivec3& p, int axis, int slice) const;
    glm::mat4 sliceMatrix(int axis, float degrees) const;
    glm::ivec3 rotateInt(const glm::ivec3& p, int axis, float degrees) const;

    // Immersive video helpers.
    static FaceType oppositeFace(FaceType f);
    std::vector<int> videosSortedByMotion() const; // indices, calm -> energetic
    void drawInsideVideos();                        // triptych of the 3 videos
    void drawVideoPanel(int videoIndex, float x, float z, float angleY,
                        float height, int brightness);
};
