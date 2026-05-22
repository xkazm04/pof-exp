#pragma once

#include "CoreMinimal.h"

class FJsonObject;

/**
 * PofHttpRouter - Request parsing utilities for the PoF HTTP server.
 */
class FPofHttpRouter
{
public:
	/** Parse JSON body from a POST request. Returns nullptr on failure. */
	static TSharedPtr<FJsonObject> ParseJsonBody(const TArray<uint8>& Body);

	/** Extract a query parameter by name from a URL query string. */
	static FString GetQueryParam(const FString& QueryString, const FString& ParamName);
};
