#pragma once

#include <unordered_map>
#include <mutex>
#include <glm/fwd.hpp>
#include <cstdint>
#include <memory>

#include "../model/ModelAsset.h"


class ModelBuilder;

struct ModelInstance {
    glm::vec4 position;
    glm::vec4 rotation;
    glm::vec4 scale;
};

class ModelManager {

    struct CacheEntry {
        std::unique_ptr<ModelAsset> model;
        size_t refCount = 0;
    };

public:
    using ModelID = uint64_t;

    explicit ModelManager();

    ModelID create(ModelBuilder& builder);
    ModelAsset* get(const ModelID id) const;

    void remove(const ModelID id);
    void removeAll();

private:
    std::mutex mutex;
    std::unordered_map<size_t, CacheEntry> cache;
};
