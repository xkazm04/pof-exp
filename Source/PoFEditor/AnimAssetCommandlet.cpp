#include "AnimAssetCommandlet.h"
#include "Animation/AnimMontage.h"
#include "Animation/BlendSpace1D.h"
#include "Animation/BlendSpace.h"
#include "Engine/CurveTable.h"
#include "Curves/SimpleCurve.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"

UAnimAssetCommandlet::UAnimAssetCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UAnimAssetCommandlet::Main(const FString& Params)
{
	UE_LOG(LogTemp, Display, TEXT("=== AnimAsset Commandlet: Starting animation asset creation ==="));

	// --- Parse commandlet parameters ---
	float SpeedMax = 900.f; // Default matches AARPGCharacterBase::SprintSpeed
	ParseFloatParam(Params, TEXT("SpeedMax"), SpeedMax);

	const bool bUseSpeedRatio = Params.Contains(TEXT("-UseSpeedRatio"));

	UE_LOG(LogTemp, Display, TEXT("  SpeedMax=%.0f  UseSpeedRatio=%s"), SpeedMax,
		bUseSpeedRatio ? TEXT("true") : TEXT("false"));

	int32 SuccessCount = 0;
	int32 FailCount = 0;

	// --- 1) 1D Locomotion Blend Space ---
	UE_LOG(LogTemp, Display, TEXT("--- Creating BS1D_Locomotion ---"));
	if (CreateBlendSpace1D(TEXT("/Game/Characters/Player/Animations"), TEXT("BS1D_Locomotion"), SpeedMax, bUseSpeedRatio))
	{
		SuccessCount++;
		UE_LOG(LogTemp, Display, TEXT("[OK] BS1D_Locomotion created successfully."));
	}
	else
	{
		FailCount++;
		UE_LOG(LogTemp, Warning, TEXT("[FAIL] BS1D_Locomotion creation failed."));
	}

	// --- 2) 2D Locomotion Blend Space (Direction x Speed) for strafing ---
	UE_LOG(LogTemp, Display, TEXT("--- Creating BS_Locomotion (2D) ---"));
	if (CreateBlendSpace2D(TEXT("/Game/Characters/Player/Animations"), TEXT("BS_Locomotion"), SpeedMax))
	{
		SuccessCount++;
		UE_LOG(LogTemp, Display, TEXT("[OK] BS_Locomotion (2D) created successfully."));
	}
	else
	{
		FailCount++;
		UE_LOG(LogTemp, Warning, TEXT("[FAIL] BS_Locomotion (2D) creation failed."));
	}

	// --- 3) Melee combo montage (3 sections) ---
	UE_LOG(LogTemp, Display, TEXT("--- Creating AM_MeleeCombo ---"));
	{
		TArray<FName> Sections;
		Sections.Add(FName("Attack1"));
		Sections.Add(FName("Attack2"));
		Sections.Add(FName("Attack3"));

		if (CreateMontageShell(TEXT("/Game/Characters/Player/Animations/Montages"), TEXT("AM_MeleeCombo"), Sections))
		{
			SuccessCount++;
			UE_LOG(LogTemp, Display, TEXT("[OK] AM_MeleeCombo created successfully."));
		}
		else
		{
			FailCount++;
			UE_LOG(LogTemp, Warning, TEXT("[FAIL] AM_MeleeCombo creation failed."));
		}
	}

	// --- 4) Dodge montages ---
	const TArray<FString> DodgeDirs = { TEXT("Forward"), TEXT("Backward"), TEXT("Left"), TEXT("Right") };
	for (const FString& Dir : DodgeDirs)
	{
		FString Name = FString::Printf(TEXT("AM_Dodge_%s"), *Dir);
		UE_LOG(LogTemp, Display, TEXT("--- Creating %s ---"), *Name);

		TArray<FName> Sections;
		Sections.Add(FName("Dodge"));

		if (CreateMontageShell(TEXT("/Game/Characters/Player/Animations/Montages"), Name, Sections))
		{
			SuccessCount++;
			UE_LOG(LogTemp, Display, TEXT("[OK] %s created successfully."), *Name);
		}
		else
		{
			FailCount++;
			UE_LOG(LogTemp, Warning, TEXT("[FAIL] %s creation failed."), *Name);
		}
	}

	// --- 5) Heavy melee combo montage (2 sections — slower, heavier hits) ---
	UE_LOG(LogTemp, Display, TEXT("--- Creating AM_HeavyCombo ---"));
	{
		TArray<FName> HeavySections;
		HeavySections.Add(FName("Heavy1"));
		HeavySections.Add(FName("Heavy2"));

		if (CreateMontageShell(TEXT("/Game/Characters/Player/Animations/Montages"), TEXT("AM_HeavyCombo"), HeavySections))
		{
			SuccessCount++;
			UE_LOG(LogTemp, Display, TEXT("[OK] AM_HeavyCombo created successfully."));
		}
		else
		{
			FailCount++;
			UE_LOG(LogTemp, Warning, TEXT("[FAIL] AM_HeavyCombo creation failed."));
		}
	}

	// --- 6) Hit reaction and death montages ---
	{
		TArray<FName> HitSections;
		HitSections.Add(FName("HitReact"));
		if (CreateMontageShell(TEXT("/Game/Characters/Player/Animations/Montages"), TEXT("AM_HitReact"), HitSections))
		{
			SuccessCount++;
		}
		else
		{
			FailCount++;
		}

		// Directional hit reacts for more responsive combat feedback
		const TArray<FString> HitDirs = { TEXT("Front"), TEXT("Back"), TEXT("Left"), TEXT("Right") };
		for (const FString& Dir : HitDirs)
		{
			FString Name = FString::Printf(TEXT("AM_HitReact_%s"), *Dir);
			TArray<FName> Sections;
			Sections.Add(FName("HitReact"));
			if (CreateMontageShell(TEXT("/Game/Characters/Player/Animations/Montages"), Name, Sections))
			{
				SuccessCount++;
			}
			else
			{
				FailCount++;
			}
		}

		TArray<FName> DeathSections;
		DeathSections.Add(FName("Death"));
		if (CreateMontageShell(TEXT("/Game/Characters/Player/Animations/Montages"), TEXT("AM_Death"), DeathSections))
		{
			SuccessCount++;
		}
		else
		{
			FailCount++;
		}
	}

	// --- 7) XP Requirements Curve Table ---
	UE_LOG(LogTemp, Display, TEXT("--- Creating CT_XPRequirements ---"));
	if (CreateXPCurveTable(TEXT("/Game/Data/Progression"), TEXT("CT_XPRequirements")))
	{
		SuccessCount++;
		UE_LOG(LogTemp, Display, TEXT("[OK] CT_XPRequirements created successfully."));
	}
	else
	{
		FailCount++;
		UE_LOG(LogTemp, Warning, TEXT("[FAIL] CT_XPRequirements creation failed."));
	}

	UE_LOG(LogTemp, Display, TEXT("=== AnimAsset Commandlet: Done. Success=%d, Failed=%d ==="), SuccessCount, FailCount);

	return FailCount > 0 ? 1 : 0;
}

