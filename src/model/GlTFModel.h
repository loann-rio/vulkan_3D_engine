#pragma once

////
// this class use the implementation of GlTF model from sascha willems:
//https://github.com/SaschaWillems/Vulkan-glTF-PBR?tab=readme-ov-file
////

#include <vulkan/vulkan.h>
#include "../base/Device.h"
#include "../base/Buffer.h"
#include "../objects/Texture.h"
#include "../base/Swap_chain.h"
#include "../base/descriptors.h"


#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include <vector>
#include <memory>

#if defined(_WIN32) && defined(ERROR) && defined(TINYGLTF_ENABLE_DRACO) 
#undef ERROR
#pragma message ("ERROR constant already defined, undefining")
#endif

#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_USE_RAPIDJSON_CRTALLOCATOR

#include "../../external/tiny_gltf.h"
#include "../../external/basisu/transcoder/basisu_transcoder.h"

#define MAX_NUM_JOINTS 64u
#define MAX_TEXTURES 128u

struct GltfPushConstant {
	glm::mat4 nodeMatrix{};
	int indexMaterial{ 0 };
};

class GlTFModel
{
	struct Node;

	struct BoundingBox {
		glm::vec3 min{};
		glm::vec3 max{};
		bool valid = false;
		BoundingBox() {};
		BoundingBox(glm::vec3 min, glm::vec3 max) : min{ min }, max{ max } {};
		BoundingBox getAABB(glm::mat4 m);
	};

	struct TextureModel {		
		uint32_t width, height;
		uint32_t mipLevels;
		uint32_t layerCount;
		std::shared_ptr<Texture> texture;

		TextureModel() {}
		TextureModel(uint32_t width, uint32_t height, std::shared_ptr<Texture> texture) : width{ width }, height{ height }, texture{ texture } {} 
		void TextFromglTfImage(Device& device, tinygltf::Image& gltfimage, std::string path = "");
	};

	struct TextureSampler {
		VkFilter magFilter;
		VkFilter minFilter;
		VkSamplerAddressMode addressModeU;
		VkSamplerAddressMode addressModeV;
		VkSamplerAddressMode addressModeW;
	};

	struct Material {

		enum AlphaMode { ALPHAMODE_OPAQUE, ALPHAMODE_MASK, ALPHAMODE_BLEND };
		AlphaMode alphaMode = ALPHAMODE_OPAQUE;

		float alphaCutoff = 1.0f;
		float metallicFactor = 1.0f;
		float roughnessFactor = 1.0f;

		glm::vec4 baseColorFactor = glm::vec4(1.0f); 
		glm::vec4 emissiveFactor  = glm::vec4(0.0f);

		int baseColorTexture{ -1 };
		int metallicRoughnessTexture{ -1 };
		int normalTexture{ -1 };
		int occlusionTexture{ -1 };
		int emissiveTexture{ -1 };

		bool doubleSided = false;

		struct TexCoordSets 
		{
			uint8_t baseColor = 0;
			uint8_t metallicRoughness = 0;
			uint8_t specularGlossiness = 0;
			uint8_t normal = 0;
			uint8_t occlusion = 0;
			uint8_t emissive = 0;

		} texCoordSets;

		struct Extension 
		{
			TextureModel* specularGlossinessTexture;
			TextureModel* diffuseTexture;

			glm::vec4 diffuseFactor  = glm::vec4(1.0f); 
			glm::vec3 specularFactor = glm::vec3(0.0f); 

		} extension;

		struct PbrWorkflows 
		{
			bool metallicRoughness = true;
			bool specularGlossiness = false;

		} pbrWorkflows;

		int index = 0;
		bool unlit = false;
		float emissiveStrength = 1.0f;
	};
	 
	struct Primitive {
		uint32_t firstIndex;
		uint32_t indexCount;
		uint32_t vertexCount;

		int materialIndex = 0;

