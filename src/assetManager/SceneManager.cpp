/*#include "SceneManager.h"
#include "../objects/GameObject.h"

#include <fstream>

#include <../json.hpp>
#include <vector>
using json = nlohmann::json;

namespace {



}

void SceneManager::save(std::vector<GameObject*> gameObjects)
{
	json currentSceneJson = json::parse(R"({})");

	for (auto obj : gameObjects) {
		if (obj->saveable)
			addObjectToScene(currentSceneJson, obj);
	}

	std::string scenePath = "scenes/" + currentSceneName + ".json";
	std::ofstream o(scenePath);
	o << currentSceneJson << std::endl;

}

void SceneManager::open(std::string new_scene)
{
	std::string scenePath = new_scene.size() ? new_scene : currentSceneName;
	scenePath = "scenes/" + scenePath + ".json";


	std::ifstream i(scenePath);
	if (!i.good()) {
		create(scenePath);
		i.open(scenePath);
	}

	json currentSceneJson;
}

void SceneManager::load()
{}

void SceneManager::create(std::string name)
{
	json empty = json::parse(R"({})");

	std::string path = "scenes/" + name + ".json";

	// create json file with name
	std::ofstream o(path);
	o << empty << std::endl;
}*/
