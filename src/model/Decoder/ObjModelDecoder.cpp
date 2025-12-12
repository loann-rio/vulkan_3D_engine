#include "ObjModelDecoder.h"

#include <iostream>
#include <string>
#include <unordered_map>
#include <stdexcept>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include "../../base/Utils.h"

//#define TINYOBJLOADER_IMPLEMENTATION
#include "../../external/tinyobjectloader/tiny_obj_loader.h"


#include "../Vertex/ObjVertexData.h"

namespace std {
	template<>
	struct hash<ObjVertex>
	{
		size_t operator()(ObjVertex const& vertex) const {
			size_t seed = 0;
			hashCombine(seed, vertex.position, vertex.normal, vertex.uv);
			return seed;
		}
	};
}

namespace {

	BoundingBox createAabb(const std::vector<ObjVertex>& vertices)
	{
		BoundingBox aabb{};

		for (const auto& v : vertices) {
			if (!aabb.valid)
			{
				aabb.min = v.position;
				aabb.max = v.position;
				aabb.valid = true;
			}
			else
			{
				aabb.min.x = std::min(aabb.min.x, v.position.x);
				aabb.min.y = std::min(aabb.min.y, v.position.y);
				aabb.min.z = std::min(aabb.min.z, v.position.z);

				aabb.max.x = std::max(aabb.max.x, v.position.x);
				aabb.max.y = std::max(aabb.max.y, v.position.y);
				aabb.max.z = std::max(aabb.max.z, v.position.z);
			}
		}

		if (!aabb.valid) {
			throw std::runtime_error("obj decoder : invalid AABB created from model vertices");
		}

		return aabb;
	}



	std::vector<DecodedMaterial> extractMaterials(const std::string pathRoot, std::vector<tinyobj::material_t>& materialsInput)
	{
		std::vector<DecodedMaterial> materials{};

		for (const auto& m : materialsInput) {
			DecodedMaterial md;
			md.name = m.name;

			if (!m.diffuse_texname.empty())
				md.albedoTexture = pathRoot + "/" + m.diffuse_texname;

			if (!m.bump_texname.empty())
				md.normalTexture = pathRoot + "/" + m.bump_texname;

			md.metallic = 0.0f;
			md.roughness = 1.0f;

			materials.push_back(std::move(md));
		}

		return materials;
	}


	std::vector<Primitive> extractVerticesFromObj(
		std::vector<ObjVertex>& outLocalVertices,
		std::vector<uint32_t>& outLocalIndices,
		std::vector<tinyobj::shape_t>& shapes,
		tinyobj::attrib_t& attrib)
	{

		std::vector<Primitive> primitive_list{};

		std::unordered_map<ObjVertex, uint32_t> uniqueLocalVertices;

		for (const auto& shape : shapes) {

			Primitive primitive;
			primitive.firstIndex = static_cast<uint32_t>(outLocalIndices.size());


			for (const auto& index : shape.mesh.indices)
			{
				ObjVertex vertex{};
				if (index.vertex_index >= 0) {
					vertex.position = {
						attrib.vertices[3 * index.vertex_index + 0],
						attrib.vertices[3 * index.vertex_index + 1],
						attrib.vertices[3 * index.vertex_index + 2],
					};

					if (!attrib.colors.empty() && (size_t)(3 * index.vertex_index + 2) < attrib.colors.size()) {
						vertex.color = {
							attrib.colors[3 * index.vertex_index + 0],
							attrib.colors[3 * index.vertex_index + 1],
							attrib.colors[3 * index.vertex_index + 2],
						};
					}
					else {
						vertex.color = { 1.0f, 1.0f, 1.0f }; // default color
					}
				}

				if (index.normal_index >= 0) {
					vertex.normal = {
						attrib.normals[3 * index.normal_index + 0],
						attrib.normals[3 * index.normal_index + 1],
						attrib.normals[3 * index.normal_index + 2],
					};
				}

				if (index.texcoord_index >= 0) {
					vertex.uv = {
						attrib.texcoords[2 * index.texcoord_index + 0],
						1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
					};
				}


				auto it = uniqueLocalVertices.find(vertex);
				if (it == uniqueLocalVertices.end()) {
					uint32_t newIndex = static_cast<uint32_t>(outLocalVertices.size());
					uniqueLocalVertices.emplace(vertex, newIndex);
					outLocalVertices.push_back(vertex);
					outLocalIndices.push_back(newIndex);
				}
				else {
					outLocalIndices.push_back(it->second);
				}
			}

			primitive.indexCount = static_cast<uint32_t>(outLocalIndices.size()) - primitive.firstIndex;

			if (shape.mesh.material_ids.size())
			{
				int material_id = 0;
				if (shape.mesh.material_ids[0] >= 0)
					material_id = shape.mesh.material_ids[0];


				primitive.materialIndex = static_cast<uint32_t>(material_id);
			}

			primitive_list.push_back(primitive);
		}

		if (outLocalVertices.empty() || outLocalIndices.empty()) {
			throw std::runtime_error("obj decoder : list of vertices is empty");
		}

		return primitive_list;
	}



}

bool ObjModelDecoder::canDecode(const std::filesystem::path& path) const
{
	return (moDecoder::getExtension(path.string()) == "obj");
}

DecodedModel ObjModelDecoder::decode(const std::filesystem::path& path) const
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.string().c_str(), path.parent_path().string().c_str())) {
		std::cerr << warn + err << "\n";
		throw std::runtime_error("obj decoder : could not load model from file");
	}
	

	std::vector<DecodedMaterial> decodedMaterial = extractMaterials(path.parent_path().string(), materials);

	std::vector<ObjVertex> localVertices;
	std::vector<uint32_t> localIndices;
	std::vector<Primitive> primitive_list = extractVerticesFromObj(localVertices, localIndices, shapes, attrib);

	DecodedModel model{};

	// push materials
	model.materials = std::move(decodedMaterial);

	// create global AABB using the local vertices	
	model.aabb = createAabb(localVertices);

	// push local vertices
	model.vertices = std::make_unique<ObjVertexData>(std::move(localVertices));

	// push local indices
	model.indices = std::move(localIndices);

	// push primitives
	model.primitives = std::move(primitive_list);
	
	return model;
}


