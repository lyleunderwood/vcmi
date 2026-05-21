/*
 * JsonAdapter.h, part of VCMI engine
 *
 * Spike: minimal JSON-over-TCP ingress for vcmiserver.
 * Accepts JSON-framed LobbyClientConnected, feeds it into the existing lobby pipeline.
 */
#pragma once

#include "../lib/network/NetworkInterface.h"

class CVCMIServer;

VCMI_LIB_NAMESPACE_BEGIN
class GameConnection;
class JsonNode;
struct CPack;
struct CPackForLobby;
struct CPackForClient;
VCMI_LIB_NAMESPACE_END

class JsonAdapter final : public INetworkServerListener
{
	CVCMIServer & server;
	std::unique_ptr<INetworkServer> listener;
	std::vector<std::pair<std::shared_ptr<INetworkConnection>, std::shared_ptr<GameConnection>>> jsonConnections;

public:
	explicit JsonAdapter(CVCMIServer & server);
	~JsonAdapter();

	uint16_t start(uint16_t port);

	/// Returns true if the given GameConnection was created by this adapter (JSON-mode).
	bool ownsConnection(const std::shared_ptr<GameConnection> & game) const;

	/// Serialize a pack to JSON and send to the given JSON-mode connection.
	/// Works for any CPack subtype (CPackForLobby, CPackForClient, ...);
	/// dispatches via PackCodecRegistry. No-op if the connection is not JSON-mode
	/// or no codec is registered for the pack type (emits an `Unsupported` envelope
	/// in that case so the wrapper knows something happened).
	void sendPackToJsonClient(const std::shared_ptr<GameConnection> & game, CPackForLobby & pack);
	void sendPackToJsonClient(const std::shared_ptr<GameConnection> & game, CPackForClient & pack);

	void onNewConnection(const std::shared_ptr<INetworkConnection> & connection) override;
	void onPacketReceived(const std::shared_ptr<INetworkConnection> & connection, const std::vector<std::byte> & message) override;
	void onDisconnected(const std::shared_ptr<INetworkConnection> & connection, const std::string & errorMessage) override;

private:
	/// Shared body for the typed overloads above — dispatches via the registry.
	void sendPackToJsonClientImpl(const std::shared_ptr<GameConnection> & game, CPack & pack);
	/// Decorate outbound JSON with engine-state info the codec can't access
	/// (codecs are stateless). E.g. hero positions for HeroVisit, controlled
	/// player slots for LobbyStartGame.
	void enrichOutbound(const std::shared_ptr<GameConnection> & game, const CPack & pack, JsonNode & out);
	/// Handle "Wrapper..." namespaced query packs the wrapper sends to inspect
	/// engine state (map size, regions, etc.). Not real engine packs; never
	/// forwarded to gh. We synthesize a response and send it back.
	void handleWrapperQuery(const std::shared_ptr<INetworkConnection> & sock, const std::string & queryType, const JsonNode & req);
	/// Send raw JSON over a JSON-mode connection (used by wrapper-query responses).
	void sendRawJson(const std::shared_ptr<INetworkConnection> & sock, const JsonNode & json);
};
