#include "GlTFModelDecoder.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <functional>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "../../external/tiny_gltf.h"

#include "../Vertex/ObjVertexData.h"
#include "../Vertex/GlTFVertexData.h"
#include <glm/gtc/type_ptr.hpp>

namespace {

	void loadAnimations(const tinygltf::Model& model, std::vector<AnimationClip>& clips) {
		for (const tinygltf::Animation& anim : model.animations) {
			AnimationClip clip{};
			clip.name = anim.name.empty() ? std::to_string(clips.size()) : anim.name;

			for (const auto& samp : anim.samplers) {
				AnimationSampler sampler{};
				if (samp.interpolation == "STEP") sampler.interpolation = AnimationSampler::STEP;
				else if (samp.interpolation == "CUBICSPLINE") sampler.interpolation = AnimationSampler::CUBICSPLINE;
				else sampler.interpolation = AnimationSampler::LINEAR;

				{
					const tinygltf::Accessor& accessor = model.accessors[samp.input];
					const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
					const tinygltf::Buffer& buffer = model.buffers[view.buffer];
					assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

					const float* buf = reinterpret_cast<const float*>(&buffer.data[view.byteOffset + accessor.byteOffset]);
					sampler.inputs.assign(buf, buf + accessor.count);

					if (!sampler.inputs.empty()) {
						clip.start = std::min(clip.start, sampler.inputs.front());
						clip.end = std::max(clip.end, sampler.inputs.back());
					}
				}

				{
					const tinygltf::Accessor& accessor = model.accessors[samp.output];
					const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
					const tinygltf::Buffer& buffer = model.buffers[view.buffer];
					assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

					const void* dataPtr = &buffer.data[view.byteOffset + accessor.byteOffset];
					sampler.outputs.reserve(accessor.count);

					if (accessor.type == TINYGLTF_TYPE_VEC3) {
						const glm::vec3* buf = static_cast<const glm::vec3*>(dataPtr);
						for (size_t i = 0; i < accessor.count; ++i) sampler.outputs.push_back(glm::vec4(buf[i], 0.f));
					}
					else if (accessor.type == TINYGLTF_TYPE_VEC4) {
						const glm::vec4* buf = static_cast<const glm::vec4*>(dataPtr);
						sampler.outputs.assign(buf, buf + accessor.count);
					}
					else {
						std::cerr << "GLTF warning: unsupported animation output type (morph weights?)" << std::endl;
					}
				}

				clip.samplers.push_back(std::move(sampler));
			}

			for (const auto& source : anim.channels) {
				if (source.target_node < 0) continue;

				AnimationChannel channel{};
				if (source.target_path == "translation") channel.path = AnimationChannel::TRANSLATION;
				else if (source.target_path == "rotation") channel.path = AnimationChannel::ROTATION;
				else if (source.target_path == "scale") channel.path = AnimationChannel::SCALE;
				else continue;

				channel.node = static_cast<uint32_t>(source.target_node);
				channel.samplerIndex = static_cast<uint32_t>(source.sampler);
				clip.channels.push_back(channel);
			}

			clips.push_back(std::move(clip));
		}
	}

	glm::vec3 readVec3(const tinygltf::Model& model, const tinygltf::Accessor& accessor, size_t index) {
		if (accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) return glm::vec3(0.0f);
		const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
		const tinygltf::Buffer& buffer = model.buffers[view.buffer];
		size_t stride = view.byteStride ? view.byteStride : sizeof(float) * 3;
		size_t byteOffset = view.byteOffset + accessor.byteOffset + index * stride;
		const float* f = reinterpret_cast<const float*>(&buffer.data[byteOffset]);
		return glm::vec3(f[0], f[1], f[2]);
	}

	glm::vec2 readVec2(const tinygltf::Model& model, const tinygltf::Accessor& accessor, size_t index) {
		if (accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) return glm::vec2(0.0f);
		const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
		const tinygltf::Buffer& buffer = model.buffers[view.buffer];
		size_t stride = view.byteStride ? view.byteStride : sizeof(float) * 2;
		size_t byteOffset = view.byteOffset + accessor.byteOffset + index * stride;
		const float* f = reinterpret_cast<const float*>(&buffer.data[byteOffset]);
		return glm::vec2(f[0], f[1]);
	}

