#pragma once

#include "FeatureVector.h"
#include <map>
#include <string>

class FeatureStore {
public:
    bool load(const std::string& xmlPath);
    bool save(const std::string& xmlPath) const;

    bool has(const std::string& filename) const;
    const FeatureVector* get(const std::string& filename) const;
    void put(const FeatureVector& fv);

    size_t size() const { return features.size(); }

private:
    std::map<std::string, FeatureVector> features;

    static std::string toHex(const std::vector<uint8_t>& data);
    static std::vector<uint8_t> fromHex(const std::string& hex);
};
