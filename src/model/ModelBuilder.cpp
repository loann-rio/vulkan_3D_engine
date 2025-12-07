#include "ModelBuilder.h"

ModelBuilder::ModelBuilder(Device& device) : device(device)
{
}

uint64_t ModelBuilder::hash() const
{
    return 0;
}

std::unique_ptr<ModelAsset> ModelBuilder::build()
{
    return std::unique_ptr<ModelAsset>();
}
