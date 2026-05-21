/*
 * InfoWindow.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for InfoWindow (server -> client notification pack with text
 * and a list of icon-like Components).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClient.h
 * TypeScript twin:      wrapper/src/codecs/client/InfoWindow.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/Component.h"
#include "../shared/MetaString.h"
#include "../shared/PlayerColor.h"

#include "../../../lib/networkPacks/PacksForClient.h"
#include "../../../lib/networkPacks/EInfoWindowMode.h"
#include "../../../lib/texts/MetaString.h"
#include "../../../lib/constants/EntityIdentifiers.h"

namespace {

std::string infoWindowModeToString(EInfoWindowMode mode)
{
	switch (mode)
	{
		case EInfoWindowMode::AUTO: return "AUTO";
		case EInfoWindowMode::MODAL: return "MODAL";
		case EInfoWindowMode::INFO: return "INFO";
	}
	return "MODAL";
}

EInfoWindowMode infoWindowModeFromString(const std::string & s)
{
	if (s == "AUTO") return EInfoWindowMode::AUTO;
	if (s == "MODAL") return EInfoWindowMode::MODAL;
	if (s == "INFO") return EInfoWindowMode::INFO;
	throw std::runtime_error("InfoWindow: unknown mode '" + s + "'");
}

} // namespace

class InfoWindowCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "InfoWindow"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const InfoWindow *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<InfoWindow>();

		if (json["mode"].isString())
			pack->type = infoWindowModeFromString(json["mode"].String());
		pack->text = homamweb::shared::metaStringFromJson(json["text"]);
		pack->components = homamweb::shared::componentsFromJson(json["components"]);
		if (json["player"].isNumber())
			pack->player = homamweb::shared::playerColorFromJson(json["player"]);
		if (json["soundID"].isNumber())
			pack->soundID = static_cast<uint16_t>(json["soundID"].Integer());

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const InfoWindow &>(pack);

		out["type"].String() = typeName();
		out["mode"].String() = infoWindowModeToString(p.type);
		out["text"] = homamweb::shared::metaStringToJson(p.text);
		out["components"] = homamweb::shared::componentsToJson(p.components);
		out["player"] = homamweb::shared::playerColorToJson(p.player);
		out["soundID"].Integer() = static_cast<int64_t>(p.soundID);
	}
};

REGISTER_PACK_CODEC(InfoWindowCodec)
