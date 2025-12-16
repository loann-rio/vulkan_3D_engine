#include "IModelDecoder.h"

#include <vector>

#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "../../external/tiny_gltf.h"
//#include "../ModelNode.h"
#include "../Vertex/GltfVertexData.h"


class Node;

class GlTFModelDecoder : public IModelDecoder {
public:
	bool canDecode(const std::filesystem::path& path) const override;
	DecodedModel decode(const std::filesystem::path& path) const override;

private:

	static void loadNode(
		Node* parentNode,
		const tinygltf::Node node,
		size_t nodeIndex,
		tinygltf::Model gltfModel,
		std::vector<GltfVertex>& localVertices,
		std::vector<uint32_t>& localIndices,
		std::vector<std::unique_ptr<Node>>& nodes);


};