bool UAnimAssetCommandlet::CreateBlendSpace1D(const FString& PackagePath, const FString& AssetName,
	float SpeedMax, bool bUseSpeedRatio)
{
	const FString FullPath = PackagePath / AssetName;

	UPackage* Package = CreatePackage(*FullPath);
	if (!Package)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create package: %s"), *FullPath);
		return false;
	}

	Package->FullyLoad();

	UBlendSpace1D* BlendSpace = NewObject<UBlendSpace1D>(
		Package, UBlendSpace1D::StaticClass(),
		*AssetName, RF_Public | RF_Standalone);

	if (!BlendSpace)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create UBlendSpace1D object"));
		return false;
	}

	// Configure axis via reflection (BlendParameters is protected)
	FProperty* Prop = UBlendSpace::StaticClass()->FindPropertyByName(TEXT("BlendParameters"));
	if (Prop)
	{
		FBlendParameter* Params = Prop->ContainerPtrToValuePtr<FBlendParameter>(BlendSpace);

		if (bUseSpeedRatio)
		{
			// SpeedRatio mode: [0, 1] — maps to GetSpeedRatio() from AARPGCharacterBase
			// Grid: 0.0 = Idle, ~0.67 = Walk, 1.0 = Sprint
			Params[0].DisplayName = TEXT("SpeedRatio");
			Params[0].Min = 0.f;
			Params[0].Max = 1.f;
			Params[0].GridNum = 4; // 5 sample points: 0, 0.25, 0.5, 0.75, 1.0
			UE_LOG(LogTemp, Display, TEXT("  BS1D axis: SpeedRatio [0, 1] — 5 grid samples"));
			UE_LOG(LogTemp, Display, TEXT("  Sample layout: 0.0=Idle, 0.5=Walk, 0.7=Jog, 1.0=Sprint"));
		}
		else
		{
			// Absolute speed mode: [0, SpeedMax] — matches SprintSpeed (default 900)
			Params[0].DisplayName = TEXT("Speed");
			Params[0].Min = 0.f;
			Params[0].Max = SpeedMax;
			Params[0].GridNum = 3; // 4 sample points: 0, 300, 600, 900
			UE_LOG(LogTemp, Display, TEXT("  BS1D axis: Speed [0, %.0f] — 4 grid samples"), SpeedMax);
			UE_LOG(LogTemp, Display, TEXT("  Sample layout: 0=Idle, %.0f=Walk, %.0f=Jog, %.0f=Sprint"),
				SpeedMax * 0.33f, SpeedMax * 0.67f, SpeedMax);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("  Could not find BlendParameters property via reflection"));
	}

	UE_LOG(LogTemp, Display, TEXT("  Add sample animations in-editor: Idle at min, Walk/Jog in middle, Sprint at max"));

	return SaveAsset(BlendSpace, Package, FullPath);
}

