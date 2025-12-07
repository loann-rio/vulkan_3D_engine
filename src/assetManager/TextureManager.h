#pragma once

#include "../Textures/TextureBuilder.h"
#include "../Textures/TextureObject.h"

#include <unordered_map>

class TextureManager {
public:
    using TextureID = uint64_t;

    explicit TextureManager() {}

    TextureID create(TextureBuilder& builder);
    TextureObject* get(const TextureID id) const;

    void remove(const TextureID id);
    void removeAll();

private:
    std::unordered_map<size_t, std::unique_ptr<TextureObject>> cache;
};
