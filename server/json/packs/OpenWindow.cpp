/*
 * OpenWindow.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for OpenWindow (server -> client query telling the client
 * to open a particular adventure-map sub-window: shipyard, marketplace,
 * thieves' guild, etc.).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClient.h
 * TypeScript twin:      wrapper/src/codecs/client/OpenWindow.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"

#include "../../../lib/networkPacks/PacksForClient.h"
#include "../../../lib/networkPacks/EOpenWindowMode.h"
#include "../../../lib/constants/EntityIdentifiers.h"

namespace
{
std::string windowToString(EOpenWindowMode m)
{
	switch (m)
	{
		case EOpenWindowMode::EXCHANGE_WINDOW: return "EXCHANGE_WINDOW";
		case EOpenWindowMode::RECRUITMENT_FIRST: return "RECRUITMENT_FIRST";
		case EOpenWindowMode::RECRUITMENT_ALL: return "RECRUITMENT_ALL";
		case EOpenWindowMode::SHIPYARD_WINDOW: return "SHIPYARD_WINDOW";
		case EOpenWindowMode::THIEVES_GUILD: return "THIEVES_GUILD";
		case EOpenWindowMode::UNIVERSITY_WINDOW: return "UNIVERSITY_WINDOW";
		case EOpenWindowMode::HILL_FORT_WINDOW: return "HILL_FORT_WINDOW";
		case EOpenWindowMode::MARKET_WINDOW: return "MARKET_WINDOW";
		case EOpenWindowMode::PUZZLE_MAP: return "PUZZLE_MAP";
		case EOpenWindowMode::TAVERN_WINDOW: return "TAVERN_WINDOW";
		default: return "EXCHANGE_WINDOW";
	}
}

EOpenWindowMode windowFromString(const std::string & s)
{
	if (s == "EXCHANGE_WINDOW") return EOpenWindowMode::EXCHANGE_WINDOW;
	if (s == "RECRUITMENT_FIRST") return EOpenWindowMode::RECRUITMENT_FIRST;
	if (s == "RECRUITMENT_ALL") return EOpenWindowMode::RECRUITMENT_ALL;
	if (s == "SHIPYARD_WINDOW") return EOpenWindowMode::SHIPYARD_WINDOW;
	if (s == "THIEVES_GUILD") return EOpenWindowMode::THIEVES_GUILD;
	if (s == "UNIVERSITY_WINDOW") return EOpenWindowMode::UNIVERSITY_WINDOW;
	if (s == "HILL_FORT_WINDOW") return EOpenWindowMode::HILL_FORT_WINDOW;
	if (s == "MARKET_WINDOW") return EOpenWindowMode::MARKET_WINDOW;
	if (s == "PUZZLE_MAP") return EOpenWindowMode::PUZZLE_MAP;
	if (s == "TAVERN_WINDOW") return EOpenWindowMode::TAVERN_WINDOW;
	return EOpenWindowMode::EXCHANGE_WINDOW;
}
} // namespace

class OpenWindowCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "OpenWindow"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const OpenWindow *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<OpenWindow>();

		// Inherited from Query.
		if (json["queryID"].isNumber())
			pack->queryID = QueryID(static_cast<int32_t>(json["queryID"].Integer()));

		pack->window = json["window"].isString()
			? windowFromString(json["window"].String())
			: EOpenWindowMode::EXCHANGE_WINDOW;

		if (json["object"].isNumber())
			pack->object = ObjectInstanceID(static_cast<int32_t>(json["object"].Integer()));
		if (json["visitor"].isNumber())
			pack->visitor = ObjectInstanceID(static_cast<int32_t>(json["visitor"].Integer()));

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const OpenWindow &>(pack);

		out["type"].String() = typeName();

		// Inherited from Query.
		out["queryID"].Integer() = static_cast<int64_t>(p.queryID.getNum());

		out["window"].String() = windowToString(p.window);
		out["object"].Integer() = static_cast<int64_t>(p.object.getNum());
		out["visitor"].Integer() = static_cast<int64_t>(p.visitor.getNum());
	}
};

REGISTER_PACK_CODEC(OpenWindowCodec)