bool UAnimAssetCommandlet::CreateBlendSpace2D(const FString& PackagePath, const FString& AssetName, float SpeedMax)
{
	const FString FullPath = PackagePath / AssetName;

	UPackage* Package = CreatePackage(*FullPath);
	if (!Package)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create package: %s"), *FullPath);
		return false;
	}

	Package->FullyLoad();

	// UBlendSpace is the 2D blend space class
	UBlendSpace* BlendSpace = NewObject<UBlendSpace>(
		Package, UBlendSpace::StaticClass(),
		*AssetName, RF_Public | RF_Standalone);

	if (!BlendSpace)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create UBlendSpace (2D) object"));
		return false;
	}

	// Configure both axes via reflection
	FProperty* Prop = UBlendSpace::StaticClass()->FindPropertyByName(TEXT("BlendParameters"));
	if (Prop)
	{
		FBlendParameter* Params = Prop->ContainerPtrToValuePtr<FBlendParameter>(BlendSpace);

		// Axis 0 (Horizontal): Direction in degrees [-180, 180]
		// Maps to AARPGCharacterBase::GetMovementDirection()
		Params[0].DisplayName = TEXT("Direction");
		Params[0].Min = -180.f;
		Params[0].Max = 180.f;
		Params[0].GridNum = 4; // 5 columns: -180, -90, 0, 90, 180

		// Axis 1 (Vertical): Speed [0, SprintSpeed]
		// Maps to velocity magnitude or GetSpeedRatio() * SprintSpeed
		Params[1].DisplayName = TEXT("Speed");
		Params[1].Min = 0.f;
		Params[1].Max = SpeedMax;
		Params[1].GridNum = 2; // 3 rows: 0 (idle), SpeedMax/2 (walk), SpeedMax (sprint)

		UE_LOG(LogTemp, Display, TEXT("  BS2D Axis 0 (Horizontal): Direction [-180, 180] — 5 columns"));
		UE_LOG(LogTemp, Display, TEXT("  BS2D Axis 1 (Vertical):   Speed [0, %.0f] — 3 rows"), SpeedMax);
		UE_LOG(LogTemp, Display, TEXT("  Sample grid (15 slots):"));
		UE_LOG(LogTemp, Display, TEXT("    Row 0 (Speed=0):    Idle (all directions share same idle anim)"));
		UE_LOG(LogTemp, Display, TEXT("    Row 1 (Speed=%.0f): Walk_BW, Walk_L, Walk_FW, Walk_R, Walk_BW"), SpeedMax * 0.5f);
		UE_LOG(LogTemp, Display, TEXT("    Row 2 (Speed=%.0f): Run_BW,  Run_L,  Run_FW,  Run_R,  Run_BW"), SpeedMax);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("  Could not find BlendParameters property via reflection"));
	}

	UE_LOG(LogTemp, Display, TEXT("  Use with GetMovementDirection() (X) and Velocity.Size2D() (Y) in AnimBP"));

	return SaveAsset(BlendSpace, Package, FullPath);
}

