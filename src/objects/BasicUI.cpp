#include "BasicUI.h"

BasicUI::BasicUI(Device& device, GLFWwindow* window, VkRenderPass renderPass) : device{ device }
{
   // the size of the pool is very oversize, but it's copied from imgui demo itself.
    VkDescriptorPoolSize pool_sizes[] =
    {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000;
    pool_info.poolSizeCount = static_cast<int>(std::size(pool_sizes));
    pool_info.pPoolSizes = pool_sizes;

    if (vkCreateDescriptorPool(device.device(), &pool_info, nullptr, &imguiPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui_ImplGlfw_InitForVulkan(window, false);

    ImGui_ImplVulkan_InitInfo init_info = device.getImGuiInitInfo();
    init_info.DescriptorPool = imguiPool;
    init_info.RenderPass = renderPass;

    if (!ImGui_ImplVulkan_Init(&init_info)) {
        throw std::runtime_error("ImGui_ImplVulkan_Init FAILED!");
    }

    ImGui_ImplVulkan_CreateFontsTexture();

    glfwSetMouseButtonCallback(window, ImGui_ImplGlfw_MouseButtonCallback);
    glfwSetScrollCallback(window, ImGui_ImplGlfw_ScrollCallback);
    glfwSetKeyCallback(window, ImGui_ImplGlfw_KeyCallback);
    glfwSetCharCallback(window, ImGui_ImplGlfw_CharCallback);
}

BasicUI::~BasicUI()
{
    ImGui_ImplVulkan_Shutdown();
    vkDestroyDescriptorPool(device.device(), imguiPool, nullptr); 
}

void BasicUI::drawUI(VkCommandBuffer commandBuffer, ObjectManager* manager, TerrainUbo& terrainUbo, float fps)
{
    isWindowSelected = false;

    ImGui_ImplVulkan_NewFrame(); 
    ImGui_ImplGlfw_NewFrame(); 
    ImGui::NewFrame();  

    auto objects = manager->getGameObjects();

    std::vector<std::string> listObjectsName;
    for (auto i = objects->begin(); i != objects->end(); i++) {
        auto name = i->second->getName();
        if (name.size()) 
            listObjectsName.push_back(name);
    }   

    objectSelectionWindow(listObjectsName, manager, fps);

	//terrainUboWindow(terrainUbo);

    auto gameObject = manager->get(selected_object);
    if (gameObject != nullptr) {
        gameObjectWindow(gameObject, manager); 
    }

    ImGui::Render(); 
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
}



void BasicUI::gameObjectWindow(GameObject* gameObject, ObjectManager* manager)
{
    ImGui::Begin("Object debug");

    isWindowSelected = (isWindowSelected || ImGui::IsWindowFocused()); 

    if (gameObject->getType() == GameObjectType::CAMERA) {
        bool check = (gameObject->getName() == manager->mainCamera);
        
        if (ImGui::Checkbox("main camera", &check) && check)
            manager->mainCamera = gameObject->getName();
    }

    gameObject->debugUI();

    if (ImGui::Button("remove object"))
        gameObject->toBeRemoved = true;

    ImGui::End(); 
}

void BasicUI::objectSelectionWindow(std::vector<std::string> listObjectsName, ObjectManager* manager, float fps)
{
	
    ImGui::Begin("selector");

    isWindowSelected = (isWindowSelected || ImGui::IsWindowFocused());

    static bool open_sub_popup = false;

    ImGui::Text("fps: %.1f", fps);

    ImGui::SeparatorText("create new game object");
    if (ImGui::Button("Select obj type"))
        ImGui::OpenPopup("gameObjectPopup");     

    if (ImGui::BeginPopup("gameObjectPopup"))
    {
        ImGui::SeparatorText("object type");

        if (ImGui::Selectable("Model")) { open_sub_popup = true; }
        if (ImGui::Selectable("Camera")) { selected = 2; }
        if (ImGui::Selectable("SpotLight")) { selected = 3; }

        ImGui::EndPopup(); 
    }
     
    if (open_sub_popup) 
    {
        ImGui::OpenPopup("modelSelection");
        open_sub_popup = false;
    }

    
    if (ImGui::BeginPopup("modelSelection")) 
    {
        ImGui::SeparatorText("Model type"); 

        if (ImGui::Selectable("OBJ")) selected = 0;  
        if (ImGui::Selectable("GLTF")) selected = 1;  
        if (ImGui::Selectable("SKYBOX")) selected = 4;


        ImGui::EndPopup(); 
    }

    show_create_go_window = true;
    switch (selected) 
    {
    case 0:
        createObjWindow(manager);
        break;
    case 1:
        createGLTFWindow(manager); 
        break;
    case 2:
        createCameraWindow(manager, false);
        break;
    case 3:
        createCameraWindow(manager, true);
        break;
	case 4: 
        createSkyboxWindow(manager);
        break;
    default:
        show_create_go_window = false; 
    }

    ////// scene selection //////

    ImGui::SeparatorText("scene selection");

    ImGui::Text(manager->currentScene.c_str());

    static char newScene[128] = "";
    ImGui::InputTextWithHint("##newScene", "new scene path", newScene, IM_ARRAYSIZE(newScene));

    if (ImGui::Button("change scene"))
        manager->switchScene(newScene);

    ////// object selection //////
    ImGui::SeparatorText("loaded game objects"); 

    static int item_current = 0;

    std::vector<const char*> listObjectsNameCStr;
    for (const auto& str : listObjectsName) {
        listObjectsNameCStr.push_back(str.c_str());
    } 

    ImGui::ListBox("##go", &item_current, listObjectsNameCStr.data(), static_cast<int>(listObjectsNameCStr.size()));


    selected_object = listObjectsName[item_current % listObjectsName.size()];

    ImGui::End();
}

void BasicUI::terrainUboWindow(TerrainUbo& terrainUbo)
{
    ImGui::Begin("terrain data", &show_create_terrain_window);

    isWindowSelected = (isWindowSelected || ImGui::IsWindowFocused());

    ImGui::Text("clif_slop");
    ImGui::DragFloat("##clif_slop", &terrainUbo.clif_slop, 0.01f, 0.0f, 1.0f);
    ImGui::Text("height_grass");
    ImGui::DragFloat("##height_grass", &terrainUbo.height_grass, 0.1f, -10.f, 10.0f);
    ImGui::Text("slope_snow");
    ImGui::DragFloat("##slope_snow ", &terrainUbo.slope_snow, 0.01f, 0.0f, 1.0f);
    ImGui::Text("height_grass_with_slope");
    ImGui::DragFloat("##height_grass_with_slope", &terrainUbo.height_grass_with_slope, 0.1f, -10.f, 10.0f);
    ImGui::Text("height_dirt_with_slope");
    ImGui::DragFloat("##height_dirt_with_slope", &terrainUbo.height_dirt_with_slope, 0.1f, -10.f, 10.0f);
    ImGui::Text("height_snow");
    ImGui::DragFloat("##height_snow", &terrainUbo.height_snow, 0.1f, -10.f, 10.0f);

    ImGui::End();

    if (!show_create_terrain_window) selected = -1;
}

void BasicUI::createObjWindow(ObjectManager* manager)
{
    ImGui::Begin("createObject", &show_create_go_window);

    isWindowSelected = (isWindowSelected || ImGui::IsWindowFocused());

    ImGui::SeparatorText("create obj model");


    ImGui::Text("name");
    static char name[128] = "";
    ImGui::InputTextWithHint("##name", "enter name", name, IM_ARRAYSIZE(name));

    ImGui::Text("model path");
    static char path[128] = "";
    ImGui::InputTextWithHint("##path", "enter model path", path, IM_ARRAYSIZE(path));

    if (ImGui::Button("model dialog")) {
        std::string filePath = openFileDialog("obj");

        if (!filePath.empty())
            strcpy_s(path, filePath.c_str());
    }

    ImGui::Text("texture path");
    static char pathTexture[128] = "";
    ImGui::InputTextWithHint("##pathTexture", "enter texture path", pathTexture, IM_ARRAYSIZE(pathTexture)); 

    if (ImGui::Button("texture dialog")) {
        std::string filePath = openFileDialog("jpg,png,ktx,ktx2");

        if (!filePath.empty())
            strcpy_s(pathTexture, filePath.c_str()); 
    }

    if (ImGui::Button("create"))
    {
        manager->loadObjectAsyncObj(device, path, pathTexture ? pathTexture : "textures\\whiteTexture.jpg", TransformComponent{}, name);
        show_create_go_window = false; 
      
    }

    ImGui::End(); 

    if (!show_create_go_window) selected = -1; 
}


void BasicUI::createGLTFWindow(ObjectManager* manager)
{
    ImGui::Begin("createObject", &show_create_go_window); 

    isWindowSelected = (isWindowSelected || ImGui::IsWindowFocused());

    ImGui::SeparatorText("create obj model");

    ImGui::Text("name");
    static char name[128] = "";
    ImGui::InputTextWithHint("##name", "enter name", name, IM_ARRAYSIZE(name)); 

    ImGui::Text("model path");
    static char path[128] = "";
    ImGui::InputTextWithHint("##path", "enter model path", path, IM_ARRAYSIZE(path));

    if (ImGui::Button("open dialog")) {
        std::string filePath = openFileDialog("gltf");

        if (!filePath.empty())
            strcpy_s(path, filePath.c_str());
    }

    if (ImGui::Button("create"))
    {
        manager->loadObjectAsync(device, path, TransformComponent{}, name);
        show_create_go_window = false;
        //std::memset(path, 0, sizeof(path));
    }

    ImGui::End();

    if (!show_create_go_window) selected = -1;
}

void BasicUI::createSkyboxWindow(ObjectManager* manager)
{
  //  ImGui::Begin("createObject", &show_create_go_window);

  //  isWindowSelected = (isWindowSelected || ImGui::IsWindowFocused());

  //  ImGui::SeparatorText("create skybox model");

  //  ImGui::Text("cubemap path");
  //  static char path[128] = "";
  //  ImGui::InputTextWithHint("##path", "enter model path", path, IM_ARRAYSIZE(path));

  //  if (ImGui::Button("open dialog")) {
  //      std::string filePath = openFileDialog("ktx");

  //      if (!filePath.empty())
  //          strcpy_s(path, filePath.c_str());
  //  }

  //  if (ImGui::Button("create"))
  //  {
		//manager->removeGameObject("cubemap");
  //      manager->loadObjectAsync(device, path, TransformComponent{}, "cubemap");
  //      show_create_go_window = false;
  //      //std::memset(path, 0, sizeof(path));
  //  }

  //  ImGui::End();

  //  if (!show_create_go_window) selected = -1;
}

void BasicUI::createCameraWindow(ObjectManager* manager, bool isSpotLight = false)
{
    ImGui::Begin("createObject", &show_create_go_window);

    isWindowSelected = (isWindowSelected || ImGui::IsWindowFocused());

    ImGui::SeparatorText("create obj model");

    ImGui::Text("name");
    static char name[128] = "";
    ImGui::InputTextWithHint("##name", "enter name", name, IM_ARRAYSIZE(name));

    ImGui::Text("fov");
    static float fov = 1.0f;
    ImGui::DragFloat("##fov", &fov, 0.01f, 0.1f, glm::half_pi<float>());

    static glm::vec4 color{ 1.0f };
    static float ar = 1.0f;

    if (isSpotLight)
    {
        ImGui::Text("Aspect Ratio");
        
        ImGui::DragFloat("##aspectRatio", &ar, 0.1f, 1 / 20, 20);

        ImGui::Text("Color:");
        ImGui::ColorEdit4("##clr", glm::value_ptr(color));
    }

    if (ImGui::Button("create"))
    {
        show_create_go_window = false; 

        if (isSpotLight)
        {
            auto spotLight = GameObjectFactory::createGameObject<GameObjectSpotLight>(device, fov, ar, .1f, 100.f);

            spotLight->transform.color = color;
            spotLight->setName(name);

            manager->pushGameObject(std::move(spotLight));
        }
        else {
            auto camera = GameObjectFactory::createGameObject<GameObjectCamera>(device, fov, ar, .1f, 100.f); 

            camera->setName(name);

            manager->pushGameObject(std::move(camera));
        }
    }

    ImGui::End();

    if (!show_create_go_window) selected = -1;
}

std::string BasicUI::openFileDialog(const char* filter)
{
    nfdchar_t* outPath = nullptr;

    nfdresult_t result = NFD_OpenDialog(filter, nullptr, &outPath);
    if (result == NFD_OKAY) {
        std::cout << "Selected file: " << outPath << "\n";
        return std::string{ outPath };
    }
    else if (result == NFD_CANCEL) {
        std::cout << "User canceled.\n";
    }
    else {
        std::cout << "Error: " << NFD_GetError() << "\n";
    }

    return "";
}