	glm::vec4 readVec4f(const tinygltf::Model& model, const tinygltf::Accessor& accessor, size_t index) {
		const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
		const tinygltf::Buffer& buffer = model.buffers[view.buffer];

		if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
			size_t stride = view.byteStride ? view.byteStride : sizeof(uint8_t) * 4;
			const uint8_t* v = &buffer.data[view.byteOffset + accessor.byteOffset + index * stride];
			return glm::vec4(v[0], v[1], v[2], v[3]) / 255.f;
		}
		if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
			size_t stride = view.byteStride ? view.byteStride : sizeof(uint16_t) * 4;
			const uint16_t* v = reinterpret_cast<const uint16_t*>(&buffer.data[view.byteOffset + accessor.byteOffset +
				index * stride]);
			return glm::vec4(v[0], v[1], v[2], v[3]) / 65535.f;
		}

		if (accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) return glm::vec4(0.0f);

		size_t stride = view.byteStride ? view.byteStride : sizeof(float) * 4;
		size_t byteOffset = view.byteOffset + accessor.byteOffset + index * stride;
		const float* f = reinterpret_cast<const float*>(&buffer.data[byteOffset]);
		return glm::vec4(f[0], f[1], f[2], f[3]);
	}

	void loadSkins(const tinygltf::Model& model, Skeleton& sk) {
		for (const auto& src : model.skins) {
			Skin skin;
			skin.name = src.name;
			skin.skeletonRoot = src.skeleton;
			skin.joints.assign(src.joints.begin(), src.joints.end());  

			if (src.inverseBindMatrices > -1) {
				const auto& acc = model.accessors[src.inverseBindMatrices];

				assert(acc.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT && acc.type == TINYGLTF_TYPE_MAT4);

				const auto& view = model.bufferViews[acc.bufferView];
				const auto& buf = model.buffers[view.buffer];
				skin.inverseBindMatrices.resize(acc.count);
				memcpy(skin.inverseBindMatrices.data(), &buf.data[view.byteOffset + acc.byteOffset], acc.count * sizeof(glm::mat4));
			}
			else {
				skin.inverseBindMatrices.assign(skin.joints.size(), glm::mat4(1.f));
			}
			sk.skins.push_back(std::move(skin));
		}
	}

	glm::mat4 nodeLocalMatrix(const tinygltf::Node& node) {
		if (node.matrix.size() == 16) {
			glm::dmat4 m = glm::make_mat4(node.matrix.data());
			return glm::mat4(m);
		}

		glm::mat4 m(1.f);
		if (node.translation.size() == 3)
			m = glm::translate(m, glm::vec3(glm::make_vec3(node.translation.data())));
		if (node.rotation.size() == 4) {
			// gltf stores x, y, z, w; glm::quat constructor takes w, x, y, z
			glm::quat q(
				static_cast<float>(node.rotation[3]),
				static_cast<float>(node.rotation[0]),
				static_cast<float>(node.rotation[1]),
				static_cast<float>(node.rotation[2])
			);
			m *= glm::mat4_cast(q);
		}
		if (node.scale.size() == 3)
			m = glm::scale(m, glm::vec3(glm::make_vec3(node.scale.data())));
		return m;
	}

	glm::uvec4 readUVec4(const tinygltf::Model& model, const tinygltf::Accessor& accessor, size_t index) {
		glm::uvec4 out(0);
		const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
		const tinygltf::Buffer& buffer = model.buffers[view.buffer];
		if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
			size_t stride = view.byteStride ? view.byteStride : sizeof(uint16_t) * 4;
			size_t byteOffset = view.byteOffset + accessor.byteOffset + index * stride;
			const uint16_t* v = reinterpret_cast<const uint16_t*>(&buffer.data[byteOffset]);
			out = glm::uvec4((uint32_t)v[0], (uint32_t)v[1], (uint32_t)v[2], (uint32_t)v[3]);
		}
		else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
			size_t stride = view.byteStride ? view.byteStride : sizeof(uint8_t) * 4;
			size_t byteOffset = view.byteOffset + accessor.byteOffset + index * stride;
			const uint8_t* v = reinterpret_cast<const uint8_t*>(&buffer.data[byteOffset]);
			out = glm::uvec4((uint32_t)v[0], (uint32_t)v[1], (uint32_t)v[2], (uint32_t)v[3]);
		}
		else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
			size_t stride = view.byteStride ? view.byteStride : sizeof(uint32_t) * 4;
			size_t byteOffset = view.byteOffset + accessor.byteOffset + index * stride;
			const uint32_t* v = reinterpret_cast<const uint32_t*>(&buffer.data[byteOffset]);
			out = glm::uvec4(v[0], v[1], v[2], v[3]);
		}
		return out;
	}

	DecodedTexture readTexture(const tinygltf::Model& model, int textureIndex, const std::filesystem::path& modelDir) {
		DecodedTexture out{};
		if (textureIndex < 0 || textureIndex >= static_cast<int>(model.textures.size())) return out;

		const tinygltf::Texture& tex = model.textures[textureIndex];
		if (tex.source < 0 || tex.source >= static_cast<int>(model.images.size())) return out;

		const tinygltf::Image& img = model.images[tex.source];

		if (!img.image.empty() && img.bits == 8 && img.component >= 1 && img.component <= 4) {
			const size_t pixelCount = static_cast<size_t>(img.width) * img.height;
			out.width = static_cast<uint32_t>(img.width);
			out.height = static_cast<uint32_t>(img.height);

			if (img.component == 4) {
				out.pixels = img.image;
			}
			else {
				// most devices don't support RGB only on Vulkan, expand to RGBA
				out.pixels.resize(pixelCount * 4);
				for (size_t i = 0; i < pixelCount; ++i) {
					const unsigned char* src = &img.image[i * img.component];
					unsigned char* dst = &out.pixels[i * 4];
					dst[0] = src[0];
					dst[1] = img.component >= 2 ? src[1] : src[0];
					dst[2] = img.component >= 3 ? src[2] : src[0];
					dst[3] = img.component == 2 ? src[1] : 255;
				}
			}
			return out;
		}

		if (!img.uri.empty() && img.uri.rfind("data:", 0) != 0) {
			std::filesystem::path imgPath(img.uri);
			if (imgPath.is_relative()) imgPath = modelDir / imgPath;
			out.path = imgPath.string();
		}
		else {
			std::cerr << "GLTF warning: unsupported image format for texture " << textureIndex << std::endl;
		}

		return out;
	}

	std::vector<uint32_t> readIndices(const tinygltf::Model& model, const tinygltf::Accessor& accessor) {
		std::vector<uint32_t> out;
		const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
		const tinygltf::Buffer& buffer = model.buffers[view.buffer];
		size_t count = accessor.count;
		out.reserve(count);
		size_t byteOffsetBase = view.byteOffset + accessor.byteOffset;
		if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
			for (size_t i = 0; i < count; ++i) {
				const uint16_t* v = reinterpret_cast<const uint16_t*>(&buffer.data[byteOffsetBase + i * sizeof(uint16_t)]);
				out.push_back(static_cast<uint32_t>(*v));
			}
		}
		else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
			for (size_t i = 0; i < count; ++i) {
				const uint32_t* v = reinterpret_cast<const uint32_t*>(&buffer.data[byteOffsetBase + i * sizeof(uint32_t)]);
				out.push_back(*v);
			}
		}
		else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
			for (size_t i = 0; i < count; ++i) {
				const uint8_t* v = reinterpret_cast<const uint8_t*>(&buffer.data[byteOffsetBase + i * sizeof(uint8_t)]);
				out.push_back(static_cast<uint32_t>(*v));
			}
		}
		return out;
	}

	NodePose readNodePose(const tinygltf::Node& n) {
		NodePose p;
		if (n.translation.size() == 3) 
			p.translation = glm::vec3(glm::make_vec3(n.translation.data()));
		if (n.rotation.size() == 4)    
			p.rotation = glm::quat(
			float(n.rotation[3]), 
			float(n.rotation[0]),
			float(n.rotation[1]), 
			float(n.rotation[2])
		);

		if (n.scale.size() == 3)      
			p.scale = glm::vec3(glm::make_vec3(n.scale.data()));
		return p;
	}

	void loadNodes(const tinygltf::Model& model, Skeleton& sk) {
		sk.nodes.resize(model.nodes.size());
		for (size_t i = 0; i < model.nodes.size(); ++i) {
			const auto& n = model.nodes[i];
			sk.nodes[i].name = n.name;
			sk.nodes[i].restPose = readNodePose(n);
			if (n.matrix.size() == 16) sk.nodes[i].matrix = glm::mat4(glm::make_mat4(n.matrix.data()));
			for (int c : n.children) sk.nodes[c].parent = static_cast<int32_t>(i);
		}
	}
}

