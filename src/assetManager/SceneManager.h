// devide in fix/modifiable?
// saveScene only in debug?

#include <string>


class SceneManager
{
public:
	
	void save();
	void open(std::string new_scene = "");

private:
	void load();
	void create(std::string path);

	std::string mainCamera = "mainCamera";
	std::string mainSkybox = "cubemap";

	std::string currentSceneName = "test5";

};