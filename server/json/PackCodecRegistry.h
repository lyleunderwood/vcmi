/*
 * PackCodecRegistry.h, part of homam-web fork of VCMI engine.
 *
 * Singleton registry of pack codecs. Each codec self-registers at static
 * initialization time using REGISTER_PACK_CODEC(MyCodecClass).
 */
#pragma once

#include "PackCodec.h"

VCMI_LIB_NAMESPACE_BEGIN
struct CPack;
VCMI_LIB_NAMESPACE_END

class PackCodecRegistry
{
	std::vector<std::unique_ptr<PackCodec>> codecs;

public:
	static PackCodecRegistry & instance();

	void registerCodec(std::unique_ptr<PackCodec> codec);

	/// Find a codec by its wire type name (used for inbound dispatch).
	/// Returns nullptr if no codec is registered for that type.
	const PackCodec * findByTypeName(const std::string & typeName) const;

	/// Find a codec that can serialize the given pack (used for outbound dispatch).
	/// Returns nullptr if no codec matches.
	const PackCodec * findByPack(const CPack & pack) const;
};

/// Static-init registration helper. Drop one of these in each pack codec .cpp file:
///   REGISTER_PACK_CODEC(LobbyClientConnectedCodec)
#define REGISTER_PACK_CODEC(CodecClass) \
	namespace { \
		struct CodecClass##Registrar { \
			CodecClass##Registrar() { \
				PackCodecRegistry::instance().registerCodec(std::make_unique<CodecClass>()); \
			} \
		}; \
		static CodecClass##Registrar CodecClass##_registrar_; \
	}
