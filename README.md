# The Magic Cube

The Magic Cube is a media artifact that reimagines the exploration and experience of visual content. It is a three-dimensional artifact where each of its six faces reveals photos and videos organized through a unique perceptual lens. The entire media collection is reorganized in real-time as the cube rotates.

## 1. Core Concept: Six Faces, Six Realities

The cube transforms based on the user's perspective, with each combination of rotation representing a different perceptual organization:

* **Light Face**: Organized by mean luminance, where images cascade from shadow to sunshine. Bright images are designed to float higher within the layout.
* **Contrast Face**: Sorted by luminance variance, transitioning from soft gradients to sharp contrasts. High-variance media appears larger and more prominent.
* **Hue Face**: Sorted by hue variance, grouping pictures by dominant colors.
* **Detail Face**: Arranged by edge density, moving from simple surfaces to intricate details. Detailed images are grouped more tightly.
* **Texture Face**: Grouped by texture similarity, from glass-like smoothness to rough grain.
* **Energy Face**: Ordered by motion energy specifically for video content, ranging from still meditation to kinetic chaos. High-motion videos may pulse, ripple, or cause the face to tremble.
* **Kinship Face**: Clustered by visual similarity using ORB keypoints to find photos of the same places or subjects.

## 2. Interaction & Gestures 
The system utilizes intuitive, touchless gestures detected via webcam or standard keyboard input (WASD)

* **Swipe Left/Right/Up/Down**: Rotates the cube to reveal different faces and reorganization schemes.
* **Strong Shake**: Triggers a dramatic reorganization where media flies to new positions.
* **Forward Reach**: Dives inside the cube to explore content from a new perspective.
* **Face Tracking**: The cube subtly tilts to follow the user's position, enhancing depth perception.

## 3. Technical Architecture 

The system is composed of four primary subsystems:

* **Feature Extractor**: Analyzes media to compute seven visual features, storing metadata in XML format.
* **Cube Manager**: Manages 3D geometry, face layouts, rotation states, and animations[.
* **Gesture Detector**: Processes webcam input for motion and gesture recognition.
* **Immersion Renderer**: Handles dynamic lighting, fog, particle trails, and spatial audio.

### Technical Foundation 
* **OpenCV**: Used for feature extraction (luminance, variance, edge detection, ORB).
* **Processing / OpenFrameworks**: Utilized for 3D rendering and camera integration.
* **XML**: Ensures consistent metadata storage across analysis and display.

## 4. Class Structure.

* `MagicCube`: Main controller for state and rotation.
* `CubeFace`: Represents individual faces and organization rules.
* `MediaFrame`: Base class for 3D-positioned media.
* `FeatureVector`: Encapsulates all seven visual features.
* `GestureDetector`: Processes webcam frames for user input.
* `SimilarityEngine`: Computes distance metrics for the Kinship face.
* `XMLMetadataStore`: Handles reading/writing of feature data.

## Authors
* Nasha Bagasse (60913)
* Douglas (73793)
