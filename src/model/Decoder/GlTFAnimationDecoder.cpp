#include "GlTFModelDecoder.h"

#include <iostream>
#include <glm/fwd.hpp>
#include <glm/gtc/type_ptr.hpp>


namespace
{
	const void* getBufferData(
		const tinygltf::Model& model,
		const tinygltf::Accessor& accessor)
	{
		const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
		const tinygltf::Buffer& buffer = model.buffers[view.buffer];

		return buffer.data.data() + accessor.byteOffset + view.byteOffset;
	}

	const float* getFloatBuffer(const tinygltf::Model& model, const tinygltf::Accessor& accessor)
	{
		return reinterpret_cast<const float*>(
			getBufferData(model, accessor)
			);
	}

	DecodedAnimationSampler::InterpolationType parseInterpolation(const std::string& interp)
	{
		if (interp == "LINEAR")       return DecodedAnimationSampler::InterpolationType::LINEAR;
		if (interp == "STEP")         return DecodedAnimationSampler::InterpolationType::STEP;
		if (interp == "CUBICSPLINE")  return DecodedAnimationSampler::InterpolationType::CUBICSPLINE;

		return DecodedAnimationSampler::InterpolationType::LINEAR;
	}

	DecodedAnimationChannel::PathType parsePath(const std::string& path)
	{
		if (path == "translation") return DecodedAnimationChannel::PathType::TRANSLATION;
		if (path == "rotation")    return DecodedAnimationChannel::PathType::ROTATION;
		if (path == "scale")       return DecodedAnimationChannel::PathType::SCALE;

		throw std::runtime_error("Unsupported animation channel path: " + path);
	}

}


std::vector<DecodedAnimation> GlTFModelDecoder::loadAnimations(tinygltf::Model& gltfModel)
{
	std::vector<DecodedAnimation> animations;
	animations.reserve(gltfModel.animations.size());

	for (tinygltf::Animation& anim : gltfModel.animations)
	{
		DecodedAnimation animation{};
		animation.name = anim.name;

		if (anim.name.empty()) {
			animation.name = std::to_string(animations.size());
		}

		// Samplers
		for (auto& samp : anim.samplers) {
			DecodedAnimationSampler sampler{};
			sampler.interpolation = parseInterpolation(samp.interpolation);

			// Read sampler input time values
			{
				const tinygltf::Accessor& accessor = gltfModel.accessors[samp.input];

				assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
				assert(accessor.type == TINYGLTF_TYPE_SCALAR);

				const float* times = getFloatBuffer(gltfModel, accessor);
				sampler.inputs.assign(times, times + accessor.count);

				for (auto input : sampler.inputs) {
					animation.start = std::min(animation.start, input);
					animation.end   = std::max(animation.end, input);
				}
			}

			// Read sampler output T/R/S values 
			{
				const tinygltf::Accessor& accessor = gltfModel.accessors[samp.output];
				
				assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

				const float* data = getFloatBuffer(gltfModel, accessor);

				if (accessor.type == TINYGLTF_TYPE_VEC3)
				{
					for (size_t i = 0; i < accessor.count; ++i)
					{
						glm::vec3 v = glm::make_vec3(data + i * 3);
						sampler.outputsVec4.emplace_back(v, 0.0f);
					}
				}
				else if (accessor.type == TINYGLTF_TYPE_VEC4)
				{
					for (size_t i = 0; i < accessor.count; ++i)
					{
						sampler.outputsVec4.emplace_back(
							glm::make_vec4(data + i * 4));
					}
				}
				else
				{
					std::cerr << "Unsupported animation sampler output type : "<< accessor.type << "\n";
					continue;
				}
			}

			animation.samplers.push_back(sampler);
		}

		// Channels
		for (auto& source : anim.channels) {

			if (source.target_path == "weights") {
				std::cerr << "weights not yet supported, skipping channel" << std::endl;
				continue;
			}

			DecodedAnimationChannel channel{};
			channel.path = parsePath(source.target_path);
			channel.samplerIndex = source.sampler;
			channel.nodeIndex = source.target_node;

			animation.channels.push_back(channel);
		}

		animations.push_back(animation);
	}

	return animations;
}

std::vector<DecodedSkin> GlTFModelDecoder::loadSkins(tinygltf::Model& gltfModel)
{
	std::vector<DecodedSkin> skins;
	skins.reserve(gltfModel.skins.size());

	for (tinygltf::Skin& source : gltfModel.skins) {
		DecodedSkin newSkin{};
		newSkin.name = source.name;

		// Find skeleton root node
		if (source.skeleton > -1) {
			newSkin.skeletonRootIndex = source.skeleton;
		}

		// Find joint nodes
		newSkin.jointsIndex.reserve(source.joints.size());
		for (int jointIndex : source.joints) 
		{
			newSkin.jointsIndex.push_back(
				static_cast<size_t>(jointIndex));
		}

		// Get inverse bind matrices from buffer
		if (source.inverseBindMatrices > -1) {
			const tinygltf::Accessor& accessor = gltfModel.accessors[source.inverseBindMatrices];
			assert(accessor.type == TINYGLTF_TYPE_MAT4);
			assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

			const void* data = getBufferData(gltfModel, accessor);
						
			newSkin.inverseBindMatrices.resize(accessor.count);

			if (accessor.ByteStride(
				gltfModel.bufferViews[accessor.bufferView]) == sizeof(glm::mat4))
			{
				std::memcpy(
					newSkin.inverseBindMatrices.data(),
					data,
					accessor.count * sizeof(glm::mat4));
			}
			else
			{
				const uint8_t* src = static_cast<const uint8_t*>(data);
				for (size_t i = 0; i < accessor.count; ++i)
				{
					std::memcpy(
						&newSkin.inverseBindMatrices[i],
						src + i * accessor.ByteStride(
							gltfModel.bufferViews[accessor.bufferView]),
						sizeof(glm::mat4));
				}
			}
		}

		skins.push_back(newSkin);
	}

	return skins;
}

