/*
 * SetResources.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for SetResources.
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClient.h
 * TypeScript twin:      wrapper/src/codecs/client/SetResources.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/PlayerColor.h"

#include "../../../lib/networkPacks/PacksForClient.h"
#include "../../../lib/ResourceSet.h"
#include "../../../lib/constants/Enumerations.h"

namespace
{
std::string changeValueModeToString(ChangeValueMode m)
{
	switch (m)
	{
		case ChangeValueMode::RELATIVE: return "RELATIVE";
		case ChangeValueMode::ABSOLUTE: return "ABSOLUTE";
		default: return "RELATIVE";
	}
}

ChangeValueMode changeValueModeFromString(const std::string & s)
{
	if (s == "ABSOLUTE") return ChangeValueMode::ABSOLUTE;
	return ChangeValueMode::RELATIVE;
}
} // namespace

class SetResourcesCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "SetResources"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const SetResources *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<SetResources>();

		if (json["mode"].isString())
			pack->mode = changeValueModeFromString(json["mode"].String());

		if (json["player"].isNumber())
			pack->player = homamweb::shared::playerColorFromJson(json["player"]);

		if (json["res"].isVector())
		{
			const auto & vec = json["res"].Vector();
			for (size_t i = 0; i < vec.size(); ++i)
				pack->res[i] = static_cast<TResource>(vec[i].Integer());
		}

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const SetResources &>(pack);

		out["type"].String() = typeName();
		out["mode"].String() = changeValueModeToString(p.mode);
		out["player"] = homamweb::shared::playerColorToJson(p.player);

		JsonNode & res = out["res"];
		res.Vector(); // ensure vector type
		for (size_t i = 0; i < p.res.size(); ++i)
		{
			JsonNode entry;
			entry.Integer() = static_cast<int64_t>(p.res[i]);
			res.Vector().push_back(entry);
		}
	}
};

REGISTER_PACK_CODEC(SetResourcesCodec)
