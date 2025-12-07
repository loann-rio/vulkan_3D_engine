#include "ModelManager.h"

ModelManager::ModelManager()
{
    cache = std::unordered_map<size_t, CacheEntry>{};
}

ModelManager::ModelID ModelManager::create(ModelBuilder& builder)
{
    // lock for thread safety
    std::lock_guard<std::mutex> lock(mutex);

    // time hashing process
    ModelManager::ModelID key = builder.hash();

    auto it = cache.find(key);
    if (it != cache.end()) {
        it->second.refCount++;
        return key;
    }

    std::unique_ptr<ModelAsset> model = builder.build();

    if (!model) {
        return 0; // failed to build model
    }

    cache[key] = { std::move(model), 0 };

    return key;
}

ModelAsset* ModelManager::get(const ModelID id) const
{
    auto it = cache.find(id);
    return (it != cache.end()) ? it->second.model.get() : nullptr;
}

void ModelManager::remove(const ModelID id)
{
    auto it = cache.find(id);
    if (it == cache.end())
        return; // unknown ID

    it->second.refCount--;
    if (it->second.refCount < 0)
        cache.erase(it);
}

void ModelManager::removeAll()
{
    cache.clear();
}
