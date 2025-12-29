#include "ModelAsset.h"

#include <iostream>

bool ModelLOD::updateAnimation(uint32_t index, float animationTimer, float speed)
{
	if (animations.empty()) {
		std::cout << ".glTF does not contain animation." << std::endl;
		return false;
	}

	if (index > static_cast<uint32_t>(animations.size()) - 1) {
		std::cout << "No animation with index " << index << std::endl;
		return false;
	}


	Animation& animation = animations[index];

	bool updated = false;
	for (auto& channel : animation.channels) {
		AnimationSampler& sampler = animation.samplers[channel.samplerIndex];
		if (sampler.inputs.size() > sampler.outputsVec4.size()) {
			continue;
		}

		for (size_t i = 0; i < sampler.inputs.size() - 1; i++) {
			const float t0 = sampler.inputs[i];
			const float t1 = sampler.inputs[i + 1];

			if (animationTimer < t0 || animationTimer > t1) {
				continue;
			}
			
			float u = std::max(0.0f, animationTimer - t0) / (t1 - t0);
			if (u <= 1.0f) {
				switch (channel.path) {
				case AnimationChannel::PathType::TRANSLATION:
					sampler.translate(i, animationTimer, channel.node);
					break;
				case AnimationChannel::PathType::SCALE:
					sampler.scale(i, animationTimer, channel.node);
					break;
				case AnimationChannel::PathType::ROTATION:
					sampler.rotate(i, animationTimer, channel.node);
					break;
				}
				updated = true;
			}
		}
	}

	if (updated)
	{
		for (auto& node : LinearNodes) {
			node->update_cpu();
		}
		return true;
	}

	return false;

}
