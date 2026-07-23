#include "World/PoFDuelStaging.h"
#include "GameFramework/Pawn.h"
#include "Components/PrimitiveComponent.h"
#include "Components/LightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"
#include "Engine/PostProcessVolume.h"
#include "EngineUtils.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Engine/ExponentialHeightFog.h"

namespace PoFDuelStaging
{

TArray<FSaberPalette> GetSaberPalettes()
{
	// Mirrors the items SOR bladeColor values (item-saber-crimson / item-saber-azure).
	return {
		{ FName(TEXT("Crimson")), FLinearColor(1.f, 0.16f, 0.16f) },
		{ FName(TEXT("Azure")),   FLinearColor(0.23f, 0.55f, 1.f) },
	};
}

TArray<FQuestPalette> GetQuestPalettes()
{
	// Mirrors the quests SOR Triggers & World-State `environment` blocks exactly
	// (quest-lords-challenge / quest-echoes-order) — hex values from the artifacts.
	return {
		{ FName(TEXT("lords-challenge")), TEXT("The Lord's Challenge"),
		  FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("2a0c08"))),
		  FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("ff6a3a"))),
		  FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("ff3a1a"))) },
		{ FName(TEXT("echoes-order")), TEXT("Echoes of the Order"),
		  FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("081422"))),
		  FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("6ab8ff"))),
		  FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("2a6aff"))) },
	};
}

const FSaberPalette* FindSaber(FName Id)
{
	static TArray<FSaberPalette> Palettes = GetSaberPalettes();
	return Palettes.FindByPredicate([&](const FSaberPalette& P) { return P.Id == Id; });
}

const FQuestPalette* FindQuest(FName Id)
{
	static TArray<FQuestPalette> Palettes = GetQuestPalettes();
	return Palettes.FindByPredicate([&](const FQuestPalette& P) { return P.Id == Id; });
}

static bool NameLooksBladeish(const FString& Name)
{
	return Name.Contains(TEXT("saber"), ESearchCase::IgnoreCase)
		|| Name.Contains(TEXT("blade"), ESearchCase::IgnoreCase)
		|| Name.Contains(TEXT("beam"), ESearchCase::IgnoreCase)
		|| Name.Contains(TEXT("laser"), ESearchCase::IgnoreCase)
		|| Name.Contains(TEXT("weapon"), ESearchCase::IgnoreCase);
}

