#include "TextureManager.h"

TextureManager::TextureManager()
{
	cache = std::unordered_map<size_t, CacheEntry>{};
}

TextureManager::TextureID TextureManager::create(TextureBuilder& builder)
{
	// lock for thread safety
    std::lock_guard<std::mutex> lock(mutex);

    // time hashing process
    TextureManager::TextureID key = builder.hash();

	auto it = cache.find(key);
    if (it != cache.end()) {
		it->second.refCount++;
        return key;
    }

    std::unique_ptr<TextureObject> texture = builder.build();

    if (!texture) {
        return 0; // failed to build texture
	}

    cache[key] = { std::move(texture), 0 };

    return key;
}

TextureObject* TextureManager::get(const TextureID id) const
{
    auto it = cache.find(id);
    return (it != cache.end()) ? it->second.texture.get() : nullptr;
}

void TextureManager::remove(const TextureID id)
{
    auto it = cache.find(id);
    if (it == cache.end())
        return; // unknown ID

    it->second.refCount--;
    if (it->second.refCount < 0)
        cache.erase(it);
}

void TextureManager::removeAll()
{
    cache.clear();
}
