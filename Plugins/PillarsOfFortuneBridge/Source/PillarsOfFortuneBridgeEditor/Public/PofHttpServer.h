#pragma once

#include "CoreMinimal.h"

class IHttpRouter;

/**
 * FPofHttpServer - HTTP server for the PoF Bridge plugin.
 * Exposes REST endpoints that the PoF web app connects to.
 */
class FPofHttpServer
{
public:
	void Start(uint32 Port, const FString& AuthToken, const FString& AllowedOrigins);
	void Stop();
	bool IsRunning() const { return bIsRunning; }

private:
	TSharedPtr<IHttpRouter> HttpRouter;
	FString AuthToken;
	FString AllowedOrigins;
	bool bIsRunning = false;
};
