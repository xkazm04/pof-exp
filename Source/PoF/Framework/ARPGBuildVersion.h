#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ARPGBuildVersion.generated.h"

/**
 * Tracks build version info: major.minor.patch + build number.
 * Reads from DefaultGame.ini [/Script/PoF.ARPGBuildVersionSubsystem] on init.
 * Build number auto-increments each packaged build via the build script.
 */
USTRUCT(BlueprintType)
struct FARPGVersionInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Version")
	int32 Major = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Version")
	int32 Minor = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Version")
	int32 Patch = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Version")
	int32 BuildNumber = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Version")
	FString Branch;

	UPROPERTY(BlueprintReadOnly, Category = "Version")
	FString BuildDate;

	FString ToShortString() const
	{
		return FString::Printf(TEXT("v%d.%d.%d"), Major, Minor, Patch);
	}

	FString ToFullString() const
	{
		return FString::Printf(TEXT("v%d.%d.%d (Build %d)"), Major, Minor, Patch, BuildNumber);
	}
};

UCLASS()
class POF_API UARPGBuildVersionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintPure, Category = "Version")
	const FARPGVersionInfo& GetVersionInfo() const { return VersionInfo; }

	UFUNCTION(BlueprintPure, Category = "Version")
	FString GetVersionString() const { return VersionInfo.ToShortString(); }

	UFUNCTION(BlueprintPure, Category = "Version")
	FString GetFullVersionString() const { return VersionInfo.ToFullString(); }

	/** Returns a display string like "v0.1.0 - Early Development" for UI. */
	UFUNCTION(BlueprintPure, Category = "Version")
	FString GetDisplayVersionString() const;

private:
	FARPGVersionInfo VersionInfo;

	void LoadVersionFromConfig();
};