bool GlTFModelDecoder::canDecode(const std::filesystem::path& path) const {
	auto ext = path.extension().string();
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
	if (!ext.empty() && ext[0] == '.') ext.erase(ext.begin());
	return (ext == "gltf" || ext == "glb");
}

DecodedModel GlTFModelDecoder::decode(const std::filesystem::path& path) const {
	tinygltf::Model model;
	tinygltf::TinyGLTF loader;
	std::string err;
	std::string warn;

	bool ret = false;
	auto ext = path.extension().string();
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
	if (!ext.empty() && ext[0] == '.') ext.erase(ext.begin());
	if (ext == "glb") {
		ret = loader.LoadBinaryFromFile(&model, &err, &warn, path.string());
	} else {
		ret = loader.LoadASCIIFromFile(&model, &err, &warn, path.string());
	}

	if (!warn.empty()) std::cerr << "GLTF warning: " << warn << std::endl;
	if (!err.empty()) std::cerr << "GLTF error: " << err << std::endl;
	if (!ret) throw std::runtime_error("Failed to load glTF file: " + path.string());

	DecodedModel out{};
	std::vector<GltfVertex> vertices;
	std::vector<uint32_t> indices;
	std::vector<Primitive> primitives;
	std::vector<DecodedMaterial> materials;

	// extract materials
	for (const auto& mat : model.materials) {
		DecodedMaterial dm;
		dm.name = mat.name;
		dm.albedoTexture = readTexture(model, mat.pbrMetallicRoughness.baseColorTexture.index, path.parent_path());
		dm.normalTexture = readTexture(model, mat.normalTexture.index, path.parent_path());
		dm.metallicRoughnessTexture = readTexture(model, mat.pbrMetallicRoughness.metallicRoughnessTexture.index, path.parent_path());
		dm.occlusionTexture = readTexture(model, mat.occlusionTexture.index, path.parent_path());
		dm.emissiveTexture = readTexture(model, mat.emissiveTexture.index, path.parent_path());
		dm.emissiveFactor = glm::vec3(
			static_cast<float>(mat.emissiveFactor[0]),
			static_cast<float>(mat.emissiveFactor[1]),
			static_cast<float>(mat.emissiveFactor[2])
		);
		dm.alphaMask = mat.alphaMode == "MASK";
		dm.alphaCutoff = static_cast<float>(mat.alphaCutoff);
		dm.metallic = static_cast<float>(mat.pbrMetallicRoughness.metallicFactor);
		dm.roughness = static_cast<float>(mat.pbrMetallicRoughness.roughnessFactor);
		dm.baseColorFactor = glm::vec4(
			static_cast<float>(mat.pbrMetallicRoughness.baseColorFactor[0]),
			static_cast<float>(mat.pbrMetallicRoughness.baseColorFactor[1]),
			static_cast<float>(mat.pbrMetallicRoughness.baseColorFactor[2]),
			static_cast<float>(mat.pbrMetallicRoughness.baseColorFactor[3])
		);
		materials.push_back(std::move(dm));
	}

	glm::vec3 minV(std::numeric_limits<float>::infinity());
	glm::vec3 maxV(-std::numeric_limits<float>::infinity());

	auto appendMesh = [&](const tinygltf::Mesh& mesh, const glm::mat4& nodeMatrix, int32_t nodeIndex, int32_t skinIndex) {

		for (const auto& prim : mesh.primitives) {
			size_t primFirstIndex = indices.size();

			// find accessors
			const auto itPos = prim.attributes.find("POSITION");
			if (itPos == prim.attributes.end()) continue; // invalid primitive
			const tinygltf::Accessor& posAcc = model.accessors[itPos->second];

			const tinygltf::Accessor* normalAcc = nullptr;
			const tinygltf::Accessor* tex0Acc = nullptr;
			const tinygltf::Accessor* tex1Acc = nullptr;
			const tinygltf::Accessor* jointsAcc = nullptr;
			const tinygltf::Accessor* weightsAcc = nullptr;
			const tinygltf::Accessor* colorAcc = nullptr;

			auto it = prim.attributes.find("NORMAL"); if (it != prim.attributes.end()) normalAcc = &model.accessors[it->second];
			it = prim.attributes.find("TEXCOORD_0"); if (it != prim.attributes.end()) tex0Acc = &model.accessors[it->second];
			it = prim.attributes.find("TEXCOORD_1"); if (it != prim.attributes.end()) tex1Acc = &model.accessors[it->second];
			it = prim.attributes.find("JOINTS_0"); if (it != prim.attributes.end()) jointsAcc = &model.accessors[it->second];
			it = prim.attributes.find("WEIGHTS_0"); if (it != prim.attributes.end()) weightsAcc = &model.accessors[it->second];
			it = prim.attributes.find("COLOR_0"); if (it != prim.attributes.end()) colorAcc = &model.accessors[it->second];

			const uint32_t vertexCount = static_cast<uint32_t>(posAcc.count);

			std::vector<uint32_t> primIndices;
			if (prim.indices >= 0) {
				primIndices = readIndices(model, model.accessors[prim.indices]);
			}
			else {
				primIndices.resize(vertexCount);
				for (uint32_t k = 0; k < vertexCount; ++k) primIndices[k] = k;
			}
			if (primIndices.empty()) continue;

			if (std::any_of(primIndices.begin(), primIndices.end(), [&](uint32_t idx) { return idx >= vertexCount; })) {
				std::cerr << "GLTF warning: primitive of mesh '" << mesh.name << "' has out of range indices, skipped" << std::endl;
				continue;
			}

			const uint32_t baseVertex = static_cast<uint32_t>(vertices.size());
			for (uint32_t k = 0; k < vertexCount; ++k) {
				GltfVertex v{};
				v.position = readVec3(model, posAcc, k);
				if (normalAcc) v.normal = readVec3(model, *normalAcc, k);
				if (tex0Acc) v.uv0 = readVec2(model, *tex0Acc, k);
				if (tex1Acc) v.uv1 = readVec2(model, *tex1Acc, k);
				if (colorAcc) {
					glm::vec4 c = readVec4f(model, *colorAcc, k);
					v.color = glm::vec3(c.r, c.g, c.b);
				}
				if (jointsAcc && weightsAcc) {
					v.joint0 = readUVec4(model, *jointsAcc, k);
					v.weight0 = readVec4f(model, *weightsAcc, k);
				}

				const glm::vec3 restPos = glm::vec3(nodeMatrix * glm::vec4(v.position, 1.f));
				minV = glm::min(minV, restPos);
				maxV = glm::max(maxV, restPos);

				vertices.push_back(v);
			}

			for (uint32_t idx : primIndices)
				indices.push_back(baseVertex + idx);

			Primitive p;
			p.firstIndex = static_cast<uint32_t>(primFirstIndex);
			p.indexCount = static_cast<uint32_t>(primIndices.size());
			p.materialIndex = prim.material >= 0 ? static_cast<uint32_t>(prim.material) : 0;
			p.nodeIndex = static_cast<uint32_t>(nodeIndex);
			p.skinIndex = static_cast<uint32_t>(skinIndex); 
			primitives.push_back(p);
		}
	};

	std::function<void(int, const glm::mat4&)> visitNode = [&](int nodeIndex, const glm::mat4& parentMatrix) {
		out.skeleton.evalOrder.push_back(static_cast<uint32_t>(nodeIndex));
		const tinygltf::Node& node = model.nodes[nodeIndex];
		const glm::mat4 globalMatrix = parentMatrix * nodeLocalMatrix(node);

		if (node.mesh >= 0)
			appendMesh(model.meshes[node.mesh], globalMatrix, nodeIndex, node.skin);

		for (int child : node.children)
			visitNode(child, globalMatrix);
	};

	loadNodes(model, out.skeleton);
	std::vector<int> rootNodes;
	if (!model.scenes.empty()) {
		const int sceneIndex = model.defaultScene >= 0 ? model.defaultScene : 0;
		rootNodes = model.scenes[sceneIndex].nodes;
	}
	else {
		std::vector<bool> isChild(model.nodes.size(), false);
		for (const auto& node : model.nodes)
			for (int child : node.children) isChild[child] = true;
		for (int i = 0; i < static_cast<int>(model.nodes.size()); ++i)
			if (!isChild[i]) rootNodes.push_back(i);
	}

	for (int root : rootNodes)
		visitNode(root, glm::mat4(1.f));

	loadSkins(model, out.skeleton);
	loadAnimations(model, out.skeleton.clips);

	uint32_t matrixCount = static_cast<uint32_t>(out.skeleton.nodes.size());
	for (Skin& skin : out.skeleton.skins) {
		skin.jointOffset = matrixCount;
		matrixCount += static_cast<uint32_t>(skin.joints.size());
	}

	out.skeleton.matrixCount = matrixCount;
	out.vertices = std::make_unique<GltfVertexData>(std::move(vertices));
	out.indices = std::move(indices);
	out.primitives = std::move(primitives);
	out.materials = std::move(materials);
	out.aabb = BoundingBox(minV, maxV);
	out.aabb.valid = true;
	out.name = path.filename().string();

	return out;
}