bool UAnimAssetCommandlet::CreateMontageShell(const FString& PackagePath, const FString& AssetName, const TArray<FName>& SectionNames)
{
	const FString FullPath = PackagePath / AssetName;

	UPackage* Package = CreatePackage(*FullPath);
	if (!Package)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create package: %s"), *FullPath);
		return false;
	}

	Package->FullyLoad();

	UAnimMontage* Montage = NewObject<UAnimMontage>(
		Package, UAnimMontage::StaticClass(),
		*AssetName, RF_Public | RF_Standalone);

	if (!Montage)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create UAnimMontage object"));
		return false;
	}

	// Set slot name
	if (Montage->SlotAnimTracks.Num() > 0)
	{
		Montage->SlotAnimTracks[0].SlotName = FName("DefaultSlot");
	}

	// Add composite sections (montage sections for combo chaining)
	for (int32 i = 0; i < SectionNames.Num(); ++i)
	{
		if (i == 0)
		{
			// First section already exists by default as "Default"
			// Rename it
			if (Montage->CompositeSections.Num() > 0)
			{
				Montage->CompositeSections[0].SectionName = SectionNames[i];
			}
			else
			{
				FCompositeSection NewSection;
				NewSection.SectionName = SectionNames[i];
				NewSection.SetTime(0.f);
				Montage->CompositeSections.Add(NewSection);
			}
		}
		else
		{
			FCompositeSection NewSection;
			NewSection.SectionName = SectionNames[i];
			// Space sections every 1 second as placeholders
			NewSection.SetTime(static_cast<float>(i));
			Montage->CompositeSections.Add(NewSection);
		}

		UE_LOG(LogTemp, Display, TEXT("  Added montage section: %s (at %.1fs)"),
			*SectionNames[i].ToString(), static_cast<float>(i));
	}

	// For combo montages, link sections sequentially
	if (SectionNames.Num() > 1)
	{
		for (int32 i = 0; i < SectionNames.Num() - 1; ++i)
		{
			Montage->CompositeSections[i].NextSectionName = SectionNames[i + 1];
		}
		// Last section loops or stops (no next section = stop)
		UE_LOG(LogTemp, Display, TEXT("  Sections linked sequentially for combo chaining"));
	}

	UE_LOG(LogTemp, Display, TEXT("  NOTE: Assign a Skeleton and animation sequences in-editor"));

	return SaveAsset(Montage, Package, FullPath);
}

bool UAnimAssetCommandlet::CreateXPCurveTable(const FString& PackagePath, const FString& AssetName)
{
	const FString FullPath = PackagePath / AssetName;

	UPackage* Package = CreatePackage(*FullPath);
	if (!Package)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create package: %s"), *FullPath);
		return false;
	}

	Package->FullyLoad();

	UCurveTable* CurveTable = NewObject<UCurveTable>(
		Package, UCurveTable::StaticClass(),
		*AssetName, RF_Public | RF_Standalone);

	if (!CurveTable)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create UCurveTable object"));
		return false;
	}

	// Create the XPToNextLevel curve: XP = 50*L^2 + 50*L
	// Level 1: 100, Level 2: 250, Level 3: 500, ..., Level 50: 127500
	FSimpleCurve& XPCurve = CurveTable->AddSimpleCurve(FName("XPToNextLevel"));

	constexpr int32 MaxLevel = 50;
	for (int32 Level = 1; Level <= MaxLevel; ++Level)
	{
		const float XP = 50.f * Level * Level + 50.f * Level;
		XPCurve.AddKey(static_cast<float>(Level), FMath::RoundToFloat(XP));
	}

	UE_LOG(LogTemp, Display, TEXT("  XP curve: 50*L^2 + 50*L (L1=100, L2=250, L3=500, L10=5500, L50=127500)"));

	return SaveAsset(CurveTable, Package, FullPath);
}

bool UAnimAssetCommandlet::SaveAsset(UObject* Asset, UPackage* Package, const FString& PackagePath)
{
	// Notify the asset registry
	FAssetRegistryModule::AssetCreated(Asset);
	Asset->MarkPackageDirty();

	// Build the file path for saving
	const FString FilePath = FPackageName::LongPackageNameToFilename(
		PackagePath, FPackageName::GetAssetPackageExtension());

	// Ensure the directory exists
	const FString Directory = FPaths::GetPath(FilePath);
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*Directory))
	{
		PlatformFile.CreateDirectoryTree(*Directory);
	}

	// Save
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	const bool bSaved = UPackage::SavePackage(Package, Asset, *FilePath, SaveArgs);

	if (bSaved)
	{
		UE_LOG(LogTemp, Display, TEXT("  Saved: %s"), *FilePath);
		return true;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("  Failed to save: %s"), *FilePath);
		return false;
	}
}

bool UAnimAssetCommandlet::ParseFloatParam(const FString& Params, const FString& ParamName, float& OutValue)
{
	FString SearchKey = FString::Printf(TEXT("-%s="), *ParamName);
	int32 KeyIdx = Params.Find(SearchKey, ESearchCase::IgnoreCase);
	if (KeyIdx == INDEX_NONE)
	{
		return false;
	}

	int32 ValueStart = KeyIdx + SearchKey.Len();
	if (ValueStart >= Params.Len())
	{
		return false;
	}

	// Read until space or end of string
	int32 ValueEnd = Params.Find(TEXT(" "), ESearchCase::IgnoreCase, ESearchDir::FromStart, ValueStart);
	if (ValueEnd == INDEX_NONE)
	{
		ValueEnd = Params.Len();
	}

	FString ValueStr = Params.Mid(ValueStart, ValueEnd - ValueStart);
	if (ValueStr.IsNumeric())
	{
		OutValue = FCString::Atof(*ValueStr);
		return true;
	}

	return false;
}
