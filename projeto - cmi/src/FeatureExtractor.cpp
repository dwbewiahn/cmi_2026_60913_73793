#include "FeatureExtractor.h"

#include "ofMain.h"
#include <opencv2/opencv.hpp>
#include <opencv2/videoio.hpp>
#include <cstring>
#include <cmath>

FeatureVector FeatureExtractor::computeFromPath(const std::string& imagePath,
                                                int maxOrbKeypoints,
                                                int maxDim) {
    FeatureVector fv;
    fv.filename = ofFilePath::getFileName(imagePath);

    // Use OF's image loader (FreeImage) since the bundled opencv.a does NOT
    // include libjpeg/libpng — cv::imread silently returns an empty Mat for
    // any JPEG/PNG. Loading through ofImage and copying into a cv::Mat avoids
    // the dependency entirely.
    ofImage img;
    if (!img.load(imagePath)) {
        ofLogError() << "FeatureExtractor: ofImage::load failed for " << imagePath;
        return fv;
    }
    img.setImageType(OF_IMAGE_COLOR);
    ofPixels& px = img.getPixels();
    int w = (int)px.getWidth();
    int h = (int)px.getHeight();
    if (w <= 0 || h <= 0) {
        ofLogError() << "FeatureExtractor: zero-size image for " << imagePath;
        return fv;
    }

    // ofPixels in OF_IMAGE_COLOR is RGB, 8-bit, 3-channel, tightly packed.
    cv::Mat rgb(h, w, CV_8UC3, (void*)px.getData());
    cv::Mat bgr;
    cv::cvtColor(rgb, bgr, cv::COLOR_RGB2BGR);

    // Downsample for speed/consistency.
    float scale = std::min(1.f, (float)maxDim / std::max(w, h));
    if (scale < 1.f) {
        cv::resize(bgr, bgr, cv::Size((int)(w * scale), (int)(h * scale)),
                   0, 0, cv::INTER_AREA);
    }

    cv::Mat gray;
    cv::cvtColor(bgr, gray, cv::COLOR_BGR2GRAY);

    // Luminance mean + variance.
    {
        cv::Scalar mean, stddev;
        cv::meanStdDev(gray, mean, stddev);
        fv.meanLum = (float)(mean[0] / 255.0);
        float s = (float)(stddev[0] / 255.0);
        fv.varLum = s * s;
    }

    // Hue variance (OpenCV's H channel is 0..180).
    {
        cv::Mat hsv;
        cv::cvtColor(bgr, hsv, cv::COLOR_BGR2HSV);
        std::vector<cv::Mat> ch;
        cv::split(hsv, ch);
        cv::Scalar mean, stddev;
        cv::meanStdDev(ch[0], mean, stddev);
        float s = (float)(stddev[0] / 180.0);
        fv.varHue = s * s;
    }

    // Edge density via Canny.
    {
        cv::Mat edges;
        cv::Canny(gray, edges, 80, 160);
        fv.edgeDensity = (float)cv::countNonZero(edges) /
                         (float)(edges.rows * edges.cols);
    }

    // Texture: mean of local variance over a 7x7 window.
    {
        cv::Mat gray32; gray.convertTo(gray32, CV_32F);
        cv::Mat localMean, localSqMean;
        cv::boxFilter(gray32, localMean, CV_32F, cv::Size(7, 7));
        cv::Mat sq; cv::multiply(gray32, gray32, sq);
        cv::boxFilter(sq, localSqMean, CV_32F, cv::Size(7, 7));
        cv::Mat localVar = localSqMean - localMean.mul(localMean);
        cv::Scalar mean, stddev;
        cv::meanStdDev(localVar, mean, stddev);
        double v = std::max(0.0, mean[0]);
        fv.textureVar = (float)(std::sqrt(v) / 255.0);
    }

    // ORB keypoints + binary descriptors.
    {
        auto orb = cv::ORB::create(maxOrbKeypoints);
        std::vector<cv::KeyPoint> kp;
        cv::Mat desc;
        orb->detectAndCompute(gray, cv::noArray(), kp, desc);
        if (!desc.empty()) {
            fv.orbNumKeypoints = desc.rows;
            size_t bytes = (size_t)desc.rows * (size_t)desc.cols;
            fv.orbDescriptors.assign((uint8_t*)desc.datastart,
                                     (uint8_t*)desc.datastart + bytes);
        }
    }

    fv.valid = true;
    return fv;
}

FeatureVector FeatureExtractor::computeFromVideoPath(const std::string& videoPath,
                                                     int maxSamples,
                                                     int maxDim) {
    FeatureVector fv;
    fv.filename = ofFilePath::getFileName(videoPath);
    fv.isVideo = true;

    cv::VideoCapture cap(videoPath);
    if (!cap.isOpened()) {
        ofLogError() << "FeatureExtractor: cannot open video " << videoPath;
        return fv;
    }

    int total = (int)cap.get(cv::CAP_PROP_FRAME_COUNT);
    if (total <= 1) total = 0; // unknown / unreliable

    // Sample evenly across the clip so cost is bounded regardless of length.
    int samples = (total > 0) ? std::min(maxSamples, total) : maxSamples;
    int step = (total > 0) ? std::max(1, total / samples) : 1;

    std::vector<float> diffs;       // per-step mean absolute difference (0..1)
    diffs.reserve(samples);
    cv::Mat frame, gray, prevGray;
    double lumSum = 0.0; int lumN = 0;

    for (int s = 0; s < samples; s++) {
        if (total > 0) cap.set(cv::CAP_PROP_POS_FRAMES, (double)(s * step));
        if (!cap.read(frame) || frame.empty()) break;

        // Downsample for speed/consistency.
        float scale = std::min(1.f, (float)maxDim / std::max(frame.cols, frame.rows));
        if (scale < 1.f)
            cv::resize(frame, frame, cv::Size((int)(frame.cols * scale),
                                              (int)(frame.rows * scale)), 0, 0, cv::INTER_AREA);

        if (frame.channels() == 1) gray = frame;
        else cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

        { cv::Scalar m = cv::mean(gray); lumSum += m[0]; lumN++; }

        if (!prevGray.empty() && prevGray.size() == gray.size()) {
            cv::Mat d;
            cv::absdiff(gray, prevGray, d);
            diffs.push_back((float)(cv::mean(d)[0] / 255.0)); // 0..1
        }
        gray.copyTo(prevGray);
    }

    if (diffs.empty()) {
        ofLogError() << "FeatureExtractor: no frames decoded for " << videoPath;
        return fv;
    }

    // Motion energy = mean frame-to-frame difference.
    double mean = 0.0;
    for (float d : diffs) mean += d;
    mean /= diffs.size();

    // Rhythm = standard deviation of the motion signal (how bursty / pulsing).
    double var = 0.0;
    for (float d : diffs) var += (d - mean) * (d - mean);
    var /= diffs.size();

    fv.motionEnergy = (float)mean;
    fv.videoRhythm  = (float)std::sqrt(var);
    fv.meanLum      = (lumN > 0) ? (float)(lumSum / lumN / 255.0) : 0.f;
    fv.valid = true;

    ofLogNotice() << "Video " << fv.filename
                  << "  motionEnergy=" << fv.motionEnergy
                  << "  rhythm=" << fv.videoRhythm
                  << "  (" << diffs.size() << " samples)";
    return fv;
}
