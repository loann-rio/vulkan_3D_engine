/*// devide in fix/modifiable?
// saveScene only in debug?

#include <string>


class SceneManager
{
public:
	
	void save(std::vector<GameObject*> gameObjects);
	void open(std::string new_scene = "");

private:
	void load();
	void create(std::string name);

	std::string mainCamera = "mainCamera";
	std::string mainSkybox = "cubemap";

	std::string currentSceneName = "test5";

};*/