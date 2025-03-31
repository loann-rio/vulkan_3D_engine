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
    pool_info.poolSizeCount = std::size(pool_sizes);
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

void BasicUI::drawUI(VkCommandBuffer commandBuffer, ObjectManager* manager)
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

    objectSelectionWindow(listObjectsName,  manager);

    auto gameObject = manager->get(selected_object);
    if (gameObject != nullptr) {
        gameObjectWindow(gameObject); 
    }

    ImGui::Render(); 
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);  
}



void BasicUI::gameObjectWindow(GameObject* gameObject)
{
    ImGui::Begin("Object debug");

    isWindowSelected = (isWindowSelected || ImGui::IsWindowFocused()); 

    gameObject->debugUI();

    ImGui::End(); 
}

void BasicUI::objectSelectionWindow(std::vector<std::string> listObjectsName, ObjectManager* manager)
{
    ImGui::Begin("selector");

    isWindowSelected = (isWindowSelected || ImGui::IsWindowFocused());

    static int sub_selected = -1;  // Store sub-selection
    static bool open_sub_popup = false;

    ImGui::SeparatorText("create new game object");
    if (ImGui::Button("Select obj type"))
        ImGui::OpenPopup("gameObjectPopup");     

    if (ImGui::BeginPopup("gameObjectPopup"))
    {
        ImGui::SeparatorText("object type");

        if (ImGui::Selectable("Model")) { open_sub_popup = true; }
        if (ImGui::Selectable("Camera")) {}
        if (ImGui::Selectable("SpotLight")) {}

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

        if (ImGui::Selectable("OBJ")) sub_selected = 0;
        if (ImGui::Selectable("GLTF")) sub_selected = 1; 


        ImGui::EndPopup(); 
    }

    ImGui::SeparatorText("loaded game objects"); 

    static int item_current = 0;

    std::vector<const char*> listObjectsNameCStr;
    for (const auto& str : listObjectsName) {
        listObjectsNameCStr.push_back(str.c_str());
    } 

    
    ImGui::ListBox("##go", &item_current, listObjectsNameCStr.data(), listObjectsNameCStr.size());

    selected_object = listObjectsName[item_current];

    ImGui::End();
}

void BasicUI::createObjWindow()
{
    ImGui::Begin("createObject");

    ImGui::SeparatorText("create obj model");

    ImGui::Text("model path");
    static char path[128] = "";
    ImGui::InputTextWithHint("##path", "enter model path", path, IM_ARRAYSIZE(path));

    ImGui::Text("texture path");
    static char pathTexture[128] = "";
    ImGui::InputTextWithHint("##pathTexture", "enter texture path", pathTexture, IM_ARRAYSIZE(pathTexture)); 

    if (ImGui::Button("create"))
    {
        std::memset(path, 0, sizeof(path));
        std::memset(pathTexture, 0, sizeof(pathTexture));
    }

    ImGui::End(); 
}

void BasicUI::createGLTFWindow()
{
    ImGui::Begin("createObject");

    ImGui::SeparatorText("create obj model");

    ImGui::Text("model path");
    static char path[128] = "";
    ImGui::InputTextWithHint("##path", "enter model path", path, IM_ARRAYSIZE(path));

    if (ImGui::Button("create"))
    {
        std::memset(path, 0, sizeof(path));
    }

    ImGui::End();
}

