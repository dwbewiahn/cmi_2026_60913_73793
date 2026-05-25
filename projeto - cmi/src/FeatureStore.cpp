#include "FeatureStore.h"
#include "ofMain.h"

bool FeatureStore::has(const std::string& filename) const {
    return features.find(filename) != features.end();
}

const FeatureVector* FeatureStore::get(const std::string& filename) const {
    auto it = features.find(filename);
    return (it == features.end()) ? nullptr : &it->second;
}

void FeatureStore::put(const FeatureVector& fv) {
    features[fv.filename] = fv;
}

bool FeatureStore::load(const std::string& xmlPath) {
    if (!ofFile::doesFileExist(xmlPath)) return false;

    ofXml xml;
    if (!xml.load(xmlPath)) {
        ofLogWarning() << "FeatureStore: failed to parse " << xmlPath;
        return false;
    }

    features.clear();
    auto photos = xml.find("//photo");
    for (auto& p : photos) {
        FeatureVector fv;
        fv.filename       = p.getChild("filename").getValue();
        fv.meanLum        = p.getChild("mean_lum").getFloatValue();
        fv.varLum         = p.getChild("var_lum").getFloatValue();
        fv.varHue         = p.getChild("var_hue").getFloatValue();
        fv.edgeDensity    = p.getChild("edge_density").getFloatValue();
        fv.textureVar     = p.getChild("texture_var").getFloatValue();
        fv.orbNumKeypoints = p.getChild("orb_num").getIntValue();
        fv.orbDescriptors = fromHex(p.getChild("orb_desc").getValue());
        fv.valid = !fv.filename.empty();
        if (fv.valid) features[fv.filename] = fv;
    }

    ofLogNotice() << "FeatureStore: loaded " << features.size()
                  << " entries from " << xmlPath;
    return true;
}

bool FeatureStore::save(const std::string& xmlPath) const {
    ofXml xml;
    auto root = xml.appendChild("features");
    for (const auto& kv : features) {
        const FeatureVector& fv = kv.second;
        auto p = root.appendChild("photo");
        p.appendChild("filename").set(fv.filename);
        p.appendChild("mean_lum").set(fv.meanLum);
        p.appendChild("var_lum").set(fv.varLum);
        p.appendChild("var_hue").set(fv.varHue);
        p.appendChild("edge_density").set(fv.edgeDensity);
        p.appendChild("texture_var").set(fv.textureVar);
        p.appendChild("orb_num").set(fv.orbNumKeypoints);
        p.appendChild("orb_desc").set(toHex(fv.orbDescriptors));
    }
    bool ok = xml.save(xmlPath);
    if (ok) ofLogNotice() << "FeatureStore: saved " << features.size()
                          << " entries to " << xmlPath;
    return ok;
}

std::string FeatureStore::toHex(const std::vector<uint8_t>& data) {
    static const char hexDigits[] = "0123456789abcdef";
    std::string out;
    out.resize(data.size() * 2);
    for (size_t i = 0; i < data.size(); i++) {
        out[2*i]     = hexDigits[data[i] >> 4];
        out[2*i + 1] = hexDigits[data[i] & 0xF];
    }
    return out;
}

std::vector<uint8_t> FeatureStore::fromHex(const std::string& hex) {
    auto hexVal = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return 0;
    };
    std::vector<uint8_t> out;
    out.reserve(hex.size() / 2);
    for (size_t i = 0; i + 1 < hex.size(); i += 2) {
        out.push_back((uint8_t)((hexVal(hex[i]) << 4) | hexVal(hex[i + 1])));
    }
    return out;
}
