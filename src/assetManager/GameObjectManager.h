#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "../objects/GameObject.h"

#pragma once

class GameObjectManager final
{
public:
    using ObjectId = GameObject::id_t;

    explicit GameObjectManager() {};

    GameObjectManager(const GameObjectManager&) = delete;
    GameObjectManager& operator=(const GameObjectManager&) = delete;

    GameObject& add(std::unique_ptr<GameObject> object);
    GameObject* find(ObjectId id) noexcept;
    GameObject* findByName(const std::string& name) noexcept;

    bool remove(ObjectId id) noexcept;
    bool removeByName(const std::string& name) noexcept;

    void update(float deltaTime);

private:
    void indexObject(GameObject& object);
    void unindexObject(GameObject& object);

    std::unordered_map<ObjectId, std::unique_ptr<GameObject>> objects_;
    std::unordered_map<std::string, ObjectId> idsByName_;
    std::vector<ObjectId> object_to_be_removed_;

};
