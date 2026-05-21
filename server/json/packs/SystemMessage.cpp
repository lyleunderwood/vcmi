/*
 * SystemMessage.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for SystemMessage.
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClient.h
 * TypeScript twin:      wrapper/src/codecs/client/SystemMessage.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/MetaString.h"

#include "../../../lib/networkPacks/PacksForClient.h"
#include "../../../lib/texts/MetaString.h"

class SystemMessageCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "SystemMessage"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const SystemMessage *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<SystemMessage>();
		pack->text = homamweb::shared::metaStringFromJson(json["text"]);
		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const SystemMessage &>(pack);

		out["type"].String() = typeName();
		out["text"] = homamweb::shared::metaStringToJson(p.text);
	}
};

REGISTER_PACK_CODEC(SystemMessageCodec)
