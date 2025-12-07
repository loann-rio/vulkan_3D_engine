#pragma once
#include <memory>
//#include "ModelManager.h"
#include "TextureManager.h"
//#include "ShaderManager.h"
//#include "ScriptManager.h"

class AssetManager {
public:
    AssetManager() {
		textureMgr = std::make_unique<TextureManager>();
    };

    //ModelManager& models() { return *modelMgr; }
    TextureManager& textures() { return *textureMgr; }
    //ShaderManager& shaders() { return *shaderMgr; }
    //ScriptManager& scripts() { return *scriptMgr; }

    // Update async tasks
    //void update();

private:
    //std::unique_ptr<ModelManager> modelMgr;
    std::unique_ptr<TextureManager> textureMgr;
    //std::unique_ptr<ShaderManager> shaderMgr;
    //std::unique_ptr<ScriptManager> scriptMgr;
};
