# The Magic Cube

The Magic Cube is an interactive visual medium that reimagines how a personal collection of photos is explored. The entire collection is mapped onto a three-dimensional Rubik-style $3\times3\times3$ cube where each of the six faces reorganizes the photographs according to a different computer-vision perceptual principle.

## 1. Core Concept: Six Faces, Six Perceptions

The photos are sorted automatically by their extracted features and assigned to the faces in ascending order (from the top-left to the bottom-right cell of each grid). 

* **Light Face (Front)**: Sorted by mean luminance, creating a visual cascade from shadow to sunshine.
* **Contrast Face (Left)**: Sorted by luminance variance, transitioning from soft gradients to sharp, dramatic contrasts.
* **Hue Face (Right)**: Sorted by hue variance, grouping images from single-color compositions to multi-color, rainbow-like ones.
* **Detail Face (Top)**: Arranged by edge density using a Canny filter, moving from simple, flat surfaces to intricate details.
* **Texture Face (Bottom)**: Grouped by local roughness computed via RMS of local variance, tracking from glass-like smoothness to rough grain.
* **Kinship Face (Back)**: Clustered by visual similarity using ORB keypoint binary descriptors matched via average minimum Hamming distance around the most "central" anchor photo.

## 2. Controls & Interaction 

The system supports multi-modal interaction combining manual desktop inputs with automated computer vision navigation:

* **Mouse Drag**: Rotates the whole cube structure. The rotation uses a quaternion-based system with inertia and damping to give the cube a physical, weighted feeling.
* **Mouse Scroll**: Controls the global camera viewport zoom. The scale factor is restricted between $0.3\times$ and $3.0\times$ via clamping to prevent mesh clipping.
* **Mouse Click (Photo Selection)**: Clicking directly on any photo on the active face locks the cube and expands the image into a high-resolution 2D full-screen preview with a dimmed background. It uses an inverse rotation matrix calculation (`glm::inverse(curRot)`) to reliably map the 2D cursor coordinates to the correct 3D cubie regardless of how many times the cube was rotated.
* **Rubik Slice Rotations**: Keyboard commands trigger internal slice animations:
  * *Arrow Keys*: Rotate the middle slices.
  * *Q / E*: Rotate the top and bottom horizontal layers.
  * *Z / X*: Rotate the left and right vertical columns.
* **System Hotkeys**:
  * **R**: Resets the cube layout and re-applies the perceptual sorting (solves the cube).
  * **F**: Toggles the Phase 4 reactive graphics layer (particle bursts, outline glow, and sorting path lines).
  * **T**: Toggles the face-tracking system on or off dynamically to save CPU cycles.
  * **I** and **[ / ]**: Opens the Feature Inspector panel to debug scalar outputs and ORB keypoint metrics.

### Face-Tracking Engine
Navigates the cube faces hands-free via real-time webcam input ($320\times240$). Using an OpenCV Haar Cascade classifier, the system tracks the user's face position. To guarantee stability, it implements a *Discrete Trigger State Machine*: when an intentional head tilt breaks past a set threshold ($\pm18$\,px horizontal or $\pm22$\,px vertical), a smooth 90-degree camera orbit is executed using spherical linear interpolation (`glm::slerp`) with a 1.0-second cooldown. Input drift caused by lighting or off-center seating is countered by an *Adaptive Baseline Calibration* that continuously updates the user's neutral posture center over time.

## 3. Technical Architecture 

The system relies on five cohesive structural layers:

* **Feature Extraction (`FeatureExtractor`)**: Responsible for single-pass processing of raw images into normalized scalar and binary visual feature vectors using OpenCV.
* **Metadata Storage (`FeatureStore`)**: Handles persisting and reading feature records via openFrameworks' XML serialization, acting as a local cache database to achieve near-instantaneous load times on subsequent runs.
* **Cube Management (`MagicCube`)**: Coordinates the geometry configuration of the 27 cubies, monitors active face view orientation vectors, processes slice scrambling calculations, and orchestrates sorting triggers.
* **Rendering & Particle Effects (`Cubie`, `ParticleSystem`)**: Draws individual 3D blocks with mapping coordinates, pulsing halos, index path routes, and manages the lifecycle of additive-blended particle fields.
* **Application Framework (`ofApp`)**: Drives the main window execution loop, hardware capture streams, inverse matrix click arithmetic, full-screen overlay state switches, and the HUD layout.

### Technical Foundation 
* **OpenCV**: Core analysis framework used for spatial and descriptor feature metrics alongside Haar Cascade face tracking.
* **openFrameworks**: Handles OpenGL 3D graphics rendering, asset loading pipelines, windowing, and window events.
* **ofXml**: Structured XML system handling local cache parsing.

## Authors
* Nasha Bagasse (60913)
* Douglas (73793)