int32 ApplySaberChoice(APawn* Pawn, FName SaberId)
{
	const FSaberPalette* Palette = FindSaber(SaberId);
	if (!Pawn || !Palette)
	{
		return 0;
	}
	int32 Touched = 0;
	TArray<UActorComponent*> Components;
	Pawn->GetComponents(Components);
	for (UActorComponent* Comp : Components)
	{
		const FString CompName = Comp->GetName();
		if (!NameLooksBladeish(CompName))
		{
			continue;
		}
		if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Comp))
		{
			for (int32 i = 0; i < Prim->GetNumMaterials(); ++i)
			{
				if (UMaterialInstanceDynamic* MID = Prim->CreateAndSetMaterialInstanceDynamic(i))
				{
					// Discover the material contract: log every vector param + set them ALL
					// to the palette color (a blade material exposes its glow color somewhere).
					TArray<FMaterialParameterInfo> Infos;
					TArray<FGuid> Ids;
					MID->GetAllVectorParameterInfo(Infos, Ids);
					for (const FMaterialParameterInfo& Info : Infos)
					{
						UE_LOG(LogTemp, Log, TEXT("[DuelStaging]   slot %d vector param: %s"), i, *Info.Name.ToString());
						MID->SetVectorParameterValue(Info.Name, Palette->Color);
					}
					if (Infos.Num() == 0)
					{
						// Param-less material — swap in the engine basic material (exposes
						// 'Color') so the choice is VISIBLE; a flat blade beats a lying one.
						if (UMaterialInterface* Basic = LoadObject<UMaterialInterface>(
								nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
						{
							UMaterialInstanceDynamic* Swap = UMaterialInstanceDynamic::Create(Basic, Prim);
							Swap->SetVectorParameterValue(TEXT("Color"), Palette->Color * 4.f);
							Prim->SetMaterial(i, Swap);
							UE_LOG(LogTemp, Log, TEXT("[DuelStaging]   slot %d param-less - basic-material swap"), i);
						}
					}
					// Cover the common parameter spellings; unused ones no-op.
					MID->SetVectorParameterValue(TEXT("Color"), Palette->Color);
					MID->SetVectorParameterValue(TEXT("EmissiveColor"), Palette->Color * 8.f);
					MID->SetVectorParameterValue(TEXT("BaseColor"), Palette->Color);
					MID->SetVectorParameterValue(TEXT("Tint"), Palette->Color);
				}
			}
			++Touched;
			UE_LOG(LogTemp, Log, TEXT("[DuelStaging] saber mesh recolored: %s"), *CompName);
		}
		else if (ULightComponent* Light = Cast<ULightComponent>(Comp))
		{
			Light->SetLightColor(Palette->Color);
			++Touched;
			UE_LOG(LogTemp, Log, TEXT("[DuelStaging] saber light recolored: %s"), *CompName);
		}
	}
	UE_LOG(LogTemp, Log, TEXT("[DuelStaging] saber choice %s applied to %d component(s) on %s"),
		*SaberId.ToString(), Touched, *Pawn->GetName());
	// Inventory every renderable component once — the blade may hide under any name.
	for (UActorComponent* Comp : Components)
	{
		if (Cast<UPrimitiveComponent>(Comp) || Cast<ULightComponent>(Comp))
		{
			UE_LOG(LogTemp, Log, TEXT("[DuelStaging]   component: %s (%s)"),
				*Comp->GetName(), *Comp->GetClass()->GetName());
		}
	}
	if (Touched == 0)
	{
		// Honest signal: nothing blade-ish found — list candidates so the log teaches us.
		for (UActorComponent* Comp : Components)
		{
			if (Cast<UPrimitiveComponent>(Comp) || Cast<ULightComponent>(Comp))
			{
				UE_LOG(LogTemp, Log, TEXT("[DuelStaging]   candidate component: %s"), *Comp->GetName());
			}
		}
	}
	return Touched;
}

bool ApplyQuestStaging(UWorld* World, FName QuestId)
{
	const FQuestPalette* Palette = FindQuest(QuestId);
	if (!World || !Palette)
	{
		return false;
	}

	// Unbound post-process volume: scene color tint toward the quest light tint.
	APostProcessVolume* PPV = World->SpawnActor<APostProcessVolume>();
	if (PPV)
	{
		PPV->bUnbound = true;
		PPV->BlendWeight = 1.f;
		PPV->Settings.bOverride_SceneColorTint = true;
		// Keep luminance: lerp white toward the tint so the scene grades, not crushes.
		PPV->Settings.SceneColorTint = FMath::Lerp(FLinearColor::White, Palette->LightTint, 0.45f);
		UE_LOG(LogTemp, Log, TEXT("[DuelStaging] quest '%s' post-process tint applied"), *Palette->DisplayName);
	}

	// Height fog color (spawn one if the map has none — FeatureLab's template may not).
	AExponentialHeightFog* Fog = nullptr;
	for (TActorIterator<AExponentialHeightFog> It(World); It; ++It) { Fog = *It; break; }
	if (!Fog)
	{
		Fog = World->SpawnActor<AExponentialHeightFog>();
		if (Fog && Fog->GetComponent()) Fog->GetComponent()->SetFogDensity(0.05f);
	}
	if (Fog && Fog->GetComponent())
	{
		Fog->GetComponent()->SetFogInscatteringColor(Palette->FogColor);
		UE_LOG(LogTemp, Log, TEXT("[DuelStaging] quest fog color set (%s)"), *Palette->FogColor.ToString());
	}

	return PPV != nullptr;
}

} // namespace PoFDuelStaging
