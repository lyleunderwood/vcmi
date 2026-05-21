/*
 * NewTurn.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for NewTurn (server -> client: announces a new game turn with
 * per-hero movement/mana resets, dwelling refills, per-player resource
 * income, and optional weekly rumor + popup notification).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClient.h (NewTurn)
 * TypeScript twin:      wrapper/src/codecs/client/NewTurn.ts
 *
 * Nested sub-structs (SetMovePoints / SetMana / SetAvailableCreatures /
 * ResourceSet / RumorState / InfoWindow body) are inlined here as field
 * sub-codecs. If reused by other packs, promote to shared/.
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/Component.h"
#include "../shared/MetaString.h"
#include "../shared/PlayerColor.h"

#include "../../../lib/networkPacks/PacksForClient.h"
#include "../../../lib/networkPacks/EInfoWindowMode.h"
#include "../../../lib/gameState/RumorState.h"
#include "../../../lib/constants/EntityIdentifiers.h"
#include "../../../lib/constants/Enumerations.h"
#include "../../../lib/ResourceSet.h"
#include "../../../lib/texts/MetaString.h"

namespace
{

// ---- EWeekType ----------------------------------------------------------
std::string weekTypeToString(EWeekType t)
{
	switch (t)
	{
		case EWeekType::FIRST_WEEK:    return "FIRST_WEEK";
		case EWeekType::NORMAL:        return "NORMAL";
		case EWeekType::DOUBLE_GROWTH: return "DOUBLE_GROWTH";
		case EWeekType::BONUS_GROWTH:  return "BONUS_GROWTH";
		case EWeekType::DEITYOFFIRE:   return "DEITYOFFIRE";
		case EWeekType::PLAGUE:        return "PLAGUE";
	}
	return "NORMAL";
}

EWeekType weekTypeFromString(const std::string & s)
{
	if (s == "FIRST_WEEK")    return EWeekType::FIRST_WEEK;
	if (s == "NORMAL")        return EWeekType::NORMAL;
	if (s == "DOUBLE_GROWTH") return EWeekType::DOUBLE_GROWTH;
	if (s == "BONUS_GROWTH")  return EWeekType::BONUS_GROWTH;
	if (s == "DEITYOFFIRE")   return EWeekType::DEITYOFFIRE;
	if (s == "PLAGUE")        return EWeekType::PLAGUE;
	throw std::runtime_error("NewTurn: unknown specialWeek '" + s + "'");
}

// ---- ChangeValueMode ----------------------------------------------------
std::string changeValueModeToString(ChangeValueMode m)
{
	switch (m)
	{
		case ChangeValueMode::RELATIVE: return "RELATIVE";
		case ChangeValueMode::ABSOLUTE: return "ABSOLUTE";
	}
	return "RELATIVE";
}

ChangeValueMode changeValueModeFromString(const std::string & s)
{
	if (s == "ABSOLUTE") return ChangeValueMode::ABSOLUTE;
	return ChangeValueMode::RELATIVE;
}

// ---- EInfoWindowMode ----------------------------------------------------
std::string infoWindowModeToString(EInfoWindowMode mode)
{
	switch (mode)
	{
		case EInfoWindowMode::AUTO:  return "AUTO";
		case EInfoWindowMode::MODAL: return "MODAL";
		case EInfoWindowMode::INFO:  return "INFO";
	}
	return "MODAL";
}

EInfoWindowMode infoWindowModeFromString(const std::string & s)
{
	if (s == "AUTO")  return EInfoWindowMode::AUTO;
	if (s == "MODAL") return EInfoWindowMode::MODAL;
	if (s == "INFO")  return EInfoWindowMode::INFO;
	throw std::runtime_error("NewTurn: unknown InfoWindow mode '" + s + "'");
}

// ---- ERumorType ---------------------------------------------------------
std::string rumorTypeToString(RumorState::ERumorType t)
{
	switch (t)
	{
		case RumorState::TYPE_NONE:    return "TYPE_NONE";
		case RumorState::TYPE_RAND:    return "TYPE_RAND";
		case RumorState::TYPE_SPECIAL: return "TYPE_SPECIAL";
		case RumorState::TYPE_MAP:     return "TYPE_MAP";
	}
	return "TYPE_NONE";
}

RumorState::ERumorType rumorTypeFromString(const std::string & s)
{
	if (s == "TYPE_NONE")    return RumorState::TYPE_NONE;
	if (s == "TYPE_RAND")    return RumorState::TYPE_RAND;
	if (s == "TYPE_SPECIAL") return RumorState::TYPE_SPECIAL;
	if (s == "TYPE_MAP")     return RumorState::TYPE_MAP;
	throw std::runtime_error("NewTurn: unknown rumor type '" + s + "'");
}

// ---- SetMovePoints entry -----------------------------------------------
JsonNode setMovePointsToJson(const SetMovePoints & p)
{
	JsonNode out;
	out.Struct();
	out["hid"].Integer() = static_cast<int64_t>(p.hid.getNum());
	out["val"].Integer() = static_cast<int64_t>(p.val);
	return out;
}

SetMovePoints setMovePointsFromJson(const JsonNode & json)
{
	SetMovePoints p;
	if (json["hid"].isNumber())
		p.hid = ObjectInstanceID(static_cast<int32_t>(json["hid"].Integer()));
	if (json["val"].isNumber())
		p.val = static_cast<si32>(json["val"].Integer());
	return p;
}

// ---- SetMana entry ------------------------------------------------------
JsonNode setManaToJson(const SetMana & p)
{
	JsonNode out;
	out.Struct();
	out["hid"].Integer() = static_cast<int64_t>(p.hid.getNum());
	out["val"].Integer() = static_cast<int64_t>(p.val);
	out["mode"].String() = changeValueModeToString(p.mode);
	return out;
}

SetMana setManaFromJson(const JsonNode & json)
{
	SetMana p;
	if (json["hid"].isNumber())
		p.hid = ObjectInstanceID(static_cast<int32_t>(json["hid"].Integer()));
	if (json["val"].isNumber())
		p.val = static_cast<si32>(json["val"].Integer());
	if (json["mode"].isString())
		p.mode = changeValueModeFromString(json["mode"].String());
	return p;
}

// ---- SetAvailableCreatures entry ---------------------------------------
JsonNode setAvailableCreaturesToJson(const SetAvailableCreatures & p)
{
	JsonNode out;
	out.Struct();
	out["tid"].Integer() = static_cast<int64_t>(p.tid.getNum());
	JsonNode & creatures = out["creatures"];
	creatures.Vector();
	for (const auto & pair : p.creatures)
	{
		JsonNode entry;
		entry.Struct();
		entry["level"].Integer() = static_cast<int64_t>(pair.first);
		JsonNode & ids = entry["ids"];
		ids.Vector();
		for (const auto & cid : pair.second)
		{
			JsonNode v;
			v.Integer() = static_cast<int64_t>(cid.getNum());
			ids.Vector().push_back(v);
		}
		creatures.Vector().push_back(entry);
	}
	return out;
}

SetAvailableCreatures setAvailableCreaturesFromJson(const JsonNode & json)
{
	SetAvailableCreatures p;
	if (json["tid"].isNumber())
		p.tid = ObjectInstanceID(static_cast<int32_t>(json["tid"].Integer()));
	if (json["creatures"].isVector())
	{
		for (const auto & entry : json["creatures"].Vector())
		{
			ui32 level = 0;
			if (entry["level"].isNumber())
				level = static_cast<ui32>(entry["level"].Integer());
			std::vector<CreatureID> ids;
			if (entry["ids"].isVector())
			{
				for (const auto & v : entry["ids"].Vector())
					ids.emplace_back(static_cast<int32_t>(v.Integer()));
			}
			p.creatures.emplace_back(level, ids);
		}
	}
	return p;
}

// ---- ResourceSet --------------------------------------------------------
JsonNode resourceSetToJson(const ResourceSet & res)
{
	JsonNode out;
	out.Vector();
	for (size_t i = 0; i < res.size(); ++i)
	{
		JsonNode entry;
		entry.Integer() = static_cast<int64_t>(res[i]);
		out.Vector().push_back(entry);
	}
	return out;
}

ResourceSet resourceSetFromJson(const JsonNode & json)
{
	ResourceSet res;
	if (json.isVector())
	{
		const auto & vec = json.Vector();
		for (size_t i = 0; i < vec.size() && i < res.size(); ++i)
			res[i] = static_cast<TResource>(vec[i].Integer());
	}
	return res;
}

// ---- RumorState ---------------------------------------------------------
JsonNode rumorStateToJson(const RumorState & r)
{
	JsonNode out;
	out.Struct();
	out["type"].String() = rumorTypeToString(r.type);
	JsonNode & last = out["last"];
	last.Vector();
	for (const auto & kv : r.last)
	{
		JsonNode entry;
		entry.Struct();
		entry["rumorType"].String() = rumorTypeToString(kv.first);
		entry["id"].Integer() = static_cast<int64_t>(kv.second.first);
		entry["extra"].Integer() = static_cast<int64_t>(kv.second.second);
		last.Vector().push_back(entry);
	}
	return out;
}

RumorState rumorStateFromJson(const JsonNode & json)
{
	RumorState r;
	if (json["type"].isString())
		r.type = rumorTypeFromString(json["type"].String());
	if (json["last"].isVector())
	{
		for (const auto & entry : json["last"].Vector())
		{
			RumorState::ERumorType key = RumorState::TYPE_NONE;
			if (entry["rumorType"].isString())
				key = rumorTypeFromString(entry["rumorType"].String());
			int id = 0;
			int extra = 0;
			if (entry["id"].isNumber())
				id = static_cast<int>(entry["id"].Integer());
			if (entry["extra"].isNumber())
				extra = static_cast<int>(entry["extra"].Integer());
			r.last[key] = std::make_pair(id, extra);
		}
	}
	return r;
}

// ---- InfoWindow body (no top-level "type" discriminator) ---------------
JsonNode infoWindowBodyToJson(const InfoWindow & iw)
{
	JsonNode out;
	out.Struct();
	out["mode"].String() = infoWindowModeToString(iw.type);
	out["text"] = homamweb::shared::metaStringToJson(iw.text);
	out["components"] = homamweb::shared::componentsToJson(iw.components);
	out["player"] = homamweb::shared::playerColorToJson(iw.player);
	out["soundID"].Integer() = static_cast<int64_t>(iw.soundID);
	return out;
}

InfoWindow infoWindowBodyFromJson(const JsonNode & json)
{
	InfoWindow iw;
	if (json["mode"].isString())
		iw.type = infoWindowModeFromString(json["mode"].String());
	iw.text = homamweb::shared::metaStringFromJson(json["text"]);
	iw.components = homamweb::shared::componentsFromJson(json["components"]);
	if (json["player"].isNumber())
		iw.player = homamweb::shared::playerColorFromJson(json["player"]);
	if (json["soundID"].isNumber())
		iw.soundID = static_cast<uint16_t>(json["soundID"].Integer());
	return iw;
}

} // namespace

class NewTurnCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "NewTurn"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const NewTurn *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<NewTurn>();

		if (json["day"].isNumber())
			pack->day = static_cast<ui32>(json["day"].Integer());

		if (json["creatureid"].isNumber())
			pack->creatureid = CreatureID(static_cast<int32_t>(json["creatureid"].Integer()));

		if (json["specialWeek"].isString())
			pack->specialWeek = weekTypeFromString(json["specialWeek"].String());

		if (json["heroesMovement"].isVector())
		{
			for (const auto & entry : json["heroesMovement"].Vector())
				pack->heroesMovement.push_back(setMovePointsFromJson(entry));
		}

		if (json["heroesMana"].isVector())
		{
			for (const auto & entry : json["heroesMana"].Vector())
				pack->heroesMana.push_back(setManaFromJson(entry));
		}

		if (json["availableCreatures"].isVector())
		{
			for (const auto & entry : json["availableCreatures"].Vector())
				pack->availableCreatures.push_back(setAvailableCreaturesFromJson(entry));
		}

		if (json["playerIncome"].isVector())
		{
			for (const auto & entry : json["playerIncome"].Vector())
			{
				PlayerColor player = PlayerColor::NEUTRAL;
				if (entry["player"].isNumber())
					player = homamweb::shared::playerColorFromJson(entry["player"]);
				ResourceSet res = resourceSetFromJson(entry["res"]);
				pack->playerIncome[player] = res;
			}
		}

		if (!json["newRumor"].isNull())
			pack->newRumor = rumorStateFromJson(json["newRumor"]);

		if (!json["newWeekNotification"].isNull())
			pack->newWeekNotification = infoWindowBodyFromJson(json["newWeekNotification"]);

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const NewTurn &>(pack);

		out["type"].String() = typeName();
		out["day"].Integer() = static_cast<int64_t>(p.day);
		out["creatureid"].Integer() = static_cast<int64_t>(p.creatureid.getNum());
		out["specialWeek"].String() = weekTypeToString(p.specialWeek);

		JsonNode & heroesMovement = out["heroesMovement"];
		heroesMovement.Vector();
		for (const auto & e : p.heroesMovement)
			heroesMovement.Vector().push_back(setMovePointsToJson(e));

		JsonNode & heroesMana = out["heroesMana"];
		heroesMana.Vector();
		for (const auto & e : p.heroesMana)
			heroesMana.Vector().push_back(setManaToJson(e));

		JsonNode & availableCreatures = out["availableCreatures"];
		availableCreatures.Vector();
		for (const auto & e : p.availableCreatures)
			availableCreatures.Vector().push_back(setAvailableCreaturesToJson(e));

		JsonNode & playerIncome = out["playerIncome"];
		playerIncome.Vector();
		for (const auto & kv : p.playerIncome)
		{
			JsonNode entry;
			entry.Struct();
			entry["player"] = homamweb::shared::playerColorToJson(kv.first);
			entry["res"] = resourceSetToJson(kv.second);
			playerIncome.Vector().push_back(entry);
		}

		if (p.newRumor.has_value())
			out["newRumor"] = rumorStateToJson(*p.newRumor);
		else
			out["newRumor"].setType(JsonNode::JsonType::DATA_NULL);

		if (p.newWeekNotification.has_value())
			out["newWeekNotification"] = infoWindowBodyToJson(*p.newWeekNotification);
		else
			out["newWeekNotification"].setType(JsonNode::JsonType::DATA_NULL);
	}
};

REGISTER_PACK_CODEC(NewTurnCodec)
