#pragma once

#include "IModelDecoder.h"

class GlTFModelDecoder : public IModelDecoder {
public:
	bool canDecode(const std::filesystem::path& path) const override;
	DecodedModel decode(const std::filesystem::path& path) const override;
};
