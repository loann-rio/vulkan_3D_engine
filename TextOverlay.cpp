#include "TextOverlay.h"


#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <stdexcept>
#include <array>
#include <cassert>


TextOverlay::TextOverlay(Device& device, VkRenderPass renderPass) : device{ device }
{
	createPipelineLayout({ (*DescriptorSetLayout::Builder(device)
		.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.build())
		.getDescriptorSetLayout() });

	createPipeline(renderPass);
}

TextOverlay::~TextOverlay()
{
	vkDestroyPipelineLayout(device.device(), pipelineLayout, nullptr);
}

void TextOverlay::createPipelineLayout(std::vector<VkDescriptorSetLayout> descriptorSetLayout)
{
	VkDescriptorBindingFlags descriptorBindingFlags[] = {
		0,  // For non-dynamic descriptor sets
		VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT // For dynamic descriptor sets
	};

	
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayout.size());
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayout.data();
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;
	pipelineLayoutInfo.pNext = nullptr;

	if (vkCreatePipelineLayout(device.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) !=
		VK_SUCCESS) {
		throw std::runtime_error("fail to create pipeline layout");
	}
}

void TextOverlay::createPipeline(VkRenderPass renderPass)
{
	assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

	PipelineConfigInfo pipelineConfig{};

	Pipeline::defaultPipelineConfigInfo(pipelineConfig);
	Pipeline::enableAlphaBlending(pipelineConfig);


	pipelineConfig.renderPass = renderPass;

	pipelineConfig.pipelineLayout = pipelineLayout;

	std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

	attributeDescriptions.push_back({ 0, 0, VK_FORMAT_R32G32_SFLOAT, 0 });
	attributeDescriptions.push_back({ 1, 0, VK_FORMAT_R32G32_SFLOAT, sizeof(glm::vec2) });

	pipelineConfig.attributeDescription = attributeDescriptions;

	std::vector<VkVertexInputBindingDescription> bindingDescription(1);
	bindingDescription[0].binding = 0;
	bindingDescription[0].stride = 2 * sizeof(glm::vec2);
	bindingDescription[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	pipelineConfig.bindingDescription = bindingDescription;

	

	pipeline = std::make_unique<Pipeline>(
		device,
		"text.vert.spv",
		"text.frag.spv",
		pipelineConfig
	);
}

void TextOverlay::renderText(FrameInfo& frameInfo)
{
	pipeline->bind(frameInfo.commandBuffer);

	vkCmdBindDescriptorSets(
		frameInfo.commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipelineLayout,
		0, 1,
		&descriptorSet[frameInfo.frameIndex],
		0,
		nullptr
	);

	VkBuffer buffers[] = { vertexBuffer->getBuffer() };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(frameInfo.commandBuffer, 0, 1, buffers, offsets);

	vkCmdDraw(frameInfo.commandBuffer, 6 * numLetters, numLetters, 0, 0);

}

void TextOverlay::prepareResources(DescriptorPool& pool)
{
	const uint32_t fontWidth = STB_FONT_consolas_24_latin1_BITMAP_WIDTH;
	const uint32_t fontHeight = STB_FONT_consolas_24_latin1_BITMAP_HEIGHT;

	static unsigned char font24pixels[fontHeight][fontWidth];
	stb_font_consolas_24_latin1(stbFontData, font24pixels, fontHeight);
	VkDeviceSize bufferSize = TEXTOVERLAY_MAX_CHAR_COUNT * 4 * sizeof(glm::vec4);


	// create buffer corresponding to the position in the texture of each letter (max nb of letters)

	vertexBuffer = std::make_unique<Buffer>(
		device,
		bufferSize,
		1,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);


	// create texture
	
	// convert the font24pixels data to RGBA format
	unsigned char* rgbaPixels = new unsigned char[fontWidth * fontHeight * 4];
	for (int y = 0; y < fontHeight; y++) {
		for (int x = 0; x < fontWidth; x++) {
			int index = (y * fontWidth + x) * 4;
			unsigned char pixel = font24pixels[y][x];
			rgbaPixels[index] = pixel;        // R
			rgbaPixels[index + 1] = pixel;    // G
			rgbaPixels[index + 2] = pixel;    // B
			rgbaPixels[index + 3] = 255;      // A
		}
	}
		
	texture = std::make_unique<Texture>(device, rgbaPixels, fontWidth, fontHeight );

	delete[] rgbaPixels;

	// descriptor things

	auto textureSetLayout = DescriptorSetLayout::Builder(device)
		.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.build();

	for (int i = 0; i < descriptorSet.size(); i++)
	{
		auto imageInfo = texture->getImageInfo();
		DescriptorWriter(*textureSetLayout, pool)
			.writeImage(0, &imageInfo)
			.build(descriptorSet[i]);
	}
}

void TextOverlay::addText(std::string text, float x, float y, TextAlign align, uint32_t width, uint32_t height)
{

	const uint32_t firstChar = STB_FONT_consolas_24_latin1_FIRST_CHAR;

	glm::vec4* mapped = (glm::vec4*)(vertexBuffer->getMappedMemory());

	assert(mapped != nullptr);

	float fbW = width;
	float fbH = height;

	const float charW = 1.5f * scale / fbW;
	const float charH = 1.5f * scale / fbH;

	x = (x / fbW * 2.0f) - 1.0f;
	y = (y / fbH * 2.0f) - 1.0f;


	// Calculate text width
	float textWidth = 0;
	for (auto letter : text)
	{
		stb_fontchar* charData = &stbFontData[(uint32_t)letter - firstChar];
		textWidth += charData->advance * charW;
	}

	switch (align)
	{
	case alignRight:
		x -= textWidth;
		break;
	case alignCenter:
		x -= textWidth / 2.0f;
		break;
	case alignLeft:
		break;
	}

	

	// Generate a uv mapped quad per char in the new text
	for (auto letter : text)
	{

		
		stb_fontchar* charData = &stbFontData[(uint32_t)letter - firstChar];
		
		
		float x0 = x + charData->x0 * charW;
		float y0 = y + charData->y0 * charH;
		float x1 = x + charData->x1 * charW;
		float y1 = y + charData->y1 * charH;

		mapped->x = x0;
		mapped->y = y0;
		mapped->z = charData->s0;
		mapped->w = charData->t0;
		mapped++;

		mapped->x = x1;
		mapped->y = y0;
		mapped->z = charData->s1;
		mapped->w = charData->t0;
		mapped++;

		mapped->x = x1;
		mapped->y = y1;
		mapped->z = charData->s1;
		mapped->w = charData->t1;
		mapped++;

		mapped->x = x0;
		mapped->y = y0;
		mapped->z = charData->s0;
		mapped->w = charData->t0;
		mapped++;

		mapped->x = x1;
		mapped->y = y1;
		mapped->z = charData->s1;
		mapped->w = charData->t1;
		mapped++;

		mapped->x = x0;
		mapped->y = y1;
		mapped->z = charData->s0;
		mapped->w = charData->t1;
		mapped++;


		x += charData->advance * charW;

		numLetters++;
	}
}