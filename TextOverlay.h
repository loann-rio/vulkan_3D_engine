#pragma once

#define TEXTOVERLAY_MAX_CHAR_COUNT 2048

#include "Pipeline.h"
#include "device.h"
#include "GameObject.h"
#include "Camera.h"
#include "Frame_info.h"
#include "descriptors.h"
#include "Model.h"


#include "external/stb/consolas/stb_font_consolas_24_latin1.inl"


#include <memory>
#include <vector>


class TextOverlay
{
public:
	TextOverlay(Device& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
	~TextOverlay();

	TextOverlay(const TextOverlay&) = delete;
	TextOverlay& operator=(const TextOverlay&) = delete;

	enum TextAlign { alignLeft, alignCenter, alignRight };

	void prepareResources(DescriptorPool& pool);

	void beginTextUpdate() { vertexBuffer->map(VK_WHOLE_SIZE, 0); numLetters = 0; };
	void endTextUpdate() { vertexBuffer->unmap(); mapped = nullptr; };

	void addText(std::string text, float x, float y, TextAlign align, uint32_t width, uint32_t height);

	void renderText(FrameInfo& frameInfo);

	uint32_t numLetters;
	bool visible = true;

private:
	void createPipelineLayout(std::vector<VkDescriptorSetLayout> descriptorSetLayout);
	void createPipeline(VkRenderPass renderPass);

	Device& device;

	std::unique_ptr<Buffer> vertexBuffer;

	std::unique_ptr<Pipeline> pipeline;
	VkPipelineLayout pipelineLayout;

	uint32_t* frameBufferWidth;
	uint32_t* frameBufferHeight;

	float scale = 1.f;
	glm::vec4* mapped = nullptr;
	stb_fontchar stbFontData[STB_FONT_consolas_24_latin1_NUM_CHARS];

	std::unique_ptr<Texture> texture;
	std::vector<VkDescriptorSet> descriptorSet{ Swap_chain::MAX_FRAMES_IN_FLIGHT };

};

