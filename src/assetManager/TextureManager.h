#pragma once

#include "../Textures/TextureBuilder.h"
#include "../Textures/TextureObject.h"

#include <unordered_map>
#include <mutex>

class TextureManager {

    struct CacheEntry {
        std::unique_ptr<TextureObject> texture;
		size_t refCount = 0;
	};

public:
    using TextureID = uint64_t;

    explicit TextureManager();

    TextureID create(TextureBuilder& builder);
    TextureObject* get(const TextureID id) const;

    void remove(const TextureID id);
    void removeAll();

private:
    std::mutex mutex;
    std::unordered_map<size_t, CacheEntry> cache;
};
