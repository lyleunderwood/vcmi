/*
 * JsonAdapter.cpp, part of VCMI engine
 *
 * Spike: minimal JSON-over-TCP ingress for vcmiserver.
 */
#include "StdInc.h"

#include "JsonAdapter.h"
#include "CVCMIServer.h"

#include "../lib/json/JsonNode.h"
#include "../lib/network/NetworkInterface.h"
#include "../lib/networkPacks/PacksForLobby.h"
#include "../lib/serializer/GameConnection.h"
#include "../lib/StartInfo.h"

JsonAdapter::JsonAdapter(CVCMIServer & srv) : server(srv) {}
JsonAdapter::~JsonAdapter() = default;

bool JsonAdapter::ownsConnection(const std::shared_ptr<GameConnection> & game) const
{
	for (const auto & pair : jsonConnections)
		if (pair.second == game)
			return true;
	return false;
}

static std::string modeToString(EStartMode m)
{
	switch (m)
	{
		case EStartMode::NEW_GAME: return "NEW_GAME";
		case EStartMode::LOAD_GAME: return "LOAD_GAME";
		case EStartMode::CAMPAIGN: return "CAMPAIGN";
		default: return "INVALID";
	}
}

void JsonAdapter::sendPackToJsonClient(const std::shared_ptr<GameConnection> & game, CPackForLobby & pack)
{
	std::shared_ptr<INetworkConnection> sock;
	for (const auto & pair : jsonConnections)
		if (pair.second == game) { sock = pair.first; break; }
	if (!sock)
		return;

	JsonNode out;
	out.Struct(); // ensure struct type

	// Translate supported pack subtypes.
	if (auto * p = dynamic_cast<LobbyClientConnected *>(&pack))
	{
		out["type"].String() = "LobbyClientConnected";
		out["uuid"].String() = p->uuid;
		JsonNode & names = out["names"];
		names.Vector();
		for (const auto & n : p->names)
		{
			JsonNode v;
			v.String() = n;
			names.Vector().push_back(v);
		}
		out["mode"].String() = modeToString(p->mode);
		out["clientId"].Integer() = static_cast<int64_t>(p->clientId);
		out["hostClientId"].Integer() = static_cast<int64_t>(p->hostClientId);
		out["version"].Integer() = static_cast<int64_t>(p->version);
	}
	else if (auto * p = dynamic_cast<LobbyClientDisconnected *>(&pack))
	{
		out["type"].String() = "LobbyClientDisconnected";
		out["clientId"].Integer() = static_cast<int64_t>(p->clientId);
		out["shutdownServer"].Bool() = p->shutdownServer;
	}
	else
	{
		// Unsupported pack subtype: emit a minimal envelope so the client knows
		// something happened without us having to translate every field.
		out["type"].String() = "Unsupported";
		out["cppType"].String() = typeid(pack).name();
	}

	std::string body = out.toCompactString();
	std::vector<std::byte> payload(body.size());
	std::memcpy(payload.data(), body.data(), body.size());
	logNetwork->info("[JsonAdapter] outbound: %s", body);
	sock->sendPacket(payload); // NetworkConnection::sendPacket prepends the 4-byte size header itself
}

uint16_t JsonAdapter::start(uint16_t port)
{
	listener = server.getNetworkHandler().createServerTCP(*this);
	auto bound = listener->start(port);
	logNetwork->info("[JsonAdapter] Listening for JSON connections on port %d", bound);
	return bound;
}

void JsonAdapter::onNewConnection(const std::shared_ptr<INetworkConnection> & connection)
{
	logNetwork->info("[JsonAdapter] New JSON client connected");
	// Wrap in a GameConnection so the lobby pipeline can address us symmetrically.
	auto game = std::make_shared<GameConnection>(connection);
	game->enterLobbyConnectionMode();
	jsonConnections.emplace_back(connection, game);
	// Also register in the main server so announcePack / lobby state propagation work.
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
			// Match the binary path: synthesize LobbyClientDisconnected.
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

		if (packType == "LobbyClientConnected")
		{
			LobbyClientConnected pack;
			pack.uuid = root["uuid"].isString() ? root["uuid"].String() : "json-client";
			if (root["names"].isVector())
				for (const auto & n : root["names"].Vector())
					pack.names.push_back(n.String());
			if (pack.names.empty())
				pack.names.push_back("JsonPlayer");
			// Default to NEW_GAME if mode not supplied.
			pack.mode = EStartMode::NEW_GAME;
			if (root["mode"].isString())
			{
				const auto & m = root["mode"].String();
				if (m == "NEW_GAME") pack.mode = EStartMode::NEW_GAME;
				else if (m == "LOAD_GAME") pack.mode = EStartMode::LOAD_GAME;
				else if (m == "CAMPAIGN") pack.mode = EStartMode::CAMPAIGN;
			}
			server.handleReceivedPack(game, pack);
			logNetwork->info("[JsonAdapter] dispatch complete: clientId=%d hostClientId=%d",
				static_cast<int>(pack.clientId), static_cast<int>(pack.hostClientId));
		}
		else if (packType == "LobbyClientDisconnected")
		{
			LobbyClientDisconnected pack;
			pack.clientId = game->connectionID;
			pack.shutdownServer = root["shutdownServer"].Bool();
			server.handleReceivedPack(game, pack);
		}
		else
		{
			throw std::runtime_error("Unsupported pack type: " + packType);
		}
	}
	catch (const std::exception & e)
	{
		logNetwork->error("[JsonAdapter] error handling packet: %s. Dropping connection.", e.what());
		try { connection->close(); } catch (...) {}
	}
}
