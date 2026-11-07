#include "SceneManager.h"
#include <fstream>

#include <../json.hpp>
using json = nlohmann::json;

namespace {



}

void SceneManager::save()
{}

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

void SceneManager::create(std::string path)
{}
