#include "ModelNode.h"

#include <glm/gtc/matrix_transform.hpp>

Node::~Node()
{
	for (auto& child : children) {
		delete child;
	}
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