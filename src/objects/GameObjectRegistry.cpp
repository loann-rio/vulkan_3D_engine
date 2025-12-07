#include "GameObjectRegistry.h"

GameObject* GameObjectRegistry::add(std::unique_ptr<GameObject> gameObject)
{
    GameObject::id_t id = gameObject->getId();
    std::string name = gameObject->getName();
    std::type_index type = typeid(*gameObject);

    GameObject* ptr = gameObject.get();

    (*objects)[id] = std::move(gameObject);

    // Store pointer in name map
    if (!name.empty()) {
        byName[name] = ptr;
    }

    // Store in type-indexed list
    byType[type].push_back(ptr);

    return ptr;
}

void GameObjectRegistry::remove(const GameObject::id_t id)
{
    auto it = objects->find(id);
    if (it == objects->end()) {
        return; // nothing to remove
    }

    GameObject* obj = it->second.get();

    // Remove from name map
    if (!obj->getName().empty()) {
        byName.erase(obj->getName());
    }

    // Remove from type-indexed list
    std::type_index type = typeid(*obj);
    auto& vec = byType[type];
    vec.erase(std::remove(vec.begin(), vec.end(), obj), vec.end());
    if (vec.empty())
        byType.erase(type);

    // remove from main storage
    objects->erase(it);
}

void GameObjectRegistry::remove(const std::string& name)
{
    GameObject* gameObject = get(name);
    if (gameObject) remove(gameObject->getId());
}

void GameObjectRegistry::remove(GameObject* gameObject)
{
    if (gameObject) remove(gameObject->getId());
}

GameObject* GameObjectRegistry::get(GameObject::id_t id)
{
    auto it = objects->find(id);
    return (it != objects->end()) ? it->second.get() : nullptr;
}

GameObject* GameObjectRegistry::get(const std::string& name)
{
    auto it = byName.find(name);
    return (it != byName.end()) ? it->second : nullptr;
}
