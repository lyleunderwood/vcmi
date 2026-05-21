/*
 * QueryReply.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for QueryReply (a client -> server gameplay action).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForServer.h
 * TypeScript twin:      wrapper/src/codecs/server/QueryReply.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/CPackForServerBase.h"

#include "../../../lib/networkPacks/PacksForServer.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class QueryReplyCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "QueryReply"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const QueryReply *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<QueryReply>();
		homamweb::shared::readServerPackBase(json, *pack);

		if (json["qid"].isNumber())
			pack->qid = QueryID(static_cast<int32_t>(json["qid"].Integer()));

		if (json["reply"].isNumber())
			pack->reply = static_cast<int32_t>(json["reply"].Integer());
		else
			pack->reply = std::nullopt;

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const QueryReply &>(pack);

		out["type"].String() = typeName();
		homamweb::shared::writeServerPackBase(p, out);

		out["qid"].Integer() = static_cast<int64_t>(p.qid.getNum());

		if (p.reply.has_value())
			out["reply"].Integer() = static_cast<int64_t>(*p.reply);
	}
};

REGISTER_PACK_CODEC(QueryReplyCodec)
