#include "Framework/ARPGBuildVersion.h"
#include "Misc/ConfigCacheIni.h"

void UARPGBuildVersionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadVersionFromConfig();

	UE_LOG(LogTemp, Log, TEXT("[BuildVersion] %s"), *VersionInfo.ToFullString());
}

void UARPGBuildVersionSubsystem::LoadVersionFromConfig()
{
	const FString Section = TEXT("/Script/PoF.ARPGBuildVersionSubsystem");

	int32 Val;
	if (GConfig->GetInt(*Section, TEXT("Major"), Val, GGameIni))
	{
		VersionInfo.Major = Val;
	}
	if (GConfig->GetInt(*Section, TEXT("Minor"), Val, GGameIni))
	{
		VersionInfo.Minor = Val;
	}
	if (GConfig->GetInt(*Section, TEXT("Patch"), Val, GGameIni))
	{
		VersionInfo.Patch = Val;
	}
	if (GConfig->GetInt(*Section, TEXT("BuildNumber"), Val, GGameIni))
	{
		VersionInfo.BuildNumber = Val;
	}

	GConfig->GetString(*Section, TEXT("Branch"), VersionInfo.Branch, GGameIni);
	GConfig->GetString(*Section, TEXT("BuildDate"), VersionInfo.BuildDate, GGameIni);

	// Fallback build date to compile time if not set by build script
	if (VersionInfo.BuildDate.IsEmpty())
	{
		VersionInfo.BuildDate = FString(ANSI_TO_TCHAR(__DATE__));
	}
}

FString UARPGBuildVersionSubsystem::GetDisplayVersionString() const
{
	FString Phase;
	if (VersionInfo.Major == 0)
	{
		Phase = TEXT("Early Development");
	}
	else if (VersionInfo.Major == 1 && VersionInfo.Minor == 0 && VersionInfo.Patch == 0)
	{
		Phase = TEXT("Release");
	}
	else
	{
		Phase = FString::Printf(TEXT("Build %d"), VersionInfo.BuildNumber);
	}

	return FString::Printf(TEXT("%s - %s"), *VersionInfo.ToShortString(), *Phase);
}
