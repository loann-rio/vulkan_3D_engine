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
#include <algorithm>
#include <iterator>

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

	tinygltf::Model loadModel(tinygltf::TinyGLTF& gltfContext, const std::filesystem::path& path) {
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
	}

	size_t gltfIndexToLinearNodeIndex(std::vector<size_t>& nodes, size_t gltfIndex)
	{
		auto it = std::find(nodes.begin(), nodes.end(), gltfIndex);

		if (it != nodes.end()) {
			return std::distance(nodes.begin(), it);
		}
		else {
			throw std::exception("gltf decoder : Could not find gltf node index");
		}
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


	// load file 
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

	// initial scene
	const tinygltf::Scene& scene = gltfModel.scenes[gltfModel.defaultScene > -1 ? gltfModel.defaultScene : 0];

	std::vector<GltfVertex> localVertices;
	std::vector<uint32_t> localIndices;

	std::vector<std::unique_ptr<Node>> nodes;
	std::vector<size_t> nodesGlTFIndices;

	std::vector<size_t> rootNodes;

	for (size_t i = 0; i < scene.nodes.size(); i++) {
		const tinygltf::Node& node = gltfModel.nodes[scene.nodes[i]];
		loadNode(
			/* parent index : */ -1, 
			/* current node */   node, 
			/* node gltf index*/ scene.nodes[i], 
			/* model */          gltfModel, 
			/* targets : */
			localVertices, localIndices, 
			nodes, nodesGlTFIndices, rootNodes);
	}

	decodedModel.vertices    = std::make_unique<GltfVertexData>(std::move(localVertices));
	decodedModel.indices     = std::move(localIndices);
	decodedModel.nodes       = std::move(nodes);
	decodedModel.rootNodes   = std::move(rootNodes);

	decodedModel.materials   = loadMaterials(gltfModel);
	decodedModel.textures    = loadTextures(gltfModel);


	decodedModel.animations  = loadAnimations(gltfModel);

	// convert from gltf indices to linearNode indices
	for (DecodedAnimation anim : decodedModel.animations) {
		for (DecodedAnimationChannel& channel : anim.channels) {
			channel.nodeIndex = 
				gltfIndexToLinearNodeIndex(nodesGlTFIndices, channel.nodeIndex);
		}
	}


	decodedModel.skins       = loadSkins(gltfModel);

	// convert from gltf indices to linearNode indices
	for (DecodedSkin& skin : decodedModel.skins) {
		skin.skeletonRootIndex = gltfIndexToLinearNodeIndex(nodesGlTFIndices, skin.skeletonRootIndex);

		for (size_t& index : skin.jointsIndex) 
			index = gltfIndexToLinearNodeIndex(nodesGlTFIndices, index);
	}
	
	return decodedModel;
}
