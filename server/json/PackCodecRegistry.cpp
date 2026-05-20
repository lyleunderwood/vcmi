/*
 * PackCodecRegistry.cpp, part of homam-web fork of VCMI engine.
 */
#include "StdInc.h"

#include "PackCodecRegistry.h"

#include "../../lib/networkPacks/NetPacksBase.h"

PackCodecRegistry & PackCodecRegistry::instance()
{
	static PackCodecRegistry singleton;
	return singleton;
}

void PackCodecRegistry::registerCodec(std::unique_ptr<PackCodec> codec)
{
	codecs.push_back(std::move(codec));
}

const PackCodec * PackCodecRegistry::findByTypeName(const std::string & typeName) const
{
	for (const auto & codec : codecs)
		if (codec->typeName() == typeName)
			return codec.get();
	return nullptr;
}

const PackCodec * PackCodecRegistry::findByPack(const CPack & pack) const
{
	for (const auto & codec : codecs)
		if (codec->matches(pack))
			return codec.get();
	return nullptr;
}
