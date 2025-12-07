#include "TextureManager.h"

TextureManager::TextureID TextureManager::create(TextureBuilder& builder)
{
    size_t key = builder.hash();

    if (cache.find(key) != cache.end()) {
        return key;
    }

    std::unique_ptr<TextureObject> texture = builder.build();
    cache[key] = std::move(texture);

    return key;
}

TextureObject* TextureManager::get(const TextureID id) const
{
    auto it = cache.find(id);
    return (it != cache.end()) ? it->second.get() : nullptr;
}

void TextureManager::remove(const TextureID id)
{
    auto it = cache.find(id);
    if (it == cache.end())
        return; // unknown ID

    cache.erase(it);
}

void TextureManager::removeAll()
{
    cache.clear();
}
