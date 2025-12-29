#include "GlTFModelDecoder.h"

#include <string>
#include <vector>

#include "../../../external/tiny_gltf.h"
#include <glm/gtc/type_ptr.hpp>


namespace {

	ToBeDecodedTexture TextFromglTfImage(tinygltf::Image& gltfimage, std::string path = "") {
		ToBeDecodedTexture texture;

		if (moDecoder::getExtension(gltfimage.uri) == "ktx2") {
			texture.textureName = path;
			return texture;
		}

		unsigned char* buffer = nullptr;
		VkDeviceSize bufferSize = 0;
		bool deleteBuffer = false;

		if (gltfimage.component == 3) {
			// Most devices dont support RGB only on Vulkan so convert if necessary
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

		texture.width = gltfimage.width;
		texture.height = gltfimage.height;
		texture.rawData = std::vector<unsigned char>(buffer, buffer + bufferSize);

		if (deleteBuffer)
			delete[] buffer;

		return texture;
		
	}

}

std::vector<ToBeDecodedTexture> GlTFModelDecoder::loadTextures(tinygltf::Model& gltfModel)
{
	std::vector<ToBeDecodedTexture> textures;


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

		//TextureSampler textureSampler;

		//if (tex.sampler == -1) {
		//	// No sampler specified, use a default one
		//	textureSampler.magFilter = VK_FILTER_LINEAR;
		//	textureSampler.minFilter = VK_FILTER_LINEAR;
		//	textureSampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		//	textureSampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		//	textureSampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		//}
		//else {
		//	textureSampler = textureSamplers[tex.sampler];
		//}

		textures.push_back(TextFromglTfImage(image));
	}

	if (!textures.size()) {
		ToBeDecodedTexture texture;
		texture.textureName = "textures/whiteTexture.jpg";

		textures.push_back(texture);
	}

	return textures;

}


std::vector<DecodedMaterial> GlTFModelDecoder::loadMaterials(tinygltf::Model& gltfModel)
{
	std::vector<DecodedMaterial> materials;

	for (tinygltf::Material& mat : gltfModel.materials) {
		DecodedMaterial material{};
		material.doubleSided = mat.doubleSided;

		if (mat.values.find("roughnessFactor") != mat.values.end()) {
			material.roughness = static_cast<float>(mat.values["roughnessFactor"].Factor());
		}
		if (mat.values.find("metallicFactor") != mat.values.end()) {
			material.metallic = static_cast<float>(mat.values["metallicFactor"].Factor());
		}
		if (mat.values.find("baseColorFactor") != mat.values.end()) {
			material.baseColorFactor = glm::make_vec4(mat.values["baseColorFactor"].ColorFactor().data());
		}


		if (mat.values.find("baseColorTexture") != mat.values.end()) {
			//material.texCoordSets.baseColor = mat.values["baseColorTexture"].TextureTexCoord();
			material.baseColorTextureIndex = mat.values["baseColorTexture"].TextureIndex();
		}

		if (mat.values.find("metallicRoughnessTexture") != mat.values.end()) {
			//material.texCoordSets.metallicRoughness = mat.values["metallicRoughnessTexture"].TextureTexCoord();
			material.metallicRoughnessTextureIndex = mat.values["metallicRoughnessTexture"].TextureIndex();
		}

		if (mat.additionalValues.find("normalTexture") != mat.additionalValues.end()) {
			//material.texCoordSets.normal = mat.additionalValues["normalTexture"].TextureTexCoord();
			material.normalTextureIndex = mat.additionalValues["normalTexture"].TextureIndex();
		}

		if (mat.additionalValues.find("emissiveTexture") != mat.additionalValues.end()) {
			//material.texCoordSets.emissive = mat.additionalValues["emissiveTexture"].TextureTexCoord();
			material.emissiveTextureIndex = mat.additionalValues["emissiveTexture"].TextureIndex();
		}

		if (mat.additionalValues.find("occlusionTexture") != mat.additionalValues.end()) {
			//material.texCoordSets.occlusion = mat.additionalValues["occlusionTexture"].TextureTexCoord();
			material.occlusionTextureIndex = mat.additionalValues["occlusionTexture"].TextureIndex();
		}

		if (mat.additionalValues.find("alphaMode") != mat.additionalValues.end()) {
			tinygltf::Parameter param = mat.additionalValues["alphaMode"];
			if (param.string_value == "BLEND") {
				material.alphaMode = AlphaMode::ALPHAMODE_BLEND;
			}
			if (param.string_value == "MASK") {
				material.alphaCutoff = 0.5f;
				material.alphaMode = AlphaMode::ALPHAMODE_MASK;
			}
		}
		if (mat.additionalValues.find("alphaCutoff") != mat.additionalValues.end()) {
			material.alphaCutoff = static_cast<float>(mat.additionalValues["alphaCutoff"].Factor());
		}
		if (mat.additionalValues.find("emissiveFactor") != mat.additionalValues.end()) {
			material.emissiveFactor = glm::vec4(glm::make_vec3(mat.additionalValues["emissiveFactor"].ColorFactor().data()), 1.0);
		}

		// Extensions
		/*if (mat.extensions.find("KHR_materials_pbrSpecularGlossiness") != mat.extensions.end()) {
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
		}*/

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
	materials.push_back(DecodedMaterial());

	return materials;
}
