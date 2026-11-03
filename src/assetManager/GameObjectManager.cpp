#include "GameObjectManager.h"
#include "../objects/GameObject.h"

#include <stdexcept>
#include <string>
#include <memory>

GameObject& GameObjectManager::add(std::unique_ptr<GameObject> object)
{
    if (!object) {
        throw std::invalid_argument("Cannot add a null GameObject");
    }

    const ObjectId id = object->getId();
    if (objects_.find(id) != objects_.end()) {
        throw std::invalid_argument("A GameObject with this ID is already managed");
    }

    const std::string name = object->getName();
    if (!name.empty() && idsByName_.find(name) != idsByName_.end()) {
        throw std::invalid_argument("A GameObject with this name is already managed");
    }

    auto [objectIt, inserted] = objects_.emplace(id, std::move(object));
    if (!inserted) {
        throw std::invalid_argument("A GameObject with this ID is already managed");
    }

    try {
        indexObject(*objectIt->second);
    }
    catch (...) {
        objects_.erase(objectIt);
        throw;
    }

    return *objectIt->second;
}

GameObject* GameObjectManager::find(ObjectId id) noexcept
{
	auto it = objects_.find(id);
	return (it != objects_.end()) ? it->second.get() : nullptr;
}

GameObject* GameObjectManager::findByName(const std::string& name) noexcept
{
	auto it = idsByName_.find(name);
	return find((it != idsByName_.end()) ? it->second : 0);
}

bool GameObjectManager::remove(ObjectId id) noexcept
{
    const auto it = objects_.find(id);
    if (it == objects_.end()) {
        return false;
    }

    unindexObject(*it->second);
    objects_.erase(it);
    return true;
}

bool GameObjectManager::removeByName(const std::string& name) noexcept
{
    const auto it = idsByName_.find(name);
    return it != idsByName_.end() && remove(it->second);
}

void GameObjectManager::update(float deltaTime)
{}

void GameObjectManager::indexObject(GameObject& object)
{
    const std::string name = object.getName();
    if (name.empty()) {
        return;
    }

    const auto [it, inserted] = idsByName_.emplace(name, object.getId());
    if (!inserted) {
        throw std::invalid_argument("A GameObject with this name is already managed");
    }
}

void GameObjectManager::unindexObject(GameObject& object)
{
    for (auto it = idsByName_.begin(); it != idsByName_.end();) {
        if (it->second == object.getId()) {
            it = idsByName_.erase(it);
        }
        else {
            ++it;
        }
    }
}