		bool hasIndices;
		BoundingBox bb;
		Primitive(uint32_t firstIndex, uint32_t indexCount, uint32_t vertexCount, int matIndex);
		void setBoundingBox(glm::vec3 min, glm::vec3 max);
	};

	struct Mesh {
		std::vector<Primitive*> primitives;
		BoundingBox bb;
		BoundingBox aabb;

		std::unique_ptr<Buffer> uniformBuffer;

		struct UniformBlock {
			glm::mat4 matrix;
			glm::mat4 jointMatrix[MAX_NUM_JOINTS]{};
			uint32_t jointcount{ 0 };
		} uniformBlock;

		std::vector<VkDescriptorSet> descriptorSet{ Swap_chain::MAX_FRAMES_IN_FLIGHT };

		Mesh(Device& device, glm::mat4 matrix);
		~Mesh();

		int bufferCreated = 0;
		int descriptorCreated = 0;
		
		void setBoundingBox(glm::vec3 min, glm::vec3 max);
		void createBuffer(bool hasSkin);
		void createDescriptorSet(DescriptorPool& pool, Device& device);
		Device& device;
	};

	struct Skin {
		std::string name;
		Node* skeletonRoot = nullptr;
		std::vector<glm::mat4> inverseBindMatrices;
		std::vector<Node*> joints;
	};

	struct Node {
		Node* parent;
		uint32_t index;

		std::vector<Node*> children;

		glm::mat4 matrix;

		std::string name;

		Mesh* mesh;
		Skin* skin;
		int32_t skinIndex = -1;

		glm::vec3 translation{};
		glm::vec3 scale{ 1.0f };
		glm::quat rotation;

		BoundingBox bvh;
		BoundingBox aabb;

		bool useCachedMatrix{ false };
		glm::mat4 cachedLocalMatrix{ glm::mat4(1.0f) };  
		glm::mat4 cachedMatrix{ glm::mat4(1.0f) }; 
		glm::mat4 localMatrix();
		glm::mat4 getMatrix();

		void createDescriptorSets(DescriptorPool& pool, Device& device);

		void update();
		~Node();
	};

	struct AnimationChannel {
		enum PathType { TRANSLATION, ROTATION, SCALE };
		PathType path;
		Node* node;
		uint32_t samplerIndex;
	};

	struct AnimationSampler {
		enum InterpolationType { LINEAR, STEP, CUBICSPLINE };
		InterpolationType interpolation;
		std::vector<float> inputs;
		std::vector<glm::vec4> outputsVec4;
		std::vector<float> outputs;
		glm::vec4 cubicSplineInterpolation(size_t index, float time, uint32_t stride);
		void translate(size_t index, float time, Node* node);
		void scale(size_t index, float time, Node* node);
		void rotate(size_t index, float time, Node* node);
	};

	struct Animation {
		std::string name;
		std::vector<AnimationSampler> samplers;
		std::vector<AnimationChannel> channels;
		float start = std::numeric_limits<float>::max();
		float end = std::numeric_limits<float>::min();
	};

	struct alignas(16) ShaderMaterial { 
		glm::vec4 baseColorFactor; 
		glm::vec4 emissiveFactor;
		glm::vec4 diffuseFactor;
		glm::vec4 specularFactor;
		float workflow;
		int colorTextureSet;
		int PhysicalDescriptorTextureSet;
		int normalTextureSet;
		int occlusionTextureSet;
		int emissiveTextureSet;

		int baseColorTextureIndex;
		int metallicRoughnessTextureIndex;
		int normalTextureIndex;
		int occlusionTextureIndex;
		int emissiveTextureIndex;

		float metallicFactor;
		float roughnessFactor;
		float alphaMask;
		float alphaMaskCutoff;
		float emissiveStrength;
	};

	public:

	struct ModelGltf {

		struct Vertex {
			glm::vec3 position{};
			glm::vec3 normal{};

			glm::uvec4 joint0; 
			glm::vec4 weight0;

			glm::vec2 uv0{};
			glm::vec2 uv1{};

			glm::vec3 color{};

