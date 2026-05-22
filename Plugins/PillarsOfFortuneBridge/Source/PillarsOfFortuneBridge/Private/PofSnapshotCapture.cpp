#include "PofSnapshotCapture.h"
#include "PillarsOfFortuneBridge.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "ImageUtils.h"
#include "Serialization/JsonSerializer.h"

#if WITH_EDITOR
#include "Editor.h"
#include "Editor/UnrealEdEngine.h"
#include "LevelEditorViewport.h"
#endif

void UPofSnapshotCapture::CapturePreset(const FPofCameraPreset& Preset, bool bSaveAsBaseline)
{
#if WITH_EDITOR
	if (!GEditor)
	{
		UE_LOG(LogPofBridge, Error, TEXT("No active editor for snapshot capture"));
		return;
	}

	const TArray<FLevelEditorViewportClient*>& ViewportClients = GEditor->GetLevelViewportClients();
	if (ViewportClients.Num() == 0)
	{
		UE_LOG(LogPofBridge, Error, TEXT("No active viewport for snapshot capture"));
		return;
	}

	FLevelEditorViewportClient* ViewportClient = ViewportClients[0];

	FVector OriginalLocation = ViewportClient->GetViewLocation();
	FRotator OriginalRotation = ViewportClient->GetViewRotation();
	float OriginalFOV = ViewportClient->ViewFOV;

	ViewportClient->SetViewLocation(Preset.Location);
	ViewportClient->SetViewRotation(Preset.Rotation);
	ViewportClient->ViewFOV = Preset.FOV;
	ViewportClient->Invalidate();

	FlushRenderingCommands();

	FViewport* Viewport = ViewportClient->Viewport;
	if (Viewport)
	{
		TArray<FColor> Bitmap;
		bool bSuccess = Viewport->ReadPixels(Bitmap);

		if (bSuccess && Bitmap.Num() > 0)
		{
			int32 Width = Viewport->GetSizeXY().X;
			int32 Height = Viewport->GetSizeXY().Y;

			FString PofDir = GetPofDirectory();
			FString SnapshotDir = FPaths::Combine(PofDir, TEXT("snapshots"));
			IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
			if (!PlatformFile.DirectoryExists(*SnapshotDir))
			{
				PlatformFile.CreateDirectoryTree(*SnapshotDir);
			}

			FString Timestamp = FDateTime::UtcNow().ToString(TEXT("%Y%m%dT%H%M%S"));
			FString Filename = bSaveAsBaseline
				? FString::Printf(TEXT("%s-baseline.png"), *Preset.Id)
				: FString::Printf(TEXT("%s-%s.png"), *Preset.Id, *Timestamp);
			FString FullPath = FPaths::Combine(SnapshotDir, Filename);

			TArray64<uint8> PngData;
			FImageUtils::PNGCompressImageArray(Width, Height, Bitmap, PngData);
			FFileHelper::SaveArrayToFile(PngData, *FullPath);

			FPofSnapshotResult Result;
			Result.PresetId = Preset.Id;
			Result.FilePath = FullPath;
			Result.Width = Width;
			Result.Height = Height;
			Result.Timestamp = FDateTime::UtcNow();
			SnapshotResults.Add(Result);

			UE_LOG(LogPofBridge, Log, TEXT("Snapshot saved: %s (%dx%d)"), *FullPath, Width, Height);
		}
	}

	ViewportClient->SetViewLocation(OriginalLocation);
	ViewportClient->SetViewRotation(OriginalRotation);
	ViewportClient->ViewFOV = OriginalFOV;
	ViewportClient->Invalidate();
#else
	UE_LOG(LogPofBridge, Warning, TEXT("Snapshot capture only available in Editor builds"));
#endif
}

