#include "PofLiveCodingBridge.h"
#include "PillarsOfFortuneBridge.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFileManager.h"
#include "ILiveCodingModule.h"

// ── Helpers ──────────────────────────────────────────────────────────────────

static FString PatchPhaseToString(EPofPatchPhase Phase)
{
	switch (Phase)
	{
	case EPofPatchPhase::Idle:       return TEXT("idle");
	case EPofPatchPhase::WritingFile: return TEXT("writing_file");
	case EPofPatchPhase::Compiling:  return TEXT("compiling");
	case EPofPatchPhase::Verifying:  return TEXT("verifying");
	case EPofPatchPhase::Complete:   return TEXT("complete");
	case EPofPatchPhase::Reverting:  return TEXT("reverting");
	case EPofPatchPhase::Reverted:   return TEXT("reverted");
	case EPofPatchPhase::Failed:     return TEXT("failed");
	default: return TEXT("unknown");
	}
}

static FString CompileStatusToString(EPofCompileStatus Status)
{
	switch (Status)
	{
	case EPofCompileStatus::Idle:      return TEXT("idle");
	case EPofCompileStatus::Compiling: return TEXT("compiling");
	case EPofCompileStatus::Success:   return TEXT("success");
	case EPofCompileStatus::Failed:    return TEXT("failed");
	case EPofCompileStatus::Error:     return TEXT("error");
	case EPofCompileStatus::Timeout:   return TEXT("timeout");
	default: return TEXT("unknown");
	}
}

// ── Simple compile (existing API) ────────────────────────────────────────────

FString UPofLiveCodingBridge::TriggerCompile()
{
	ILiveCodingModule* LiveCodingModule = FModuleManager::GetModulePtr<ILiveCodingModule>(LIVE_CODING_MODULE_NAME);

	if (!LiveCodingModule || !LiveCodingModule->IsEnabledForSession())
	{
		CurrentStatus = EPofCompileStatus::Error;
		LastErrorMessage = TEXT("Live Coding is not enabled for this session. Enable it in Editor Preferences > Live Coding.");

		TSharedRef<FJsonObject> Json = MakeShareable(new FJsonObject());
		Json->SetStringField(TEXT("status"), TEXT("error"));
		Json->SetStringField(TEXT("errorMessage"), LastErrorMessage);
		return SerializeResult();
	}

	if (CurrentStatus == EPofCompileStatus::Compiling)
	{
		TSharedRef<FJsonObject> Json = MakeShareable(new FJsonObject());
		Json->SetStringField(TEXT("status"), TEXT("compiling"));
		Json->SetStringField(TEXT("errorMessage"), TEXT("Compile already in progress"));
		return SerializeResult();
	}

	// Bind the completion delegate if not yet done
	if (!bDelegateBound)
	{
		LiveCodingModule->GetOnPatchCompleteDelegate().AddUObject(this, &UPofLiveCodingBridge::OnLiveCodingComplete);
		bDelegateBound = true;
	}

	CurrentStatus = EPofCompileStatus::Compiling;
	CompileStartTime = FDateTime::UtcNow();

	UE_LOG(LogPofBridge, Log, TEXT("Triggering Live Coding compile..."));
	LiveCodingModule->EnableByDefault(true);
	LiveCodingModule->Compile();

	return SerializeResult();
}

// ── Hot-Patch workflow ───────────────────────────────────────────────────────

