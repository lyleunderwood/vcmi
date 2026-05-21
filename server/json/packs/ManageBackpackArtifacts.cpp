/*
 * ManageBackpackArtifacts.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for ManageBackpackArtifacts (a client -> server gameplay action).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForServer.h
 * TypeScript twin:      wrapper/src/codecs/server/ManageBackpackArtifacts.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/CPackForServerBase.h"

#include "../../../lib/networkPacks/PacksForServer.h"
#include "../../../lib/constants/EntityIdentifiers.h"

namespace
{
using ManageCmd = ManageBackpackArtifacts::ManageCmd;

std::string cmdToString(ManageCmd c)
{
	switch (c)
	{
		case ManageCmd::SCROLL_LEFT: return "SCROLL_LEFT";
		case ManageCmd::SCROLL_RIGHT: return "SCROLL_RIGHT";
		case ManageCmd::SORT_BY_SLOT: return "SORT_BY_SLOT";
		case ManageCmd::SORT_BY_CLASS: return "SORT_BY_CLASS";
		case ManageCmd::SORT_BY_COST: return "SORT_BY_COST";
		default: return "SCROLL_LEFT";
	}
}

ManageCmd cmdFromString(const std::string & s)
{
	if (s == "SCROLL_LEFT") return ManageCmd::SCROLL_LEFT;
	if (s == "SCROLL_RIGHT") return ManageCmd::SCROLL_RIGHT;
	if (s == "SORT_BY_SLOT") return ManageCmd::SORT_BY_SLOT;
	if (s == "SORT_BY_CLASS") return ManageCmd::SORT_BY_CLASS;
	if (s == "SORT_BY_COST") return ManageCmd::SORT_BY_COST;
	return ManageCmd::SCROLL_LEFT;
}
} // namespace

class ManageBackpackArtifactsCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "ManageBackpackArtifacts"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const ManageBackpackArtifacts *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<ManageBackpackArtifacts>();
		homamweb::shared::readServerPackBase(json, *pack);

		if (json["artHolder"].isNumber())
			pack->artHolder = ObjectInstanceID(static_cast<int32_t>(json["artHolder"].Integer()));

		pack->cmd = json["cmd"].isString()
			? cmdFromString(json["cmd"].String())
			: ManageCmd::SCROLL_LEFT;

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const ManageBackpackArtifacts &>(pack);

		out["type"].String() = typeName();
		homamweb::shared::writeServerPackBase(p, out);

		out["artHolder"].Integer() = static_cast<int64_t>(p.artHolder.getNum());
		out["cmd"].String() = cmdToString(p.cmd);
	}
};

REGISTER_PACK_CODEC(ManageBackpackArtifactsCodec)
