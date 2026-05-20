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
struct CPackForLobby;
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

	/// Serialize a lobby pack to JSON and send to the given JSON-mode connection.
	/// No-op if the connection is not JSON-mode or the pack type is unsupported.
	void sendPackToJsonClient(const std::shared_ptr<GameConnection> & game, CPackForLobby & pack);

	void onNewConnection(const std::shared_ptr<INetworkConnection> & connection) override;
	void onPacketReceived(const std::shared_ptr<INetworkConnection> & connection, const std::vector<std::byte> & message) override;
	void onDisconnected(const std::shared_ptr<INetworkConnection> & connection, const std::string & errorMessage) override;
};
