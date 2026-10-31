#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "../objects/GameObject.h"

#pragma once

class ObjectManager final
{
public:
    using ObjectId = GameObject::id_t;

    explicit ObjectManager(GameObjectFactory& factory);

    ObjectManager(const ObjectManager&) = delete;
    ObjectManager& operator=(const ObjectManager&) = delete;

    GameObject& add(std::unique_ptr<GameObject> object);
    GameObject* find(ObjectId id);
    GameObject* findByName(const std::string& name);

    bool remove(ObjectId id);
    bool removeByName(const std::string& name);

    void update(float deltaTime);

private:
    void indexObject(GameObject& object);
    void unindexObject(const GameObject& object);

    std::unordered_map<ObjectId, std::unique_ptr<GameObject>> objects_;
    std::unordered_map<std::string, ObjectId> idsByName_;
    std::vector<ObjectId> object_to_be_removed_;

    GameObjectFactory& factory_;
};
