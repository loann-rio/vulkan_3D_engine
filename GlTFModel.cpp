#define TINYGLTF_IMPLEMENTATION

#define STBI_MSC_SECURE_CRT


#include <glm/ext/quaternion_common.hpp>
#include <iostream>

#include "GlTFModel.h"

bool loadImageDataFunc(tinygltf::Image* image, const int imageIndex, std::string* error, std::string* warning, int req_width, int req_height, const unsigned char* bytes, int size, void* userData)
{
	// KTX files will be handled by our own code
	if (image->uri.find_last_of(".") != std::string::npos) {
		if (image->uri.substr(image->uri.find_last_of(".") + 1) == "ktx2") {
			return true;
		}
	}

	return tinygltf::LoadImageData(image, imageIndex, error, warning, req_width, req_height, bytes, size, userData);
}

void GlTFModel::ModelGltf::destroy(VkDevice device)
{
	textures.resize(0);

	for (auto node : nodes) {
		delete node;
	}

	for (auto node : linearNodes) {
		delete node;
	}

	materials.resize(0);
	animations.resize(0);
	nodes.resize(0);
	linearNodes.resize(0);
	extensions.resize(0);

	for (auto skin : skins) {
		delete skin;
	}

	skins.resize(0);

}

void GlTFModel::ModelGltf::loadNode(Node* parent, const tinygltf::Node& node, uint32_t nodeIndex, const tinygltf::Model& model, LoaderInfo& loaderInfo, float globalscale)
{
	Node* newNode = new Node{};
	newNode->index = nodeIndex;
	newNode->parent = parent;
	newNode->name = node.name;
	newNode->skinIndex = node.skin;
	newNode->matrix = glm::mat4(1.0f);

	// Generate local node matrix
	glm::vec3 translation = glm::vec3(0.0f);
	if (node.translation.size() == 3) {
		translation = glm::make_vec3(node.translation.data());
		newNode->translation = translation;
	}

	glm::mat4 rotation = glm::mat4(1.0f);
	if (node.rotation.size() == 4) {
		glm::quat q = glm::make_quat(node.rotation.data());
		newNode->rotation = glm::mat4(q);
	}

	glm::vec3 scale = glm::vec3(1.0f);
	if (node.scale.size() == 3) {
		scale = glm::make_vec3(node.scale.data());
		newNode->scale = scale;
	}

	if (node.matrix.size() == 16) {
		newNode->matrix = glm::make_mat4x4(node.matrix.data());
	};

	// Node with children
	if (node.children.size() > 0) {
		for (size_t i = 0; i < node.children.size(); i++) {
			loadNode(newNode, model.nodes[node.children[i]], node.children[i], model, loaderInfo, globalscale);
		}
	}

	// Node contains mesh data
	if (node.mesh > -1) 
	{
		const tinygltf::Mesh mesh = model.meshes[node.mesh];

		Mesh* newMesh = new Mesh(device, newNode->matrix);

		for (size_t j = 0; j < mesh.primitives.size(); j++) 
		{
			const tinygltf::Primitive& primitive = mesh.primitives[j];

			uint32_t vertexStart = static_cast<uint32_t>(loaderInfo.vertexPos);
			uint32_t indexStart = static_cast<uint32_t>(loaderInfo.indexPos);

			uint32_t indexCount = 0;
			uint32_t vertexCount = 0;

			glm::vec3 posMin{};
			glm::vec3 posMax{};

			bool hasSkin = false;
			bool hasIndices = primitive.indices > -1;

			// Vertices
			{
				const float* bufferPos = nullptr;
				const float* bufferNormals = nullptr;
				const float* bufferTexCoordSet0 = nullptr;
				const float* bufferTexCoordSet1 = nullptr;
				const float* bufferColorSet0 = nullptr;
				const void* bufferJoints = nullptr;
				const float* bufferWeights = nullptr;

				int posByteStride;
				int normByteStride;
				int uv0ByteStride;
				int uv1ByteStride;
				int color0ByteStride;
				int jointByteStride;
				int weightByteStride;

				int jointComponentType;

				// Position attribute is required
				assert(primitive.attributes.find("POSITION") != primitive.attributes.end() && "Position attribute is required");

				const tinygltf::Accessor& posAccessor = model.accessors[primitive.attributes.find("POSITION")->second];
				const tinygltf::BufferView& posView = model.bufferViews[posAccessor.bufferView];

				bufferPos = reinterpret_cast<const float*>(&(model.buffers[posView.buffer].data[posAccessor.byteOffset + posView.byteOffset]));
				
				posMin = glm::vec3(posAccessor.minValues[0], posAccessor.minValues[1], posAccessor.minValues[2]);
				posMax = glm::vec3(posAccessor.maxValues[0], posAccessor.maxValues[1], posAccessor.maxValues[2]);
				
				vertexCount = static_cast<uint32_t>(posAccessor.count);
				posByteStride = posAccessor.ByteStride(posView) ? (posAccessor.ByteStride(posView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);

				if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) 
				{
					const tinygltf::Accessor& normAccessor = model.accessors[primitive.attributes.find("NORMAL")->second];
					const tinygltf::BufferView& normView = model.bufferViews[normAccessor.bufferView];
					bufferNormals = reinterpret_cast<const float*>(&(model.buffers[normView.buffer].data[normAccessor.byteOffset + normView.byteOffset]));
					normByteStride = normAccessor.ByteStride(normView) ? (normAccessor.ByteStride(normView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);
				}

				// UVs
				if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
					const tinygltf::Accessor& uvAccessor = model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
					const tinygltf::BufferView& uvView = model.bufferViews[uvAccessor.bufferView];
					bufferTexCoordSet0 = reinterpret_cast<const float*>(&(model.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
					uv0ByteStride = uvAccessor.ByteStride(uvView) ? (uvAccessor.ByteStride(uvView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC2);
				}
				if (primitive.attributes.find("TEXCOORD_1") != primitive.attributes.end()) {
					const tinygltf::Accessor& uvAccessor = model.accessors[primitive.attributes.find("TEXCOORD_1")->second];
					const tinygltf::BufferView& uvView = model.bufferViews[uvAccessor.bufferView];
					bufferTexCoordSet1 = reinterpret_cast<const float*>(&(model.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
					uv1ByteStride = uvAccessor.ByteStride(uvView) ? (uvAccessor.ByteStride(uvView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC2);
				}

				// Vertex colors
				if (primitive.attributes.find("COLOR_0") != primitive.attributes.end()) {
					const tinygltf::Accessor& accessor = model.accessors[primitive.attributes.find("COLOR_0")->second];
					const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
					bufferColorSet0 = reinterpret_cast<const float*>(&(model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
					color0ByteStride = accessor.ByteStride(view) ? (accessor.ByteStride(view) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);
				}

				// Skinning
				// Joints
				if (primitive.attributes.find("JOINTS_0") != primitive.attributes.end()) {
					const tinygltf::Accessor& jointAccessor = model.accessors[primitive.attributes.find("JOINTS_0")->second];
					const tinygltf::BufferView& jointView = model.bufferViews[jointAccessor.bufferView];
					bufferJoints = &(model.buffers[jointView.buffer].data[jointAccessor.byteOffset + jointView.byteOffset]);
					jointComponentType = jointAccessor.componentType;
					jointByteStride = jointAccessor.ByteStride(jointView) ? (jointAccessor.ByteStride(jointView) / tinygltf::GetComponentSizeInBytes(jointComponentType)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC4);
				}

				if (primitive.attributes.find("WEIGHTS_0") != primitive.attributes.end()) {
					const tinygltf::Accessor& weightAccessor = model.accessors[primitive.attributes.find("WEIGHTS_0")->second];
					const tinygltf::BufferView& weightView = model.bufferViews[weightAccessor.bufferView];
					bufferWeights = reinterpret_cast<const float*>(&(model.buffers[weightView.buffer].data[weightAccessor.byteOffset + weightView.byteOffset]));
					weightByteStride = weightAccessor.ByteStride(weightView) ? (weightAccessor.ByteStride(weightView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC4);
				}

				hasSkin = (bufferJoints && bufferWeights);

				for (size_t v = 0; v < posAccessor.count; v++) 
				{
					Vertex& vert = loaderInfo.vertexBuffer[loaderInfo.vertexPos];
					vert.position = glm::vec4(glm::make_vec3(&bufferPos[v * posByteStride]), 1.0f);
					vert.normal = glm::normalize(glm::vec3(bufferNormals ? glm::make_vec3(&bufferNormals[v * normByteStride]) : glm::vec3(0.0f)));
					vert.uv0 = bufferTexCoordSet0 ? glm::make_vec2(&bufferTexCoordSet0[v * uv0ByteStride]) : glm::vec3(0.0f);
					vert.uv1 = bufferTexCoordSet1 ? glm::make_vec2(&bufferTexCoordSet1[v * uv1ByteStride]) : glm::vec3(0.0f);
					vert.color = bufferColorSet0  ? glm::make_vec4(&bufferColorSet0[v * color0ByteStride]) : glm::vec4(1.0f);

					if (hasSkin)
					{
						switch (jointComponentType) {
						case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
							const uint16_t* buf = static_cast<const uint16_t*>(bufferJoints);
							vert.joint0 = glm::uvec4(glm::make_vec4(&buf[v * jointByteStride]));
							break;
						}
						case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
							const uint8_t* buf = static_cast<const uint8_t*>(bufferJoints);
							vert.joint0 = glm::vec4(glm::make_vec4(&buf[v * jointByteStride]));
							break;
						}
						default:
							// Not supported by spec
							std::cerr << "Joint component type " << jointComponentType << " not supported!" << std::endl;
							break;
						}
					}
					else {
						vert.joint0 = glm::vec4(0.0f);
					}

					vert.weight0 = hasSkin ? glm::make_vec4(&bufferWeights[v * weightByteStride]) : glm::vec4(0.0f);
					newMesh->createBuffer(hasSkin);

					// Fix for all zero weights
					if (glm::length(vert.weight0) == 0.0f) {
						vert.weight0 = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
					}

					loaderInfo.vertexPos++;
				}
			}

			// Indices
			if (hasIndices)
			{
				const tinygltf::Accessor& accessor = model.accessors[primitive.indices > -1 ? primitive.indices : 0];
				const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
				const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

				indexCount = static_cast<uint32_t>(accessor.count);
				const void* dataPtr = &(buffer.data[accessor.byteOffset + bufferView.byteOffset]);

				switch (accessor.componentType) {
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
					const uint32_t* buf = static_cast<const uint32_t*>(dataPtr);
					for (size_t index = 0; index < accessor.count; index++) {
						loaderInfo.indexBuffer[loaderInfo.indexPos] = buf[index] + vertexStart;
						loaderInfo.indexPos++;
					}
					break;
				}
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
					const uint16_t* buf = static_cast<const uint16_t*>(dataPtr);
					for (size_t index = 0; index < accessor.count; index++) {
						loaderInfo.indexBuffer[loaderInfo.indexPos] = buf[index] + vertexStart;
						loaderInfo.indexPos++;
					}
					break;
				}
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
					const uint8_t* buf = static_cast<const uint8_t*>(dataPtr);
					for (size_t index = 0; index < accessor.count; index++) {
						loaderInfo.indexBuffer[loaderInfo.indexPos] = buf[index] + vertexStart;
						loaderInfo.indexPos++;
					}
					break;
				}
				default:
					std::cerr << "Index component type " << accessor.componentType << " not supported!" << std::endl;
					return;
				}
			}
			
			
			Primitive* newPrimitive = new Primitive(indexStart, indexCount, vertexCount, primitive.material > -1 ? materials[primitive.material] : materials.back());
			newPrimitive->setBoundingBox(posMin, posMax);
			newMesh->primitives.push_back(newPrimitive);
		}

		// Mesh BB from BBs of primitives
		for (auto p : newMesh->primitives) {
			if (p->bb.valid && !newMesh->bb.valid) {
				newMesh->bb = p->bb;
				newMesh->bb.valid = true;
			}
			newMesh->bb.min = glm::min(newMesh->bb.min, p->bb.min);
			newMesh->bb.max = glm::max(newMesh->bb.max, p->bb.max);
		}

		newNode->mesh = newMesh;
	}

	if (parent) {
		parent->children.push_back(newNode);
	}
	else {
		nodes.push_back(newNode);
	}

	linearNodes.push_back(newNode);
}


void GlTFModel::ModelGltf::getNodeProps(const tinygltf::Node& node, const tinygltf::Model& model, size_t& vertexCount, size_t& indexCount)
{
	if (node.children.size() > 0) {
		for (size_t i = 0; i < node.children.size(); i++) {
			getNodeProps(model.nodes[node.children[i]], model, vertexCount, indexCount);
		}
	}

	if (node.mesh > -1) {
		const tinygltf::Mesh mesh = model.meshes[node.mesh];
		
		for (size_t i = 0; i < mesh.primitives.size(); i++) {
			auto primitive = mesh.primitives[i];

			vertexCount += model.accessors[primitive.attributes.find("POSITION")->second].count;
			if (primitive.indices > -1) {
				indexCount += model.accessors[primitive.indices].count;
			}
		}
	}
}

void GlTFModel::ModelGltf::loadSkins(tinygltf::Model& gltfModel)
{
	for (tinygltf::Skin& source : gltfModel.skins) {
		Skin* newSkin = new Skin{};
		newSkin->name = source.name; 

		// Find skeleton root node
		if (source.skeleton > -1) {
			newSkin->skeletonRoot = nodeFromIndex(source.skeleton);
		}

		// Find joint nodes
		for (int jointIndex : source.joints) {
			Node* node = nodeFromIndex(jointIndex);
			if (node) {
				newSkin->joints.push_back(nodeFromIndex(jointIndex));
			}
		}

		// Get inverse bind matrices from buffer
		if (source.inverseBindMatrices > -1) {
			const tinygltf::Accessor& accessor = gltfModel.accessors[source.inverseBindMatrices];
			const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
			const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];
			newSkin->inverseBindMatrices.resize(accessor.count);
			memcpy(newSkin->inverseBindMatrices.data(), &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(glm::mat4));
		}

		skins.push_back(newSkin);
	}

}

VkSamplerAddressMode GlTFModel::ModelGltf::getVkWrapMode(int32_t wrapMode)
{
	switch (wrapMode) {
	case -1:
	case 10497:
		return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	case 33071:
		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	case 33648:
		return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	}

	std::cerr << "Unknown wrap mode for getVkWrapMode: " << wrapMode << std::endl;
	return VK_SAMPLER_ADDRESS_MODE_REPEAT;

}

VkFilter GlTFModel::ModelGltf::getVkFilterMode(int32_t filterMode)
{
	switch (filterMode) {
	case -1:
	case 9728:
		return VK_FILTER_NEAREST;
	case 9729:
		return VK_FILTER_LINEAR;
	case 9984:
		return VK_FILTER_NEAREST;
	case 9985:
		return VK_FILTER_NEAREST;
	case 9986:
		return VK_FILTER_LINEAR;
	case 9987:
		return VK_FILTER_LINEAR;
	}

	std::cerr << "Unknown filter mode for getVkFilterMode: " << filterMode << std::endl;
	return VK_FILTER_NEAREST;

}

void GlTFModel::ModelGltf::loadTextures(tinygltf::Model& gltfModel, Device& device, VkQueue transferQueue)
{
	for (tinygltf::Texture& tex : gltfModel.textures) 
	{
		int source = tex.source;
		// If this texture uses the KHR_texture_basisu, we need to get the source index from the extension structure
		if (tex.extensions.find("KHR_texture_basisu") != tex.extensions.end()) 
		{
			auto ext = tex.extensions.find("KHR_texture_basisu");
			auto value = ext->second.Get("source");
			source = value.Get<int>();
		}

		tinygltf::Image image = gltfModel.images[source];

		TextureSampler textureSampler;

		if (tex.sampler == -1) {
			// No sampler specified, use a default one
			textureSampler.magFilter = VK_FILTER_LINEAR;
			textureSampler.minFilter = VK_FILTER_LINEAR;
			textureSampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			textureSampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			textureSampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		}
		else {
			textureSampler = textureSamplers[tex.sampler];
		}

		TextureModel texture;
		texture.TextFromglTfImage(device, image);
		textures.push_back(texture);
	}
}

void GlTFModel::ModelGltf::loadTextureSamplers(tinygltf::Model& gltfModel)
{
	for (tinygltf::Sampler smpl : gltfModel.samplers) 
	{
		TextureSampler sampler{};
		sampler.minFilter = getVkFilterMode(smpl.minFilter);
		sampler.magFilter = getVkFilterMode(smpl.magFilter);
		sampler.addressModeU = getVkWrapMode(smpl.wrapS);
		sampler.addressModeV = getVkWrapMode(smpl.wrapT);
		sampler.addressModeW = sampler.addressModeV;
		textureSamplers.push_back(sampler);
	}

}

void GlTFModel::ModelGltf::loadMaterials(tinygltf::Model& gltfModel)
{
	Texture nullTexture = Texture{ device,  "textures/nullTexture.png" };

	for (tinygltf::Material& mat : gltfModel.materials) {
		Material material{};
		material.doubleSided = mat.doubleSided;
		if (mat.values.find("baseColorTexture") != mat.values.end()) {
			material.baseColorTexture = std::make_shared<TextureModel>(textures[mat.values["baseColorTexture"].TextureIndex()]);
			material.texCoordSets.baseColor = mat.values["baseColorTexture"].TextureTexCoord();
		}
		else {
			material.baseColorTexture = std::make_shared<TextureModel>( 1, 1, nullTexture );
		}

		if (mat.values.find("metallicRoughnessTexture") != mat.values.end()) {
			material.metallicRoughnessTexture = std::make_shared<TextureModel>(textures[mat.values["metallicRoughnessTexture"].TextureIndex()]);
			material.texCoordSets.metallicRoughness = mat.values["metallicRoughnessTexture"].TextureTexCoord();
		}
		else {
			material.baseColorTexture = std::make_shared<TextureModel>( 1, 1, nullTexture );
		}

		if (mat.values.find("roughnessFactor") != mat.values.end()) {
			material.roughnessFactor = static_cast<float>(mat.values["roughnessFactor"].Factor());
		}
		if (mat.values.find("metallicFactor") != mat.values.end()) {
			material.metallicFactor = static_cast<float>(mat.values["metallicFactor"].Factor());
		}
		if (mat.values.find("baseColorFactor") != mat.values.end()) {
			material.baseColorFactor = glm::make_vec4(mat.values["baseColorFactor"].ColorFactor().data());
		}

		if (mat.additionalValues.find("normalTexture") != mat.additionalValues.end()) {
			material.normalTexture = std::make_shared<TextureModel>(textures[mat.additionalValues["normalTexture"].TextureIndex()]);
			material.texCoordSets.normal = mat.additionalValues["normalTexture"].TextureTexCoord();
		}
		else {
			material.baseColorTexture = std::make_shared<TextureModel>(1, 1, nullTexture );
		}

		if (mat.additionalValues.find("emissiveTexture") != mat.additionalValues.end()) {
			material.emissiveTexture = std::make_shared<TextureModel>(textures[mat.additionalValues["emissiveTexture"].TextureIndex()]);
			material.texCoordSets.emissive = mat.additionalValues["emissiveTexture"].TextureTexCoord();
		}
		else {
			material.baseColorTexture = std::make_shared<TextureModel>(1, 1, nullTexture );
		}

		if (mat.additionalValues.find("occlusionTexture") != mat.additionalValues.end()) {
			material.occlusionTexture = std::make_shared<TextureModel>(textures[mat.additionalValues["occlusionTexture"].TextureIndex()]);
			material.texCoordSets.occlusion = mat.additionalValues["occlusionTexture"].TextureTexCoord();
		}
		else {
			material.baseColorTexture = std::make_shared<TextureModel>(1, 1, nullTexture );
		}

		if (mat.additionalValues.find("alphaMode") != mat.additionalValues.end()) {
			tinygltf::Parameter param = mat.additionalValues["alphaMode"];
			if (param.string_value == "BLEND") {
				material.alphaMode = Material::ALPHAMODE_BLEND;
			}
			if (param.string_value == "MASK") {
				material.alphaCutoff = 0.5f;
				material.alphaMode = Material::ALPHAMODE_MASK;
			}
		}
		if (mat.additionalValues.find("alphaCutoff") != mat.additionalValues.end()) {
			material.alphaCutoff = static_cast<float>(mat.additionalValues["alphaCutoff"].Factor());
		}
		if (mat.additionalValues.find("emissiveFactor") != mat.additionalValues.end()) {
			material.emissiveFactor = glm::vec4(glm::make_vec3(mat.additionalValues["emissiveFactor"].ColorFactor().data()), 1.0);
		}

		// Extensions
		if (mat.extensions.find("KHR_materials_pbrSpecularGlossiness") != mat.extensions.end()) {
			auto ext = mat.extensions.find("KHR_materials_pbrSpecularGlossiness");
			if (ext->second.Has("specularGlossinessTexture")) {
				auto index = ext->second.Get("specularGlossinessTexture").Get("index");
				material.extension.specularGlossinessTexture = &textures[index.Get<int>()];
				auto texCoordSet = ext->second.Get("specularGlossinessTexture").Get("texCoord");
				material.texCoordSets.specularGlossiness = texCoordSet.Get<int>();
				material.pbrWorkflows.specularGlossiness = true;
				material.pbrWorkflows.metallicRoughness = false;
			}
			if (ext->second.Has("diffuseTexture")) {
				auto index = ext->second.Get("diffuseTexture").Get("index");
				material.extension.diffuseTexture = &textures[index.Get<int>()];
			}
			if (ext->second.Has("diffuseFactor")) {
				auto factor = ext->second.Get("diffuseFactor");
				for (uint32_t i = 0; i < factor.ArrayLen(); i++) {
					auto val = factor.Get(i);
					material.extension.diffuseFactor[i] = val.IsNumber() ? (float)val.Get<double>() : (float)val.Get<int>();
				}
			}
			if (ext->second.Has("specularFactor")) {
				auto factor = ext->second.Get("specularFactor");
				for (uint32_t i = 0; i < factor.ArrayLen(); i++) {
					auto val = factor.Get(i);
					material.extension.specularFactor[i] = val.IsNumber() ? (float)val.Get<double>() : (float)val.Get<int>();
				}
			}
		}

		if (mat.extensions.find("KHR_materials_unlit") != mat.extensions.end()) {
			material.unlit = true;
		}

		if (mat.extensions.find("KHR_materials_emissive_strength") != mat.extensions.end()) {
			auto ext = mat.extensions.find("KHR_materials_emissive_strength");
			if (ext->second.Has("emissiveStrength")) {
				auto value = ext->second.Get("emissiveStrength");
				material.emissiveStrength = (float)value.Get<double>();
			}
		}

		material.index = static_cast<uint32_t>(materials.size());
		materials.push_back(material);
	}
	// Push a default material at the end of the list for meshes with no material assigned
	materials.push_back(Material());
	textures.resize(0);
}

void GlTFModel::ModelGltf::loadAnimations(tinygltf::Model& gltfModel)
{
	for (tinygltf::Animation& anim : gltfModel.animations) 
	{
		Animation animation{};
		animation.name = anim.name;

		if (anim.name.empty()) {
			animation.name = std::to_string(animations.size());
		}

		// Samplers
		for (auto& samp : anim.samplers) {
			AnimationSampler sampler{};

			if (samp.interpolation == "LINEAR") {
				sampler.interpolation = AnimationSampler::InterpolationType::LINEAR;
			}
			if (samp.interpolation == "STEP") {
				sampler.interpolation = AnimationSampler::InterpolationType::STEP;
			}
			if (samp.interpolation == "CUBICSPLINE") {
				sampler.interpolation = AnimationSampler::InterpolationType::CUBICSPLINE;
			}

			// Read sampler input time values
			{
				const tinygltf::Accessor& accessor = gltfModel.accessors[samp.input];
				const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
				const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];

				assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

				const void* dataPtr = &buffer.data[accessor.byteOffset + bufferView.byteOffset];
				const float* buf = static_cast<const float*>(dataPtr);

				for (size_t index = 0; index < accessor.count; index++) {
					sampler.inputs.push_back(buf[index]);
				}

				for (auto input : sampler.inputs) {
					if (input < animation.start) {
						animation.start = input;
					};
					if (input > animation.end) {
						animation.end = input;
					}
				}
			}

			// Read sampler output T/R/S values 
			{
				const tinygltf::Accessor& accessor = gltfModel.accessors[samp.output];
				const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
				const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];

				assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

				const void* dataPtr = &buffer.data[accessor.byteOffset + bufferView.byteOffset];

				switch (accessor.type) {
					case TINYGLTF_TYPE_VEC3: {
						const glm::vec3* buf = static_cast<const glm::vec3*>(dataPtr);
						for (size_t index = 0; index < accessor.count; index++) {
							sampler.outputsVec4.push_back(glm::vec4(buf[index], 0.0f));
							sampler.outputs.push_back(buf[index][0]);
							sampler.outputs.push_back(buf[index][1]);
							sampler.outputs.push_back(buf[index][2]);
						}
						break;
					}
					case TINYGLTF_TYPE_VEC4: {
						const glm::vec4* buf = static_cast<const glm::vec4*>(dataPtr);
						for (size_t index = 0; index < accessor.count; index++) {
							sampler.outputsVec4.push_back(buf[index]);
							sampler.outputs.push_back(buf[index][0]);
							sampler.outputs.push_back(buf[index][1]);
							sampler.outputs.push_back(buf[index][2]);
							sampler.outputs.push_back(buf[index][3]);
						}
						break;
					}
					default: {
						std::cout << "unknown type" << std::endl;
						break;
					}
				}
			}

			animation.samplers.push_back(sampler);
		}

		// Channels
		for (auto& source : anim.channels) {
			AnimationChannel channel{};

			if (source.target_path == "rotation") {
				channel.path = AnimationChannel::PathType::ROTATION;
			}
			if (source.target_path == "translation") {
				channel.path = AnimationChannel::PathType::TRANSLATION;
			}
			if (source.target_path == "scale") {
				channel.path = AnimationChannel::PathType::SCALE;
			}
			if (source.target_path == "weights") {
				std::cout << "weights not yet supported, skipping channel" << std::endl;
				continue;
			}

			channel.samplerIndex = source.sampler;
			channel.node = nodeFromIndex(source.target_node);

			if (!channel.node) {
				continue;
			}

			animation.channels.push_back(channel);
		}

		animations.push_back(animation);
	}
}

void GlTFModel::ModelGltf::loadFromFile(std::string filename, VkQueue transferQueue, float scale)
{

	basist::basisu_transcoder_init();

	tinygltf::Model gltfModel;
	tinygltf::TinyGLTF gltfContext;

	std::string error;
	std::string warning;


	bool binary = false;
	size_t extpos = filename.rfind('.', filename.length());
	if (extpos != std::string::npos) {
		binary = (filename.substr(extpos + 1, filename.length() - extpos) == "glb");
	}

	size_t pos = filename.find_last_of('/');
	if (pos == std::string::npos) {
		pos = filename.find_last_of('\\');
	}

	std::string filePath = filename.substr(0, pos);

	// @todo
	gltfContext.SetImageLoader(loadImageDataFunc, nullptr);

	bool fileLoaded = binary ? gltfContext.LoadBinaryFromFile(&gltfModel, &error, &warning, filename.c_str()) : gltfContext.LoadASCIIFromFile(&gltfModel, &error, &warning, filename.c_str());

	LoaderInfo loaderInfo{};

	if (fileLoaded) {
		extensions = gltfModel.extensionsUsed;

		for (auto& extension : extensions) {
			// If this model uses basis universal compressed textures, we need to transcode them
			// So we need to initialize that transcoder once
			if (extension == "KHR_texture_basisu") {
				std::cout << "Model uses KHR_texture_basisu, initializing basisu transcoder\n";
				basist::basisu_transcoder_init();
			}
		}

		/////// load textures

		loadTextureSamplers(gltfModel);
		loadTextures(gltfModel, device, transferQueue);
		loadMaterials(gltfModel);

		const tinygltf::Scene& scene = gltfModel.scenes[gltfModel.defaultScene > -1 ? gltfModel.defaultScene : 0];


		////// Get vertex and index buffer sizes up-front
		for (size_t i = 0; i < scene.nodes.size(); i++) {
			getNodeProps(gltfModel.nodes[scene.nodes[i]], gltfModel, loaderInfo.vertexCount, loaderInfo.indexCount);
		}

		loaderInfo.vertexBuffer = new Vertex[loaderInfo.vertexCount];
		loaderInfo.indexBuffer = new uint32_t[loaderInfo.indexCount];

		/////// load nodes
		// TODO: scene handling with no default scene
		for (size_t i = 0; i < scene.nodes.size(); i++) {
			const tinygltf::Node node = gltfModel.nodes[scene.nodes[i]];
			loadNode(nullptr, node, scene.nodes[i], gltfModel, loaderInfo, scale);
		}

		/////// load animations
		if (gltfModel.animations.size() > 0) {
			loadAnimations(gltfModel);
		}

		/////// load skins
		loadSkins(gltfModel);

		for (auto node : linearNodes) {
			// Assign skins
			if (node->skinIndex > -1) {
				node->skin = skins[node->skinIndex];
			}
			// Initial pose
			if (node->mesh) {
				node->update();
			}
		}

	}
	else {
		// TODO: throw
		std::cerr << "Could not load gltf file: " << error << std::endl;
		return;
	}

	createVertexBuffers(loaderInfo);
	createIndexBuffers(loaderInfo);

	delete[] loaderInfo.vertexBuffer;
	delete[] loaderInfo.indexBuffer;

	getSceneDimensions();
}

void GlTFModel::ModelGltf::drawNode(Node* node, VkCommandBuffer commandBuffer, VkPipelineLayout& GlTFPipelineLayout)
{	
	if (node->mesh) {

		for (Primitive* primitive : node->mesh->primitives) {
			vkCmdDrawIndexed(commandBuffer, primitive->indexCount, 1, primitive->firstIndex, 0, 0);
		}
	}

	for (auto& child : node->children) {
		drawNode(child, commandBuffer, GlTFPipelineLayout);
	}

}

void GlTFModel::ModelGltf::draw(VkCommandBuffer commandBuffer, VkPipelineLayout& GlTFPipelineLayout)
{
	for (auto& node : nodes) {
		drawNode(node, commandBuffer, GlTFPipelineLayout);
	}
}

void GlTFModel::ModelGltf::calculateBoundingBox(Node* node, Node* parent)
{
	BoundingBox parentBvh = parent ? parent->bvh : BoundingBox(dimensions.min, dimensions.max);

	if (node->mesh) {
		if (node->mesh->bb.valid) {
			node->aabb = node->mesh->bb.getAABB(node->getMatrix());
			if (node->children.size() == 0) {
				node->bvh.min = node->aabb.min;
				node->bvh.max = node->aabb.max;
				node->bvh.valid = true;
			}
		}
	}

	parentBvh.min = glm::min(parentBvh.min, node->bvh.min);
	parentBvh.max = glm::min(parentBvh.max, node->bvh.max);

	for (auto& child : node->children) {
		calculateBoundingBox(child, node);
	}

}

void GlTFModel::ModelGltf::getSceneDimensions()
{
	// Calculate binary volume hierarchy for all nodes in the scene
	for (auto node : linearNodes) {
		calculateBoundingBox(node, nullptr);
	}

	dimensions.min = glm::vec3(FLT_MAX);
	dimensions.max = glm::vec3(-FLT_MAX);

	for (auto node : linearNodes) {
		if (node->bvh.valid) {
			dimensions.min = glm::min(dimensions.min, node->bvh.min);
			dimensions.max = glm::max(dimensions.max, node->bvh.max);
		}
	}

	// Calculate scene aabb
	aabb = glm::scale(glm::mat4(1.0f), glm::vec3(dimensions.max[0] - dimensions.min[0], dimensions.max[1] - dimensions.min[1], dimensions.max[2] - dimensions.min[2]));
	aabb[3][0] = dimensions.min[0];
	aabb[3][1] = dimensions.min[1];
	aabb[3][2] = dimensions.min[2];
}

void GlTFModel::ModelGltf::createVertexBuffers(LoaderInfo loaderInfo)
{

	assert(loaderInfo.vertexCount >= 3 && "Vertex count must be at least 3");

	uint32_t vertexSize = sizeof(Vertex);

	VkDeviceSize bufferSize = vertexSize * loaderInfo.vertexCount;
	
	std::cout << vertexSize << "\n";

	Buffer stagingBuffer{
		device,
		vertexSize,
		static_cast<uint32_t>(loaderInfo.vertexCount),
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	};

	stagingBuffer.map();
	stagingBuffer.writeToBuffer((void*)loaderInfo.vertexBuffer);

	vertexBuffer = std::make_unique<Buffer>(
		device,
		vertexSize,
		loaderInfo.vertexCount,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	device.copyBuffer(stagingBuffer.getBuffer(), vertexBuffer->getBuffer(), bufferSize);
}

void GlTFModel::ModelGltf::createIndexBuffers(LoaderInfo loaderInfo)
{
	bool hasIndexBuffer = loaderInfo.indexCount > 0;

	if (!hasIndexBuffer) return;

	VkDeviceSize bufferSize = sizeof(uint32_t) * loaderInfo.indexCount;
	uint32_t indexSize = sizeof(uint32_t);

	Buffer stagingBuffer{
		device,
		indexSize,
		static_cast<uint32_t>(loaderInfo.indexCount),
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	};

	stagingBuffer.map();
	stagingBuffer.writeToBuffer((void*)loaderInfo.indexBuffer);

	indexBuffer = std::make_unique<Buffer>(
		device,
		indexSize,
		static_cast<uint32_t>(loaderInfo.indexCount),
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	device.copyBuffer(stagingBuffer.getBuffer(), indexBuffer->getBuffer(), bufferSize);

}

void GlTFModel::ModelGltf::createDescriptorSet(DescriptorPool& pool, Device& device)
{

	auto textureSetLayout = DescriptorSetLayout::Builder(device)
		.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.addBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.addBinding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		//.addBinding(6, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
		.build();

	auto bufferInfo = nodes[0]->mesh->uniformBuffer->descriptorInfo(); 
	
	Material mat = materials[0];

	VkDescriptorImageInfo baseColorInfo = mat.baseColorTexture->texture->getImageInfo(); 
	VkDescriptorImageInfo metallicRoughnessInfo = mat.metallicRoughnessTexture->texture->getImageInfo(); 
	VkDescriptorImageInfo emissiveInfo = mat.emissiveTexture->texture->getImageInfo(); 
	VkDescriptorImageInfo occlusionInfo = mat.occlusionTexture->texture->getImageInfo(); 
	VkDescriptorImageInfo normalInfo = mat.normalTexture->texture->getImageInfo(); 


	for (int i = 0; i < descriptorSet.size(); i++) 
	{
		DescriptorWriter(*textureSetLayout, pool) 
			.writeImage(1, &baseColorInfo)
			.writeImage(2, &metallicRoughnessInfo)
			.writeImage(3, &emissiveInfo)
			.writeImage(4, &occlusionInfo)
			.writeImage(5, &normalInfo)
		//	.writeBuffer(6, &bufferInfo)
			.build(descriptorSet[i]); 
	}
	
}

void GlTFModel::ModelGltf::bind(VkCommandBuffer commandBuffer)
{
	VkBuffer buffers[] = { vertexBuffer->getBuffer() };
	VkDeviceSize offsets[] = { 0 };

	vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
	vkCmdBindIndexBuffer(commandBuffer, indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
}

GlTFModel::Node* GlTFModel::ModelGltf::findNode(Node* parent, uint32_t index)
{
	Node* nodeFound = nullptr;
	if (parent->index == index) {
		return parent;
	}

	for (auto& child : parent->children) {
		nodeFound = findNode(child, index);
		if (nodeFound) {
			break;
		}
	}

	return nodeFound;
}

GlTFModel::Node* GlTFModel::ModelGltf::nodeFromIndex(uint32_t index)
{
	Node* nodeFound = nullptr;
	for (auto& node : nodes) {
		nodeFound = findNode(node, index);
		if (nodeFound) {
			break;
		}
	}
	return nodeFound;

}

GlTFModel::Mesh::Mesh(Device& device, glm::mat4 matrix) : device {device}
{
	this->uniformBlock.matrix = matrix;
}

GlTFModel::Mesh::~Mesh()
{
	for (Primitive* p : primitives)
		delete p;
}

void GlTFModel::Mesh::setBoundingBox(glm::vec3 min, glm::vec3 max)
{
	bb.min = min;
	bb.max = max;
	bb.valid = true;
}

void GlTFModel::Mesh::createBuffer(bool hasSkin)
{
	VkDeviceSize sizeBuffer;
	if (hasSkin)
		sizeBuffer = sizeof(glm::mat4);
	else
		sizeBuffer = sizeof(UniformBlock);

	uniformBuffer = std::make_unique<Buffer>(
		device,
		sizeBuffer,
		1,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		device.properties.limits.minUniformBufferOffsetAlignment
	);

	uniformBuffer->map();
}

GlTFModel::Primitive::Primitive(uint32_t firstIndex, uint32_t indexCount, uint32_t vertexCount, Material& matrial) : firstIndex(firstIndex), indexCount(indexCount), vertexCount(vertexCount), material(material) 
{
	hasIndices = indexCount > 0;
}

void GlTFModel::Primitive::setBoundingBox(glm::vec3 min, glm::vec3 max)
{
	bb.min = min;
	bb.max = max;
	bb.valid = true;
}

glm::mat4 GlTFModel::Node::localMatrix()
{
	if (!useCachedMatrix) {
		cachedLocalMatrix = glm::translate(glm::mat4(1.0f), translation) * glm::mat4(rotation) * glm::scale(glm::mat4(1.0f), scale) * matrix;
	};

	return cachedLocalMatrix;

}

glm::mat4 GlTFModel::Node::getMatrix()
{
	if (!useCachedMatrix) {
		glm::mat4 m = localMatrix();
		Node* p = parent;
		while (p) {
			m = p->localMatrix() * m;
			p = p->parent;
		}
		cachedMatrix = m;
		useCachedMatrix = true;
		return m;
	}
	else {
		return cachedMatrix;
	}
}

void GlTFModel::Node::update()
{
	useCachedMatrix = false; 

	if (mesh) {
		glm::mat4 m = getMatrix();
		if (skin)
		{

			mesh->uniformBlock.matrix = m;

			// Update join matrices
			glm::mat4 inverseTransform = glm::inverse(m);

			size_t numJoints = std::min((uint32_t)skin->joints.size(), MAX_NUM_JOINTS);

			for (size_t i = 0; i < numJoints; i++)
			{
				Node* jointNode = skin->joints[i];
				glm::mat4 jointMat = jointNode->getMatrix() * skin->inverseBindMatrices[i];
				jointMat = inverseTransform * jointMat;
				mesh->uniformBlock.jointMatrix[i] = jointMat;
			}

			mesh->uniformBlock.jointcount = static_cast<uint32_t>(numJoints);
			mesh->uniformBuffer->writeToBuffer(&mesh->uniformBlock, sizeof(mesh->uniformBlock), 0);
		}
		else {
			mesh->uniformBuffer->writeToBuffer(&m, sizeof(glm::mat4), 0);
		}
		mesh->uniformBuffer->flush();
	}

	for (auto& child : children) {
		child->update();
	}
}

GlTFModel::Node::~Node()
{
	if (mesh) {
		delete mesh;
	}

	for (auto& child : children) {
		delete child;
	}
}

GlTFModel::BoundingBox GlTFModel::BoundingBox::getAABB(glm::mat4 m)
{
	glm::vec3 min = glm::vec3(m[3]);
	glm::vec3 max = min;
	glm::vec3 v0, v1;

	glm::vec3 right = glm::vec3(m[0]);
	v0 = right * this->min.x;
	v1 = right * this->max.x;
	min += glm::min(v0, v1);
	max += glm::max(v0, v1);

	glm::vec3 up = glm::vec3(m[1]);
	v0 = up * this->min.y;
	v1 = up * this->max.y;
	min += glm::min(v0, v1);
	max += glm::max(v0, v1);

	glm::vec3 back = glm::vec3(m[2]);
	v0 = back * this->min.z;
	v1 = back * this->max.z;
	min += glm::min(v0, v1);
	max += glm::max(v0, v1);

	return BoundingBox(min, max);
}

std::unique_ptr<GlTFModel::ModelGltf> GlTFModel::createModelFromFile(Device& device, const std::string& filePath)
{
	std::unique_ptr<GlTFModel::ModelGltf> model = std::make_unique<GlTFModel::ModelGltf>(device);
	model->loadFromFile(filePath, device.graphicsQueue());
	return model;
}

std::vector<VkVertexInputBindingDescription> GlTFModel::ModelGltf::Vertex::getBindingDescriptionsGlTF()
{
	std::vector<VkVertexInputBindingDescription> bindingDescription(1);
	bindingDescription[0].binding = 0;
	bindingDescription[0].stride = sizeof(Vertex);
	bindingDescription[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	return bindingDescription;
}

std::vector<VkVertexInputAttributeDescription> GlTFModel::ModelGltf::Vertex::getAttributeDescriptionsGlTF()
{
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

	attributeDescriptions.push_back({ 0, 0, VK_FORMAT_R32G32B32_SFLOAT    , offsetof(Vertex, position) });
	attributeDescriptions.push_back({ 1, 0, VK_FORMAT_R32G32B32_SFLOAT    , offsetof(Vertex, normal)   });
	attributeDescriptions.push_back({ 2, 0, VK_FORMAT_R32G32_SFLOAT       , offsetof(Vertex, uv0)      });
	attributeDescriptions.push_back({ 3, 0, VK_FORMAT_R32G32_SFLOAT       , offsetof(Vertex, uv1)      });
	attributeDescriptions.push_back({ 4, 0, VK_FORMAT_R32G32B32A32_UINT   , offsetof(Vertex, joint0)   });
	attributeDescriptions.push_back({ 5, 0, VK_FORMAT_R32G32B32A32_SFLOAT , offsetof(Vertex, weight0)  });
	attributeDescriptions.push_back({ 6, 0, VK_FORMAT_R32G32B32_SFLOAT    , offsetof(Vertex, color)    });

	return attributeDescriptions;
}

void GlTFModel::TextureModel::TextFromglTfImage(Device& device, tinygltf::Image& gltfimage, std::string path)
{
	// KTX2 files need to be handled explicitly
	bool isKtx2 = false;
	if (gltfimage.uri.find_last_of(".") != std::string::npos) {
		if (gltfimage.uri.substr(gltfimage.uri.find_last_of(".") + 1) == "ktx2") {
			isKtx2 = true;
		}
	}

	VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

	if (isKtx2) {

		std::cout << "Ktx2\n";
		// Image is KTX2 using basis universal compression. Those images need to be loaded from disk and will be transcoded to a native GPU format

		basist::ktx2_transcoder ktxTranscoder;
		const std::string filename = path + "\\" + gltfimage.uri;
		std::ifstream ifs(filename, std::ios::binary | std::ios::in | std::ios::ate);
		if (!ifs.is_open()) {
			throw std::runtime_error("Could not load the requested image file " + filename);
		}

		uint32_t inputDataSize = static_cast<uint32_t>(ifs.tellg());// get size 
		char* inputData = new char[inputDataSize]; // create local buffer

		ifs.seekg(0, std::ios::beg);
		ifs.read(inputData, inputDataSize); // write to buffer

		bool success = ktxTranscoder.init(inputData, inputDataSize);
		if (!success) {
			throw std::runtime_error("Could not initialize ktx2 transcoder for image file " + filename);
		}

		// Select target format based on device features (use uncompressed if none supported)
		auto targetFormat = basist::transcoder_texture_format::cTFRGBA32;

		{
			VkPhysicalDeviceFeatures pFeatures;

			device.getPhysicalFeatures(&pFeatures);

			// select available format
			if (pFeatures.textureCompressionBC) {
				// BC7 is the preferred block compression if available
				if (device.isFormatSupported(VK_FORMAT_BC7_UNORM_BLOCK)) {
					targetFormat = basist::transcoder_texture_format::cTFBC7_RGBA;
					format = VK_FORMAT_BC7_UNORM_BLOCK;
				}
				else {
					if (device.isFormatSupported(VK_FORMAT_BC3_SRGB_BLOCK)) {
						targetFormat = basist::transcoder_texture_format::cTFBC3_RGBA;
						format = VK_FORMAT_BC3_SRGB_BLOCK;
					}
				}
			}

			// Adaptive scalable texture compression
			if (pFeatures.textureCompressionASTC_LDR) {
				if (device.isFormatSupported(VK_FORMAT_ASTC_4x4_SRGB_BLOCK))
				{
					targetFormat = basist::transcoder_texture_format::cTFASTC_4x4_RGBA;
					format = VK_FORMAT_ASTC_4x4_SRGB_BLOCK;
				}
			}

			// Ericsson texture compression
			if (pFeatures.textureCompressionETC2) {
				if (device.isFormatSupported(VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK))
				{
					targetFormat = basist::transcoder_texture_format::cTFETC2_RGBA;
					format = VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK;
				}
			}
		}

		// @todo PowerVR texture compression support needs to be checked via an extension (VK_IMG_FORMAT_PVRTC_EXTENSION_NAME)
		const bool targetFormatIsUncompressed = basist::basis_transcoder_format_is_uncompressed(targetFormat);

		std::vector<basist::ktx2_image_level_info> levelInfos(ktxTranscoder.get_levels());
		mipLevels = ktxTranscoder.get_levels();

		// Query image level information that we need later on for several calculations
		// We only support 2D images (no cube maps or layered images)
		for (uint32_t i = 0; i < mipLevels; i++) {
			ktxTranscoder.get_image_level_info(levelInfos[i], i, 0, 0);
		}

		width = levelInfos[0].m_orig_width;
		height = levelInfos[0].m_orig_height;

		VkMemoryAllocateInfo memAllocInfo{};
		memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		VkMemoryRequirements memReqs{};

		// Create one staging buffer large enough to hold all uncompressed image levels
		const uint32_t bytesPerBlockOrPixel = basist::basis_get_bytes_per_block_or_pixel(targetFormat);
		uint32_t numBlocksOrPixels = 0;
		VkDeviceSize totalBufferSize = 0;

		for (uint32_t i = 0; i < mipLevels; i++) {
			// Size calculations differ for compressed/uncompressed formats
			numBlocksOrPixels = targetFormatIsUncompressed ? levelInfos[i].m_orig_width * levelInfos[i].m_orig_height : levelInfos[i].m_total_blocks;
			totalBufferSize += numBlocksOrPixels * bytesPerBlockOrPixel;
		}

		Buffer stagingBufferMapped{ device, totalBufferSize, numBlocksOrPixels, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 0 };

		unsigned char* buffer = new unsigned char[totalBufferSize];
		unsigned char* bufferPtr = &buffer[0];

		success = ktxTranscoder.start_transcoding();
		if (!success) {
			throw std::runtime_error("Could not start transcoding for image file " + filename);
		}

		// Transcode all mip levels into the staging buffer
		for (uint32_t i = 0; i < mipLevels; i++) {
			// Size calculations differ for compressed/uncompressed formats
			numBlocksOrPixels = targetFormatIsUncompressed ? levelInfos[i].m_orig_width * levelInfos[i].m_orig_height : levelInfos[i].m_total_blocks;
			uint32_t outputSize = numBlocksOrPixels * bytesPerBlockOrPixel;
			if (!ktxTranscoder.transcode_image_level(i, 0, 0, bufferPtr, numBlocksOrPixels, targetFormat, 0)) {
				throw std::runtime_error("Could not transcode the requested image file " + filename);
			}
			bufferPtr += outputSize;
		}

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		device.createBuffer(totalBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);


		// transfer to device and copy from staging
		void* data;
		vkMapMemory(device.device(), stagingBufferMemory, 0, totalBufferSize, 0, &data);
		memcpy(data, buffer, static_cast<size_t>(totalBufferSize));
		vkUnmapMemory(device.device(), stagingBufferMemory);


		std::shared_ptr<Texture> image = std::make_shared<Texture>(device);
		image->createImage(
			width, height, 
			format, 
			VK_IMAGE_TILING_OPTIMAL, 
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			image->textureImage, 
			image->textureImageMemory, 
			mipLevels);

		image->transitionImageLayout(image->textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);

		device.copyBufferToImage(stagingBuffer, image->textureImage, static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1, mipLevels - 1);

		image->transitionImageLayout(image->textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mipLevels);
		

		vkDestroyBuffer(device.device(), stagingBuffer, nullptr);
		vkFreeMemory(device.device(), stagingBufferMemory, nullptr);

		image->createTextureImageView(mipLevels);
		image->createTextureSampler(mipLevels);

		texture = image;

		delete[] buffer;
		delete[] inputData;
	}
	else {
		// Image is a basic glTF format like png or jpg and can be loaded directly via tinyglTF
		unsigned char* buffer = nullptr;
		VkDeviceSize bufferSize = 0;
		bool deleteBuffer = false;

		if (gltfimage.component == 3) {
			// Most devices don't support RGB only on Vulkan so convert if necessary
			bufferSize = gltfimage.width * gltfimage.height * 4;
			buffer = new unsigned char[bufferSize];
			unsigned char* rgba = buffer;
			unsigned char* rgb = &gltfimage.image[0];
			for (int32_t i = 0; i < gltfimage.width * gltfimage.height; ++i) {
				for (int32_t j = 0; j < 3; ++j) {
					rgba[j] = rgb[j];
				}
				rgba += 4;
				rgb += 3;
			}

			deleteBuffer = true;
		}
		else {
			buffer = &gltfimage.image[0];
			bufferSize = gltfimage.image.size();
		}

		width = gltfimage.width;
		height = gltfimage.height;
		mipLevels = static_cast<uint32_t>(floor(log2(std::max(width, height))) + 1.0);

		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(device.getPhysicalDevice(), format, &formatProperties);
		assert(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT);
		assert(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT);

		texture = std::make_shared<Texture>(device, buffer, width, height, bufferSize);

		if (deleteBuffer)
			delete[] buffer;
	}


}

// Cube spline interpolation function used for translate/scale/rotate with cubic spline animation samples
// Details on how this works can be found in the specs https://github.com/KhronosGroup/glTF/tree/master/specification/2.0#appendix-c-spline-interpolation
glm::vec4 GlTFModel::AnimationSampler::cubicSplineInterpolation(size_t index, float time, uint32_t stride)
{
	float delta = inputs[index + 1] - inputs[index];
	float t = (time - inputs[index]) / delta;
	const size_t current = index * stride * 3;
	const size_t next = (index + 1) * stride * 3;
	const size_t A = 0;
	const size_t V = stride * 1;
	const size_t B = stride * 2;

	float t2 = powf(t, 2);
	float t3 = powf(t, 3);
	glm::vec4 pt{ 0.0f };
	for (uint32_t i = 0; i < stride; i++) {
		float p0 = outputs[current + i + V];			// starting point at t = 0
		float m0 = delta * outputs[current + i + A];	// scaled starting tangent at t = 0
		float p1 = outputs[next + i + V];				// ending point at t = 1
		float m1 = delta * outputs[next + i + B];		// scaled ending tangent at t = 1
		pt[i] = ((2.f * t3 - 3.f * t2 + 1.f) * p0) + ((t3 - 2.f * t2 + t) * m0) + ((-2.f * t3 + 3.f * t2) * p1) + ((t3 - t2) * m0);
	}
	return pt;

}

void GlTFModel::AnimationSampler::translate(size_t index, float time, Node* node)
{
	switch (interpolation) {
		case AnimationSampler::InterpolationType::LINEAR: {
			float u = std::max(0.0f, time - inputs[index]) / (inputs[index + 1] - inputs[index]);
			node->translation = glm::mix(outputsVec4[index], outputsVec4[index + 1], u);
			break;
		}
		case AnimationSampler::InterpolationType::STEP: {
			node->translation = outputsVec4[index];
			break;
		}
		case AnimationSampler::InterpolationType::CUBICSPLINE: {
			node->translation = cubicSplineInterpolation(index, time, 3);
			break;
		}
	}

}

void GlTFModel::AnimationSampler::scale(size_t index, float time, Node* node)
{
	switch (interpolation) {
		case AnimationSampler::InterpolationType::LINEAR: {
			float u = std::max(0.0f, time - inputs[index]) / (inputs[index + 1] - inputs[index]);
			node->scale = glm::mix(outputsVec4[index], outputsVec4[index + 1], u);
			break;
		}
		case AnimationSampler::InterpolationType::STEP: {
			node->scale = outputsVec4[index];
			break;
		}
		case AnimationSampler::InterpolationType::CUBICSPLINE: {
			node->scale = cubicSplineInterpolation(index, time, 3);
			break;
		}
	}

}

void GlTFModel::AnimationSampler::rotate(size_t index, float time, Node* node)
{
	switch (interpolation) {
		case AnimationSampler::InterpolationType::LINEAR: {
			float u = std::max(0.0f, time - inputs[index]) / (inputs[index + 1] - inputs[index]);
			glm::quat q1;
			q1.x = outputsVec4[index].x;
			q1.y = outputsVec4[index].y;
			q1.z = outputsVec4[index].z;
			q1.w = outputsVec4[index].w;
			glm::quat q2;
			q2.x = outputsVec4[index + 1].x;
			q2.y = outputsVec4[index + 1].y;
			q2.z = outputsVec4[index + 1].z;
			q2.w = outputsVec4[index + 1].w;
			node->rotation = glm::normalize(glm::slerp(q1, q2, u));
			break;
		}
		case AnimationSampler::InterpolationType::STEP: {
			glm::quat q1;
			q1.x = outputsVec4[index].x;
			q1.y = outputsVec4[index].y;
			q1.z = outputsVec4[index].z;
			q1.w = outputsVec4[index].w;
			node->rotation = q1;
			break;
		}
		case AnimationSampler::InterpolationType::CUBICSPLINE: {
			glm::vec4 rot = cubicSplineInterpolation(index, time, 4);
			glm::quat q;
			q.x = rot.x;
			q.y = rot.y;
			q.z = rot.z;
			q.w = rot.w;
			node->rotation = glm::normalize(q);
			break;
		}
	}

}

void GlTFModel::ModelGltf::updateAnimation(uint32_t index, float time)
{
	if (animations.empty()) {
		std::cout << ".glTF does not contain animation." << std::endl;
		return;
	}

	if (index > static_cast<uint32_t>(animations.size()) - 1) {
		std::cout << "No animation with index " << index << std::endl;
		return;
	}

	Animation& animation = animations[index];

	bool updated = false;
	for (auto& channel : animation.channels) {
		AnimationSampler& sampler = animation.samplers[channel.samplerIndex];
		if (sampler.inputs.size() > sampler.outputsVec4.size()) {
			continue;
		}

		for (size_t i = 0; i < sampler.inputs.size() - 1; i++) {
			if ((time >= sampler.inputs[i]) && (time <= sampler.inputs[i + 1])) {
				float u = std::max(0.0f, time - sampler.inputs[i]) / (sampler.inputs[i + 1] - sampler.inputs[i]);
				if (u <= 1.0f) {
					switch (channel.path) {
					case AnimationChannel::PathType::TRANSLATION:
						sampler.translate(i, time, channel.node);
						break;
					case AnimationChannel::PathType::SCALE:
						sampler.scale(i, time, channel.node);
						break;
					case AnimationChannel::PathType::ROTATION:
						sampler.rotate(i, time, channel.node);
						break;
					}
					updated = true;
				}
			}
		}
	}
	if (updated) {
		for (auto& node : nodes) {
			node->update();
		}
	}

}