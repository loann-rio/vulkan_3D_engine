#include "ModelNode.h"

#include <glm/gtc/matrix_transform.hpp>
#include "../base/Buffer.h"


Node::~Node()
{
}

glm::mat4 Node::localMatrix()
{
	if (!useCachedMatrix) {
		cachedLocalMatrix = glm::translate(glm::mat4(1.0f), transform.translation) * glm::mat4(transform.rotation) * glm::scale(glm::mat4(1.0f), transform.scale) * matrix;
	};

	return cachedLocalMatrix;

}

glm::mat4 Node::getMatrix()
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

void Node::update_cpu() {
	useCachedMatrix = false;

	if (bufferCreated)
	{
		glm::mat4 m = getMatrix();

		if (hasSkin)
		{
			updateSkinnedUniforms(m);
		}
		else {
			updateStaticUniform(m);
		}

		uniformBuffer->flush();
	}

	for (Node* child : children) {
		child->update_cpu();
	}
}

void Node::update_gpu()
{
}

void Node::createBuffer(bool hasSkin, Device& device)
{
	vkDeviceWaitIdle(device.device());

	VkDeviceSize sizeBuffer;
	if (!hasSkin)
		sizeBuffer = sizeof(glm::mat4);
	else
		sizeBuffer = sizeof(UniformBlock);

	uniformBuffer = std::make_unique<Buffer>(
		device,
		sizeBuffer,
		1,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
	);

	uniformBuffer->map();

	bufferCreated = true;
}

void Node::updateUniformBuffer()
{
}

void Node::updateStaticUniform(glm::mat4& worldMatrix)
{
	uniformBuffer->writeToBuffer(&worldMatrix, sizeof(glm::mat4), 0);
}

void Node::updateSkinnedUniforms(const glm::mat4& worldMatrix)
{
	uniformBlock.matrix = worldMatrix;

	// Update join matrices
	glm::mat4 inverseTransform = glm::inverse(worldMatrix);

	size_t numJoints = std::min((uint32_t)skin->joints.size(), MAX_NUM_JOINTS);

	for (size_t i = 0; i < numJoints; i++)
	{
		Node* jointNode = skin->joints[i];
		glm::mat4 jointMat = jointNode->getMatrix() * skin->inverseBindMatrices[i];
		jointMat = inverseTransform * jointMat;
		uniformBlock.jointMatrix[i] = jointMat;
	}

	uniformBlock.jointcount = static_cast<uint32_t>(numJoints);


	uniformBuffer->writeToBuffer(&uniformBlock, sizeof(uniformBlock), 0);
}
