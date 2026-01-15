#include "IVertexLayout.h"

bool IVertexLayout::isCompatibleWith(const IVertexLayout& other) const
{
    const auto& required = attributes();
    const auto& provided = other.attributes();

    for (const auto& req : required)
    {
        auto it = std::find_if(
            provided.begin(),
            provided.end(),
            [&](const Attribute& a) {
                return a.name == req.name &&
                    a.format == req.format &&
                    a.size == req.size;
            });

        if (it == provided.end())
            return false;
    }

    return true;
}
