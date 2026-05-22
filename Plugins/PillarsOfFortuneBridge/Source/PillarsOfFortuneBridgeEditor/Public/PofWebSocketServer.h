#pragma once

#include "CoreMinimal.h"
#include "IWebSocketServer.h"
#include "PofWebSocketServer.generated.h"
class INetworkingWebSocket;

/**
 * FPofWebSocketLiveState — Bidirectional WebSocket server for live editor state sync.
 *
 * Runs alongside PofHttpServer, streaming delta-compressed editor state
 * (viewport, selection, PIE, properties) to the PoF web app in real-time.
 *
 * Protocol (JSON messages):
 *   Inbound  (Web → UE5): subscribe.property, unsubscribe.property, set.property, request.snapshot, ping
 *   Outbound (UE5 → Web): state.snapshot, state.delta, property.update, event.pie, event.selection, pong
 */
UCLASS()
class UPofWebSocketLiveState : public UObject, public FTickableGameObject
{
	GENERATED_BODY()

public:
	/** Start the WebSocket server on the given port. */
	void Start(uint32 Port, const FString& AllowedOrigins);

	/** Stop the server and disconnect all clients. */
	void Stop();

	bool IsRunning() const { return bIsRunning; }

	// ── FTickableGameObject ──
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override { return bIsRunning; }
	virtual bool IsTickableInEditor() const override { return true; }

private:
	// ── WebSocket lifecycle ──
	void OnClientConnected(INetworkingWebSocket* Client);
	void OnClientDisconnected(INetworkingWebSocket* Client);
	void OnMessageReceived(void* Data, int32 DataSize, INetworkingWebSocket* Client);

	// ── State collection ──
	FString BuildFullSnapshot() const;
	FString BuildDeltaSnapshot();

	// ── Property watching ──
	struct FPropertyWatch
	{
		FString WatchId;
		FString ObjectPath;
		FString PropertyName;
		float IntervalSeconds;
		float TimeSinceLastPoll;
		FString LastValueJson;
	};

	// ── Send helpers ──
	void BroadcastToAll(const FString& Message);
	void SendToClient(INetworkingWebSocket* Client, const FString& Message);
	void HandleInboundMessage(const FString& Message, INetworkingWebSocket* Client);

	// ── Editor state cache for delta detection ──
	FString LastEditorState;
	FVector LastCameraLocation = FVector::ZeroVector;
	FRotator LastCameraRotation = FRotator::ZeroRotator;
	float LastFOV = 90.0f;
	FString LastViewMode;
	FString LastOpenLevel;
	int32 LastSelectedActorCount = 0;

	TUniquePtr<IWebSocketServer> Server;
	TArray<INetworkingWebSocket*> ConnectedClients;
	TArray<FPropertyWatch> PropertyWatches;
	FString AllowedOrigins;
	float SnapshotAccumulator = 0.0f;
	float SnapshotInterval = 0.5f; // seconds between delta broadcasts
	bool bIsRunning = false;
};