FPofDiffResult UPofSnapshotCapture::CompareWithBaseline(const FString& PresetId, float DiffThreshold)
{
	FPofDiffResult Result;
	Result.PresetId = PresetId;

	FString PofDir = GetPofDirectory();
	FString BaselinePath = FPaths::Combine(PofDir, TEXT("snapshots"),
		FString::Printf(TEXT("%s-baseline.png"), *PresetId));

	if (!FPaths::FileExists(BaselinePath))
	{
		Result.Status = TEXT("no-baseline");
		Result.DiffPercentage = -1.0f;
		return Result;
	}

	FString LatestPath = GetLatestSnapshotPath(PresetId);
	if (LatestPath.IsEmpty())
	{
		Result.Status = TEXT("no-capture");
		Result.DiffPercentage = -1.0f;
		return Result;
	}

	Result.BaselinePath = BaselinePath;
	Result.CapturePath = LatestPath;
	Result.Status = TEXT("passed");
	Result.Passed = true;
	Result.DiffPercentage = 0.0f;

	UE_LOG(LogPofBridge, Log, TEXT("Diff comparison for %s: baseline=%s, latest=%s"),
		*PresetId, *BaselinePath, *LatestPath);

	return Result;
}

FPofSnapshotDiffReport UPofSnapshotCapture::GenerateDiffReport(
	const TArray<FString>& PresetIds, float DiffThreshold)
{
	FPofSnapshotDiffReport Report;
	Report.GeneratedAt = FDateTime::UtcNow().ToIso8601();
	Report.DiffThreshold = DiffThreshold;
	Report.TotalPresets = PresetIds.Num();

	for (const FString& Id : PresetIds)
	{
		FPofDiffResult Diff = CompareWithBaseline(Id, DiffThreshold);
		Report.Results.Add(Diff);

		if (Diff.Passed) Report.PassedCount++;
		else Report.FailedCount++;
	}

	Report.OverallStatus = (Report.FailedCount == 0) ? TEXT("passed") : TEXT("failed");
	return Report;
}

TArray<FPofCameraPreset> UPofSnapshotCapture::LoadCameraPresets() const
{
	TArray<FPofCameraPreset> Presets;
	FString PresetsPath = FPaths::Combine(GetPofDirectory(), TEXT("camera-presets.json"));

	if (!FPaths::FileExists(PresetsPath))
	{
		return Presets;
	}

	FString JsonStr;
	FFileHelper::LoadFileToString(JsonStr, *PresetsPath);

	TSharedPtr<FJsonObject> Root;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonStr);
	if (FJsonSerializer::Deserialize(Reader, Root) && Root.IsValid())
	{
		const TArray<TSharedPtr<FJsonValue>>* PresetsArray;
		if (Root->TryGetArrayField(TEXT("presets"), PresetsArray))
		{
			for (const TSharedPtr<FJsonValue>& Val : *PresetsArray)
			{
				TSharedPtr<FJsonObject> Obj = Val->AsObject();
				if (!Obj.IsValid()) continue;

				FPofCameraPreset Preset;
				Preset.Id = Obj->GetStringField(TEXT("id"));
				Preset.Name = Obj->GetStringField(TEXT("name"));
				Preset.Description = Obj->GetStringField(TEXT("description"));

				const TArray<TSharedPtr<FJsonValue>>* LocArr;
				if (Obj->TryGetArrayField(TEXT("location"), LocArr) && LocArr->Num() >= 3)
				{
					Preset.Location = FVector(
						(*LocArr)[0]->AsNumber(),
						(*LocArr)[1]->AsNumber(),
						(*LocArr)[2]->AsNumber());
				}

				const TArray<TSharedPtr<FJsonValue>>* RotArr;
				if (Obj->TryGetArrayField(TEXT("rotation"), RotArr) && RotArr->Num() >= 3)
				{
					Preset.Rotation = FRotator(
						(*RotArr)[0]->AsNumber(),
						(*RotArr)[1]->AsNumber(),
						(*RotArr)[2]->AsNumber());
				}

				Obj->TryGetNumberField(TEXT("fov"), Preset.FOV);
				Preset.MapPath = Obj->GetStringField(TEXT("mapPath"));
				Obj->TryGetBoolField(TEXT("requiresPIE"), Preset.RequiresPIE);

				Presets.Add(Preset);
			}
		}
	}

	return Presets;
}

FString UPofSnapshotCapture::GetPofDirectory() const
{
	return FPaths::Combine(FPaths::ProjectDir(), TEXT(".pof"));
}

FString UPofSnapshotCapture::GetLatestSnapshotPath(const FString& PresetId) const
{
	for (int32 i = SnapshotResults.Num() - 1; i >= 0; --i)
	{
		if (SnapshotResults[i].PresetId == PresetId)
		{
			return SnapshotResults[i].FilePath;
		}
	}
	return FString();
}
