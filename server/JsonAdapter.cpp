/*
 * JsonAdapter.cpp, part of VCMI engine
 *
 * JSON-over-TCP ingress for vcmiserver. Listens on a separate port, accepts
 * length-prefixed JSON frames, dispatches via the pack codec registry so
 * adding new pack types means adding a new file under server/json/packs/
 * rather than editing this one.
 */
#include "StdInc.h"

#include "JsonAdapter.h"
#include "CVCMIServer.h"
#include "json/PackCodec.h"
#include "json/PackCodecRegistry.h"

#include "../lib/json/JsonNode.h"
#include "../lib/network/NetworkInterface.h"
#include "../lib/networkPacks/PacksForLobby.h"
#include "../lib/serializer/GameConnection.h"

JsonAdapter::JsonAdapter(CVCMIServer & srv) : server(srv) {}
JsonAdapter::~JsonAdapter() = default;

uint16_t JsonAdapter::start(uint16_t port)
{
	listener = server.getNetworkHandler().createServerTCP(*this);
	auto bound = listener->start(port);
	logNetwork->info("[JsonAdapter] Listening for JSON connections on port %d", bound);
	return bound;
}

bool JsonAdapter::ownsConnection(const std::shared_ptr<GameConnection> & game) const
{
	for (const auto & pair : jsonConnections)
		if (pair.second == game)
			return true;
	return false;
}

void JsonAdapter::onNewConnection(const std::shared_ptr<INetworkConnection> & connection)
{
	logNetwork->info("[JsonAdapter] New JSON client connected");
	auto game = std::make_shared<GameConnection>(connection);
	game->enterLobbyConnectionMode();
	jsonConnections.emplace_back(connection, game);
	server.activeConnections.push_back(game);
}

void JsonAdapter::onDisconnected(const std::shared_ptr<INetworkConnection> & connection, const std::string & errorMessage)
{
	logNetwork->info("[JsonAdapter] JSON client disconnected: %s", errorMessage);
	for (auto it = jsonConnections.begin(); it != jsonConnections.end(); ++it)
	{
		if (it->first == connection)
		{
			auto game = it->second;
			jsonConnections.erase(it);
			LobbyClientDisconnected lcd;
			lcd.clientId = game->connectionID;
			server.handleReceivedPack(game, lcd);
			return;
		}
	}
}

void JsonAdapter::onPacketReceived(const std::shared_ptr<INetworkConnection> & connection, const std::vector<std::byte> & message)
{
	std::shared_ptr<GameConnection> game;
	for (auto & pair : jsonConnections)
		if (pair.first == connection) { game = pair.second; break; }

	if (!game)
	{
		logNetwork->error("[JsonAdapter] packet from unknown connection; dropping");
		connection->close();
		return;
	}

	try
	{
		JsonNode root(reinterpret_cast<const std::byte *>(message.data()), message.size(), "<json-adapter>");
		if (!root.isStruct() || !root["type"].isString())
			throw std::runtime_error("JSON message missing 'type' field");

		const std::string & packType = root["type"].String();
		logNetwork->info("[JsonAdapter] received pack type='%s'", packType);

		const PackCodec * codec = PackCodecRegistry::instance().findByTypeName(packType);
		if (codec == nullptr)
			throw std::runtime_error("No codec registered for pack type: " + packType);

		std::unique_ptr<CPack> rawPack = codec->fromJson(root);
		auto * lobbyPack = dynamic_cast<CPackForLobby *>(rawPack.get());
		if (lobbyPack == nullptr)
			throw std::runtime_error("Pack type '" + packType + "' is not a CPackForLobby");

		server.handleReceivedPack(game, *lobbyPack);
		logNetwork->info("[JsonAdapter] dispatch complete: type='%s'", packType);
	}
	catch (const std::exception & e)
	{
		logNetwork->error("[JsonAdapter] error handling packet: %s. Dropping connection.", e.what());
		try { connection->close(); } catch (...) {}
	}
}

void JsonAdapter::sendPackToJsonClient(const std::shared_ptr<GameConnection> & game, CPackForLobby & pack)
{
	sendPackToJsonClientImpl(game, pack);
}

void JsonAdapter::sendPackToJsonClient(const std::shared_ptr<GameConnection> & game, CPackForClient & pack)
{
	sendPackToJsonClientImpl(game, pack);
}

void JsonAdapter::sendPackToJsonClientImpl(const std::shared_ptr<GameConnection> & game, CPack & pack)
{
	std::shared_ptr<INetworkConnection> sock;
	for (const auto & pair : jsonConnections)
		if (pair.second == game) { sock = pair.first; break; }
	if (!sock)
		return;

	JsonNode out;
	out.Struct(); // ensure struct type

	const PackCodec * codec = PackCodecRegistry::instance().findByPack(pack);
	if (codec != nullptr)
	{
		codec->toJson(pack, out);
	}
	else
	{
		// No outbound codec for this pack subtype yet. Emit a minimal envelope
		// so the JSON client knows something happened.
		out["type"].String() = "Unsupported";
		out["cppType"].String() = typeid(pack).name();
	}

	std::string body = out.toCompactString();
	std::vector<std::byte> payload(body.size());
	std::memcpy(payload.data(), body.data(), body.size());
	logNetwork->info("[JsonAdapter] outbound: %s", body);
	sock->sendPacket(payload); // NetworkConnection::sendPacket prepends the 4-byte size header itself
}
