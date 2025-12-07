#pragma once

#include <memory>
#include <unordered_map>
#include <typeindex>
#include <vector>
#include <string>

#include "GameObject.h"

class GameObjectRegistry
{
public:

    GameObjectRegistry()
        : objects(std::make_shared<GameObject::Map>()) {
    }

    GameObject* add(std::unique_ptr<GameObject> obj);

    void remove(const GameObject::id_t id);
    void remove(const std::string& name);
    void remove(GameObject* gameObject);
    
    GameObject* get(GameObject::id_t id);
    GameObject* get(const std::string& name);

    template <typename T>
    std::vector<T*> getByType();

private:
    std::shared_ptr<GameObject::Map> objects;
    std::unordered_map<std::string, GameObject*> byName;
    std::unordered_map<std::type_index, std::vector<GameObject*>> byType;

};

template<typename T>
inline std::vector<T*> GameObjectRegistry::getByType()
{
    std::vector<T*> result;

    for (auto& [typeIndex, vec] : byType)
    {
        for (GameObject* obj : vec)
        {
            if (T* casted = dynamic_cast<T*>(obj))
                result.push_back(casted);
        }
    }

    return result;
}
