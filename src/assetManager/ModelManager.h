//#pragma once
//
//#include "../Textures/TextureBuilder.h"
//#include "../Textures/TextureObject.h"
//
//#include <unordered_map>
//#include <mutex>
//
//class ModelManager {
//
//    struct CacheEntry {
//        std::unique_ptr<TextureObject> texture;
//        size_t refCount = 0;
//    };
//
//public:
//    using ModelID = uint64_t;
//
//    explicit ModelManager();
//
//    ModelID create(TextureBuilder& builder);
//    TextureObject* get(const ModelID id) const;
//
//    void remove(const ModelID id);
//    void removeAll();
//
//private:
//    std::mutex mutex;
//    std::unordered_map<size_t, CacheEntry> cache;
//};
