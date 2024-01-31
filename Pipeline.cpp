#include "Pipeline.h"

#include <fstream>
#include <iostream>
#include <stdexcept>

Pipeline::Pipeline(const std::string& vertFilepath, const std::string& fragFilepath)
{
	createGraphicsPipeline(vertFilepath, fragFilepath);
}

std::vector<char> Pipeline::readFile(const std::string& filepath)
{
	std::ifstream file(filepath, std::ios::ate | std::ios::binary);
	if (!file.is_open()) {
		throw std::runtime_error("failed to open file: " + filepath);
	}

	size_t fileSize = static_cast<rsize_t>(file.tellg());
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();
	return buffer;

}

void Pipeline::createGraphicsPipeline(const std::string& vertFilepath, const std::string& fragFilepath)
{
	auto vertCode = readFile(vertFilepath);
	auto fragCode = readFile(fragFilepath);

	std::cout << "vert shadder code size: " << vertCode.size() << "\n";
	std::cout << "frag shadder code size: " << fragCode.size() << "\n";
}
