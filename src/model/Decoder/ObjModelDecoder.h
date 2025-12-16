#include "IModelDecoder.h"

#include <vector>

class ObjModelDecoder : public IModelDecoder {
public:
	bool canDecode(const std::filesystem::path& path) const override;
	DecodedModel decode(const std::filesystem::path& path) const override;

};