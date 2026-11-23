#pragma once

#include <string>

class GameObjectManager;

class SceneSaver final
{
public:
    static void save(const std::string& sceneName, const GameObjectManager& objects);
};