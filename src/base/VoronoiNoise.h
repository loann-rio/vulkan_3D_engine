#pragma once

#include <vector>
#include <random>

#include <glm/glm.hpp>

class VoronoiNoise {
public:

	static glm::vec2 hash(glm::vec2 p, float seed)
	{
		p = glm::vec2(glm::dot(p, glm::vec2(127.1 + 456.5 * seed, 311.7 + 987.23 * seed)),
			glm::dot(p, glm::vec2(269.5, 183.3)));
		return glm::fract(glm::sin(p) * 18.5453f);
	}

	static std::vector<glm::vec2> voronoi(glm::vec2 x, float sizeFactor, float seed = 1)
	{

		std::vector<glm::vec2> out{ 9, glm::vec2(8.0, 0)};

		glm::vec2 n = floor(x / sizeFactor);
		glm::vec2 f = fract(x / sizeFactor);

		glm::vec3 m = glm::vec3(8.0);
		for (int j = -1; j <= 1; j++)
			for (int i = -1; i <= 1; i++)
			{
				glm::vec2  g = glm::vec2(float(i), float(j));
				glm::vec2  o = hash(n + g, seed);
				glm::vec2  r = g - f + o;
				float d = dot(r, r);

				glm::vec2 contestant{ d, o.x + o.y };
				for (glm::vec2& val : out)
				{
					if (contestant.x < val.x) {
						auto temp = val;
						val = contestant;
						contestant = temp;
					}
				}
			}

		for (glm::vec2& val : out)
			val.x = std::max(0.f, std::min(1.f, 1 - sqrt(val.x)));

		return out;
	}

	static std::vector<std::vector<glm::vec2>> generate2DVornoiMap(uint16_t width, uint16_t height, float sizeFactor) {

		float maxHeight = -FLT_MAX;
		float minHeight = FLT_MAX;

		std::vector<std::vector<glm::vec2>> noiseMap(width, std::vector<glm::vec2>(height));

		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				auto noiseHeight = voronoi({ x, y }, sizeFactor);

				noiseMap[x][y] = glm::vec2{ std::min(1.f, noiseHeight[0].x / 1.18f), noiseHeight[0].y };
			}
		}

		return noiseMap;
	}
};
 