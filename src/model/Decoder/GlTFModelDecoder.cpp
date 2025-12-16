#include "GlTFModelDecoder.h"

#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_USE_RAPIDJSON_CRTALLOCATOR

#include "../../external/tiny_gltf.h"

#include "../../external/basisu/transcoder/basisu_transcoder.h"

#include <exception>
#include <iostream>
#include <vector>
#include <cstdint>
#include <memory>

#include "../Vertex/GltfVertexData.h"



namespace {
	bool isBinaryFile(const std::filesystem::path& path) {
		auto ext = moDecoder::getExtension(path.string());
		return ext == "glb";
	}

	bool loadImageDataFunc(tinygltf::Image* image, const int imageIndex, std::string* error, std::string* warning, int req_width, int req_height, const unsigned char* bytes, int size, void* userData)
	{
		auto ext = moDecoder::getExtension(image->uri);
		if (ext == "ktx" || ext == "ktx2") return true;

		return tinygltf::LoadImageData(image, imageIndex, error, warning, req_width, req_height, bytes, size, userData);
	}

	void loadGltfMaterials(tinygltf::Model& gltfModel) 
	{
		/*loadTextureSamplers(gltfModel);
		loadTextures(gltfModel, device);

		loadMaterials(gltfModel);
		createMaterialBuffer();*/

	}

	void loadSkins(tinygltf::Model& gltfModel)
	{
		//loadSkins(gltfModel);

		//for (auto node : linearNodes) {
		//	// Assign skins
		//	if (node->skinIndex > -1) {
		//		node->skin = skins[node->skinIndex];
		//	}

		//	// Initial pose
		//	if (node->mesh) {
		//		node->update();
		//	}
		//}
	}
	
}

bool GlTFModelDecoder::canDecode(const std::filesystem::path& path) const
{
    auto ext = moDecoder::getExtension(path.string());
    return (ext == "gltf" || ext == "glb");
}

DecodedModel GlTFModelDecoder::decode(const std::filesystem::path& path) const
{
	DecodedModel decodedModel;

	tinygltf::TinyGLTF gltfContext;
	gltfContext.SetImageLoader(loadImageDataFunc, nullptr);

	std::string error;
	std::string warning;

	tinygltf::Model gltfModel;
	bool fileLoaded;
	if (isBinaryFile(path))
		fileLoaded = gltfContext.LoadBinaryFromFile(&gltfModel, &error, &warning, path.string().c_str());
	else
		fileLoaded = gltfContext.LoadASCIIFromFile(&gltfModel, &error, &warning, path.string().c_str());

	if (!fileLoaded) {
		std::cerr << "Could not load gltf file: " << error << std::endl;
		throw std::exception("gltf decoder : Could not load gltf file");
	}

	loadGltfMaterials(gltfModel);

	const tinygltf::Scene& scene = gltfModel.scenes[gltfModel.defaultScene > -1 ? gltfModel.defaultScene : 0];

	std::vector<GltfVertex> localVertices;
	std::vector<uint32_t> localIndices;
	std::vector<std::unique_ptr<Node>> nodes;
	for (size_t i = 0; i < scene.nodes.size(); i++) {
		const tinygltf::Node node = gltfModel.nodes[scene.nodes[i]];
		loadNode(nullptr, node, scene.nodes[i], gltfModel, localVertices, localIndices, nodes);
	}

	decodedModel.vertices = std::make_unique<GltfVertexData>(std::move(localVertices));
	decodedModel.indices  = std::move(localIndices);

	/*if (gltfModel.animations.size() > 0) {
		loadAnimations(gltfModel);
	}

	loadSkins(gltfModel);
	*/


	return decodedModel;

}
