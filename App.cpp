#include "App.h"
#include <stdexcept>
#include <array>


App::App()
{
	loadModels();
	createPipelineLayout();
	createPipeline();
	createCommandBuffer();
}

App::~App()
{
	vkDestroyPipelineLayout(device.device(), pipelineLayout, nullptr);
}

void App::run()
{
	while (!window.shouldClose())
	{
		glfwPollEvents();
		drawFrame();
	}

	vkDeviceWaitIdle(device.device());
}

void App::loadModels()
{
	std::vector<Model::Vertex> vertices{
		{{0.0f, -0.5f}},
		{{0.5f, 0.5f}},
		{{-0.5f, 0.5f}}
	};

	model = std::make_unique<Model>(device, vertices);
}

void App::createPipelineLayout()
{
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0;
	pipelineLayoutInfo.pSetLayouts = nullptr;
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;

	if (vkCreatePipelineLayout(device.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) !=
		VK_SUCCESS) {
		throw std::runtime_error("fail to create pipeline layout");
	}
}

void App::createPipeline()
{
	auto pipelineConfig = Pipeline::defaultPipelineConfigInfo(swapChain.width(), swapChain.height());
	pipelineConfig.renderPass = swapChain.getRenderPass();
	pipelineConfig.pipelineLayout = pipelineLayout;
	pipeline = std::make_unique<Pipeline>(
		device,
		"simple_shader.vert.spv",
		"simple_shader.frag.spv",
		pipelineConfig
	);
}

void App::createCommandBuffer()
{
	commandBuffers.resize(swapChain.imageCount());

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = device.getCommandPool();
	allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

	if (vkAllocateCommandBuffers(device.device(), &allocInfo, commandBuffers.data()) !=
		VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffer");
	}

	for (int i = 0; i < commandBuffers.size(); i++) {
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording command buffer");
		}

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = swapChain.getRenderPass();
		renderPassInfo.framebuffer = swapChain.getFrameBuffer(i);

		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = swapChain.getSwapChainExtent();

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { 0.1f, 0.1f, 0.1f, 0.1f };
		clearValues[1].depthStencil = { 0.1f, 0 };

		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		pipeline->bind(commandBuffers[i]);
		model->bind(commandBuffers[i]);
		model->draw(commandBuffers[i]);


		vkCmdEndRenderPass(commandBuffers[i]);

		if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer");
		}
	}
}

void App::drawFrame()
{
	uint32_t imageIndex;
	auto result = swapChain.acquireNextImage(&imageIndex);

	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("failed to acquire swap chain image");
	}



	result = swapChain.submitCommandBuffers(&commandBuffers[imageIndex], &imageIndex);

	if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to present swap chain image");
	}
}