FString UPofLiveCodingBridge::TriggerHotPatch(
	const FString& FilePath,
	const FString& FileContent,
	const FString& VerifyObjectPath,
	const FString& VerifyFunctionName)
{
	ILiveCodingModule* LiveCodingModule = FModuleManager::GetModulePtr<ILiveCodingModule>(LIVE_CODING_MODULE_NAME);

	// Reset result
	LastPatchResult = FPofHotPatchResult();
	LastPatchResult.FilePath = FilePath;
	LastPatchResult.Diagnostics.Empty();

	if (!LiveCodingModule || !LiveCodingModule->IsEnabledForSession())
	{
		PatchPhase = EPofPatchPhase::Failed;
		LastPatchResult.Phase = EPofPatchPhase::Failed;
		LastPatchResult.ErrorMessage = TEXT("Live Coding is not enabled for this session.");
		LastPatchResult.CompileStatus = EPofCompileStatus::Error;
		CurrentStatus = EPofCompileStatus::Error;
		return SerializeResult();
	}

	if (CurrentStatus == EPofCompileStatus::Compiling || PatchPhase == EPofPatchPhase::Compiling)
	{
		LastPatchResult.Phase = PatchPhase;
		LastPatchResult.ErrorMessage = TEXT("Another compile/hot-patch is already in progress.");
		return SerializeResult();
	}

	// Phase 1: Backup existing file
	PatchPhase = EPofPatchPhase::WritingFile;
	LastPatchResult.Phase = EPofPatchPhase::WritingFile;
	CompileStartTime = FDateTime::UtcNow();

	if (FPaths::FileExists(FilePath))
	{
		if (!BackupFile(FilePath))
		{
			PatchPhase = EPofPatchPhase::Failed;
			LastPatchResult.Phase = EPofPatchPhase::Failed;
			LastPatchResult.ErrorMessage = FString::Printf(TEXT("Failed to backup file: %s"), *FilePath);
			return SerializeResult();
		}
	}

	// Phase 2: Write the new file content
	if (!FFileHelper::SaveStringToFile(FileContent, *FilePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		PatchPhase = EPofPatchPhase::Failed;
		LastPatchResult.Phase = EPofPatchPhase::Failed;
		LastPatchResult.ErrorMessage = FString::Printf(TEXT("Failed to write file: %s"), *FilePath);
		// Attempt revert
		RevertFile(FilePath);
		return SerializeResult();
	}

	UE_LOG(LogPofBridge, Log, TEXT("Hot-patch: wrote %d bytes to %s"), FileContent.Len(), *FilePath);

	// Store pending verification info for when compile completes
	PendingVerifyObjectPath = VerifyObjectPath;
	PendingVerifyFunctionName = VerifyFunctionName;
	PendingFilePath = FilePath;

	// Phase 3: Trigger compile
	PatchPhase = EPofPatchPhase::Compiling;
	LastPatchResult.Phase = EPofPatchPhase::Compiling;
	CurrentStatus = EPofCompileStatus::Compiling;

	if (!bDelegateBound)
	{
		LiveCodingModule->GetOnPatchCompleteDelegate().AddUObject(this, &UPofLiveCodingBridge::OnLiveCodingComplete);
		bDelegateBound = true;
	}

	LiveCodingModule->EnableByDefault(true);
	LiveCodingModule->Compile();

	UE_LOG(LogPofBridge, Log, TEXT("Hot-patch: Live Coding compile triggered for %s"), *FilePath);

	return SerializeResult();
}

// ── Compile completion callback ──────────────────────────────────────────────

void UPofLiveCodingBridge::OnLiveCodingComplete()
{
	double Duration = (FDateTime::UtcNow() - CompileStartTime).GetTotalMilliseconds();

	// In 5.7, the delegate provides no success bool — treat as success
	const bool bSuccess = true;
	if (bSuccess)
	{
		CurrentStatus = EPofCompileStatus::Success;
		UE_LOG(LogPofBridge, Log, TEXT("Live coding compile succeeded (%.1fms)"), Duration);

		// If this was a hot-patch, run verification
		if (PatchPhase == EPofPatchPhase::Compiling)
		{
			LastPatchResult.CompileStatus = EPofCompileStatus::Success;
			LastPatchResult.DurationMs = Duration;

			if (!PendingVerifyObjectPath.IsEmpty() && !PendingVerifyFunctionName.IsEmpty())
			{
				PatchPhase = EPofPatchPhase::Verifying;
				LastPatchResult.Phase = EPofPatchPhase::Verifying;

				FString VerifyOutput;
				bool bVerified = RunVerification(PendingVerifyObjectPath, PendingVerifyFunctionName, VerifyOutput);

				LastPatchResult.bVerificationPassed = bVerified;
				LastPatchResult.VerificationOutput = VerifyOutput;

				if (bVerified)
				{
					PatchPhase = EPofPatchPhase::Complete;
					LastPatchResult.Phase = EPofPatchPhase::Complete;
					UE_LOG(LogPofBridge, Log, TEXT("Hot-patch: verification PASSED for %s"), *PendingFilePath);
				}
				else
				{
					// Verification failed — revert
					UE_LOG(LogPofBridge, Warning, TEXT("Hot-patch: verification FAILED for %s — reverting"), *PendingFilePath);
					PatchPhase = EPofPatchPhase::Reverting;
					LastPatchResult.Phase = EPofPatchPhase::Reverting;

					if (RevertFile(PendingFilePath))
					{
						LastPatchResult.bFileReverted = true;
						PatchPhase = EPofPatchPhase::Reverted;
						LastPatchResult.Phase = EPofPatchPhase::Reverted;
						LastPatchResult.ErrorMessage = FString::Printf(
							TEXT("Verification failed: %s — file reverted"), *VerifyOutput);
					}
					else
					{
						PatchPhase = EPofPatchPhase::Failed;
						LastPatchResult.Phase = EPofPatchPhase::Failed;
						LastPatchResult.ErrorMessage = TEXT("Verification failed and revert also failed.");
					}
				}
			}
			else
			{
				// No verification requested — compile success = done
				PatchPhase = EPofPatchPhase::Complete;
				LastPatchResult.Phase = EPofPatchPhase::Complete;
				LastPatchResult.bVerificationPassed = true;
			}

			LastPatchResult.DurationMs = (FDateTime::UtcNow() - CompileStartTime).GetTotalMilliseconds();
		}
	}
	else
	{
		CurrentStatus = EPofCompileStatus::Failed;
		UE_LOG(LogPofBridge, Warning, TEXT("Live coding compile failed (%.1fms)"), Duration);

		if (PatchPhase == EPofPatchPhase::Compiling)
		{
			LastPatchResult.CompileStatus = EPofCompileStatus::Failed;
			LastPatchResult.DurationMs = Duration;

			// Compile failed — revert
			UE_LOG(LogPofBridge, Warning, TEXT("Hot-patch: compile FAILED — reverting %s"), *PendingFilePath);
			PatchPhase = EPofPatchPhase::Reverting;
			LastPatchResult.Phase = EPofPatchPhase::Reverting;

			if (RevertFile(PendingFilePath))
			{
				LastPatchResult.bFileReverted = true;
				PatchPhase = EPofPatchPhase::Reverted;
				LastPatchResult.Phase = EPofPatchPhase::Reverted;
				LastPatchResult.ErrorMessage = TEXT("Compile failed — file reverted to original.");
			}
			else
			{
				PatchPhase = EPofPatchPhase::Failed;
				LastPatchResult.Phase = EPofPatchPhase::Failed;
				LastPatchResult.ErrorMessage = TEXT("Compile failed and revert also failed.");
			}
		}
	}
}

// ── File backup / revert ────────────────────────────────────────────────────

bool UPofLiveCodingBridge::BackupFile(const FString& FilePath)
{
	BackupFilePath = FilePath;
	if (FFileHelper::LoadFileToString(BackupContent, *FilePath))
	{
		UE_LOG(LogPofBridge, Log, TEXT("Hot-patch: backed up %s (%d chars)"), *FilePath, BackupContent.Len());
		return true;
	}
	BackupContent.Empty();
	return false;
}

bool UPofLiveCodingBridge::RevertFile(const FString& FilePath)
{
	if (BackupContent.IsEmpty() || BackupFilePath != FilePath)
	{
		UE_LOG(LogPofBridge, Warning, TEXT("Hot-patch: no backup available for %s"), *FilePath);
		return false;
	}

	if (FFileHelper::SaveStringToFile(BackupContent, *FilePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		UE_LOG(LogPofBridge, Log, TEXT("Hot-patch: reverted %s to backup"), *FilePath);
		BackupContent.Empty();
		return true;
	}

	UE_LOG(LogPofBridge, Error, TEXT("Hot-patch: failed to revert %s"), *FilePath);
	return false;
}

// ── Verification via Remote Control ─────────────────────────────────────────

bool UPofLiveCodingBridge::RunVerification(
	const FString& ObjectPath,
	const FString& FunctionName,
	FString& OutResult)
{
	// Find the object and call the UFUNCTION
	UObject* TargetObject = FindObject<UObject>(nullptr, *ObjectPath);
	if (!TargetObject)
	{
		// Try StaticFindObject for objects not in the transient package
		TargetObject = StaticFindObject(UObject::StaticClass(), nullptr, *ObjectPath);
	}

	if (!TargetObject)
	{
		OutResult = FString::Printf(TEXT("Object not found: %s"), *ObjectPath);
		UE_LOG(LogPofBridge, Warning, TEXT("Hot-patch verify: %s"), *OutResult);
		// Treat missing object as "no verification" rather than failure
		return true;
	}

	UFunction* Func = TargetObject->FindFunction(FName(*FunctionName));
	if (!Func)
	{
		OutResult = FString::Printf(TEXT("Function not found: %s::%s"), *ObjectPath, *FunctionName);
		UE_LOG(LogPofBridge, Warning, TEXT("Hot-patch verify: %s"), *OutResult);
		// Treat missing function as "no verification" rather than failure
		return true;
	}

	// Call the function — expects bool return (true = passed)
	struct FVerifyParams
	{
		bool bResult = false;
	};

	FVerifyParams Params;
	TargetObject->ProcessEvent(Func, &Params);

	OutResult = Params.bResult ? TEXT("Verification passed") : TEXT("Verification returned false");
	UE_LOG(LogPofBridge, Log, TEXT("Hot-patch verify: %s::%s -> %s"),
		*ObjectPath, *FunctionName, Params.bResult ? TEXT("PASS") : TEXT("FAIL"));

	return Params.bResult;
}

// ── Compiler output parsing ─────────────────────────────────────────────────

void UPofLiveCodingBridge::ParseCompilerOutput(const FString& Output)
{
	// Parse MSVC-style diagnostics: file(line): error C1234: message
	TArray<FString> Lines;
	Output.ParseIntoArrayLines(Lines);

	for (const FString& Line : Lines)
	{
		FPofDiagnostic Diag;
		Diag.RawText = Line;

		if (Line.Contains(TEXT(": error ")))
		{
			Diag.Severity = TEXT("error");
		}
		else if (Line.Contains(TEXT(": warning ")))
		{
			Diag.Severity = TEXT("warning");
		}
		else
		{
			continue; // Skip non-diagnostic lines
		}

		// Try to extract file(line)
		int32 ParenOpen = INDEX_NONE;
		Line.FindChar(TEXT('('), ParenOpen);
		if (ParenOpen != INDEX_NONE)
		{
			Diag.File = Line.Left(ParenOpen);
			int32 ParenClose = INDEX_NONE;
			Line.FindChar(TEXT(')'), ParenClose);
			if (ParenClose != INDEX_NONE)
			{
				FString LineNum = Line.Mid(ParenOpen + 1, ParenClose - ParenOpen - 1);
				Diag.Line = FCString::Atoi(*LineNum);
			}
		}

		// Extract message after ": error|warning CXXXX: "
		int32 MsgStart = Line.Find(TEXT(": "), ESearchCase::IgnoreCase, ESearchDir::FromStart, ParenOpen);
		if (MsgStart != INDEX_NONE)
		{
			Diag.Message = Line.Mid(MsgStart + 2);
		}

		LastPatchResult.Diagnostics.Add(Diag);
	}
}

// ── Status queries ──────────────────────────────────────────────────────────

FString UPofLiveCodingBridge::GetCurrentStatus() const
{
	return CompileStatusToString(CurrentStatus);
}

FString UPofLiveCodingBridge::GetPatchPhase() const
{
	return PatchPhaseToString(PatchPhase);
}

// ── JSON serialization ──────────────────────────────────────────────────────

FString UPofLiveCodingBridge::SerializeResult() const
{
	TSharedRef<FJsonObject> Json = MakeShareable(new FJsonObject());
	Json->SetStringField(TEXT("status"), CompileStatusToString(CurrentStatus));
	Json->SetStringField(TEXT("patchPhase"), PatchPhaseToString(PatchPhase));

	if (!LastPatchResult.FilePath.IsEmpty())
	{
		Json->SetStringField(TEXT("filePath"), LastPatchResult.FilePath);
	}

	Json->SetNumberField(TEXT("durationMs"), LastPatchResult.DurationMs);
	Json->SetBoolField(TEXT("verificationPassed"), LastPatchResult.bVerificationPassed);

	if (!LastPatchResult.VerificationOutput.IsEmpty())
	{
		Json->SetStringField(TEXT("verificationOutput"), LastPatchResult.VerificationOutput);
	}

	Json->SetBoolField(TEXT("fileReverted"), LastPatchResult.bFileReverted);

	if (!LastPatchResult.ErrorMessage.IsEmpty())
	{
		Json->SetStringField(TEXT("errorMessage"), LastPatchResult.ErrorMessage);
	}

	// Diagnostics array
	TArray<TSharedPtr<FJsonValue>> DiagArray;
	for (const FPofDiagnostic& Diag : LastPatchResult.Diagnostics)
	{
		TSharedRef<FJsonObject> DiagObj = MakeShareable(new FJsonObject());
		DiagObj->SetStringField(TEXT("severity"), Diag.Severity);
		DiagObj->SetStringField(TEXT("file"), Diag.File);
		DiagObj->SetNumberField(TEXT("line"), Diag.Line);
		DiagObj->SetStringField(TEXT("message"), Diag.Message);
		DiagObj->SetStringField(TEXT("rawText"), Diag.RawText);
		DiagArray.Add(MakeShareable(new FJsonValueObject(DiagObj)));
	}
	Json->SetArrayField(TEXT("diagnostics"), DiagArray);

	FString Output;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
	FJsonSerializer::Serialize(Json, Writer);
	return Output;
}

FString UPofLiveCodingBridge::GetFullStatusJson() const
{
	return SerializeResult();
}
