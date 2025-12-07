//#include "ModelManager.h"
//
//ModelManager::ModelManager()
//{
//    cache = std::unordered_map<size_t, CacheEntry>{};
//}
//
//ModelManager::ModelID ModelManager::create(TextureBuilder& builder)
//{
//    // lock for thread safety
//    std::lock_guard<std::mutex> lock(mutex);
//
//    // time hashing process
//    ModelManager::ModelID key = 1;
//
//    auto it = cache.find(key);
//    if (it != cache.end()) {
//        it->second.refCount++;
//        return key;
//    }
//
//    std::unique_ptr<TextureObject> texture = builder.build();
//
//    if (!texture) {
//        return 0; // failed to build texture
//    }
//
//    cache[key] = { std::move(texture), 0 };
//
//    return key;
//}
//
//TextureObject* ModelManager::get(const ModelID id) const
//{
//    auto it = cache.find(id);
//    return (it != cache.end()) ? it->second.texture.get() : nullptr;
//}
//
//void ModelManager::remove(const ModelID id)
//{
//    auto it = cache.find(id);
//    if (it == cache.end())
//        return; // unknown ID
//
//    it->second.refCount--;
//    if (it->second.refCount < 0)
//        cache.erase(it);
//}
//
//void ModelManager::removeAll()
//{
//    cache.clear();
//}