			static std::vector<VkVertexInputBindingDescription> getBindingDescriptions(bool hasMutipleInstances);
			static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions(bool hasMutipleInstances);
			static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptionsShadow(bool hasMutipleInstances);
		};

		GltfPushConstant push{};

		std::unique_ptr<Buffer> vertexBuffer;
		std::unique_ptr<Buffer> indexBuffer;

		glm::mat4 aabb;

		std::vector<Node*> nodes;
		std::vector<Node*> linearNodes;

		std::vector<Skin*> skins;

		std::vector<TextureModel> textures; 
		std::vector<TextureSampler> textureSamplers;
		

		std::vector<Material> materials;
		std::shared_ptr<Buffer> materialBuffer = nullptr; 

		std::vector<Animation> animations;		

		std::vector<std::string> extensions; 

		std::vector<VkDescriptorSet> descriptorSet{ Swap_chain::MAX_FRAMES_IN_FLIGHT };

		struct Dimensions {
			glm::vec3 min = glm::vec3(FLT_MAX);
			glm::vec3 max = glm::vec3(-FLT_MAX);
		} dimensions;

		struct LoaderInfo {
			uint32_t* indexBuffer;
			Vertex* vertexBuffer;
			size_t vertexCount = 0;
			size_t indexCount = 0;
			size_t indexPos = 0;
			size_t vertexPos = 0;
		};

		ModelGltf(Device& device) : device{ device }{};
		~ModelGltf();

		void loadNode(Node* parent, const tinygltf::Node& node, uint32_t nodeIndex, const tinygltf::Model& model, LoaderInfo& loaderInfo, float globalscale);
		void loadSkins(tinygltf::Model& gltfModel);
		void loadTextures(tinygltf::Model& gltfModel, Device& device);
		void loadTextureSamplers(tinygltf::Model& gltfModel);
		void loadMaterials(tinygltf::Model& gltfModel);
		void loadAnimations(tinygltf::Model& gltfModel);
		bool loadFromFile(std::string filename, float scale = 1.0f);

		VkSamplerAddressMode getVkWrapMode(int32_t wrapMode);
		VkFilter getVkFilterMode(int32_t filterMode);
		void getNodeProps(const tinygltf::Node& node, const tinygltf::Model& model, size_t& vertexCount, size_t& indexCount);
		void getSceneDimensions();
		std::vector<VkDescriptorSet>& getDescriptorSets() { return descriptorSet; }
		static std::vector<DescriptorSetObject> getDescriptorType(); 
		static int getModelType() { return 2; }
		
		void drawNode(Node* node, VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, glm::mat4 modelMatrix, glm::mat4 normalMatrix);
		void drawNodeDepth(Node* node, VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, glm::mat4 modelMatrix, int lightIndex);
		
		void bind(VkCommandBuffer& commandBuffer, Buffer* instancesBuffer);
		void draw(VkCommandBuffer& commandBuffer, VkPipelineLayout& GlTFPipelineLayout, glm::mat4 modelMatrix, glm::mat4 normalMatrix, uint32_t instanceCount); 
		void drawDepth(VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, glm::mat4 modelMatrix, uint32_t cameraIndex, uint32_t instanceCount);

		bool updateAnimation(uint32_t index, float animationTimer);
		
		void createVertexBuffers(LoaderInfo loaderInfo);
		void createIndexBuffers(LoaderInfo loaderInfo);
		void createDescriptorSet(DescriptorPool& pool, Device& device);
		void createMaterialBuffer();

		void calculateBoundingBox(Node* node, Node* parent);

		

		GlTFModel::Node* findNode(Node* parent, uint32_t index);
		GlTFModel::Node* nodeFromIndex(uint32_t index);

		
		Device& device;
	};

	GlTFModel(const GlTFModel&) = delete;
	GlTFModel& operator=(const GlTFModel&) = delete;

	static std::unique_ptr<GlTFModel::ModelGltf> createModelFromFile(Device& device, const std::string& filePath);
	ModelGltf model;
};

