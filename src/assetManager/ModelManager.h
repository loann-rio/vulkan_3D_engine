#pragma once

#include "../model/ModelAsset.h"
#include "../model/ModelBuilder.h"

#include <unordered_map>
#include <mutex>

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
