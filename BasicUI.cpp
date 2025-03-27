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

void BasicUI::drawUI(VkCommandBuffer commandBuffer, GameObject* gameObject)
{
    ImGui_ImplVulkan_NewFrame(); 
    ImGui_ImplGlfw_NewFrame(); 
    ImGui::NewFrame(); 

    // Example UI
    gameObjectWindow(gameObject);

    ImGui::Render(); 
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer); 
}

void BasicUI::gameObjectWindow(GameObject* gameObject)
{
    ImGui::Begin("Debug Info");

    ImGui::Text("Position:");
    ImGui::DragFloat3("pos", glm::value_ptr(gameObject->transform.translation), 0.01f, -10.0f, 10.0f);

    ImGui::Text("Rotation:");
    ImGui::DragFloat3("rot", glm::value_ptr(gameObject->transform.rotation), 0.01f, -10.0f, 10.0f);
    
    ImGui::Text("Scale:");
    ImGui::DragFloat3("scale", glm::value_ptr(gameObject->transform.scale), 0.01f, -10.0f, 10.0f);

    ImGui::End();
}
