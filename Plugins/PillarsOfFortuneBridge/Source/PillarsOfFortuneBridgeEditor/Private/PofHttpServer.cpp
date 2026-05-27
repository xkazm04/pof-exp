#include "PofHttpServer.h"
#include "PillarsOfFortuneBridge.h"
#include "PillarsOfFortuneBridgeEditor.h"
#include "PofBridgeSettings.h"
#include "PofAssetManifest.h"
#include "PofTestRunner.h"
#include "PofSnapshotCapture.h"
#include "PofBlueprintIntrospector.h"
#include "PofLiveCodingBridge.h"
#include "PofPythonRunner.h"
#include "PofHttpRouter.h"
#include "Misc/EngineVersion.h"
#include "Misc/App.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "HttpServerModule.h"
#include "IHttpRouter.h"
#include "HttpPath.h"
#include "HttpServerRequest.h"
#include "HttpServerResponse.h"
#include "HttpRequestHandler.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

// ─── Helpers ───────────────────────────────────────────────────────────────────

static FString SerializeJsonObject(TSharedRef<FJsonObject> JsonObject)
{
	FString Output;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
	FJsonSerializer::Serialize(JsonObject, Writer);
	return Output;
}

static TUniquePtr<FHttpServerResponse> MakeJsonResponse(const FString& Json)
{
	return FHttpServerResponse::Create(Json, TEXT("application/json"));
}

static TUniquePtr<FHttpServerResponse> MakeErrorResponse(const FString& Message, int32 StatusCode)
{
	TSharedRef<FJsonObject> Json = MakeShareable(new FJsonObject());
	Json->SetStringField(TEXT("error"), Message);
	Json->SetNumberField(TEXT("statusCode"), StatusCode);
	return MakeJsonResponse(SerializeJsonObject(Json));
}

static void AddCorsHeaders(FHttpServerResponse& Response, const FString& AllowedOrigins)
{
	FString Origins = AllowedOrigins.IsEmpty() ? TEXT("http://localhost:3000") : AllowedOrigins;
	Response.Headers.Add(TEXT("Access-Control-Allow-Origin"), { Origins });
	Response.Headers.Add(TEXT("Access-Control-Allow-Methods"), { TEXT("GET, POST, OPTIONS") });
	Response.Headers.Add(TEXT("Access-Control-Allow-Headers"), { TEXT("Content-Type, X-Pof-Auth-Token") });
}

static bool ValidateAuth(const FHttpServerRequest& Request, const FString& AuthToken)
{
	if (AuthToken.IsEmpty())
	{
		return true;
	}

	const TArray<FString>* TokenHeader = Request.Headers.Find(TEXT("x-pof-auth-token"));
	if (!TokenHeader || TokenHeader->Num() == 0)
	{
		return false;
	}

	return (*TokenHeader)[0] == AuthToken;
}

// ─── Server Implementation ─────────────────────────────────────────────────────

void FPofHttpServer::Start(uint32 Port, const FString& InAuthToken, const FString& InAllowedOrigins)
{
	AuthToken = InAuthToken;
	AllowedOrigins = InAllowedOrigins;

	FHttpServerModule& HttpServerModule = FModuleManager::LoadModuleChecked<FHttpServerModule>("HTTPServer");
	HttpRouter = HttpServerModule.GetHttpRouter(Port);

	if (!HttpRouter.IsValid())
	{
		UE_LOG(LogPofBridge, Error, TEXT("Failed to create HTTP router on port %d"), Port);
		return;
	}

	// Capture copies for lambdas
	FString CapturedAuthToken = AuthToken;
	FString CapturedOrigins = AllowedOrigins;

	// GET /pof/status
	HttpRouter->BindRoute(FHttpPath(TEXT("/pof/status")), EHttpServerRequestVerbs::VERB_GET,
		FHttpRequestHandler::CreateLambda([CapturedAuthToken, CapturedOrigins](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
		{
			if (!ValidateAuth(Request, CapturedAuthToken))
			{
				OnComplete(MakeErrorResponse(TEXT("Unauthorized"), 401));
				return true;
			}

			UPofAssetManifest* Manifest = FPillarsOfFortuneBridgeEditorModule::Get().GetManifest();

			TSharedRef<FJsonObject> Json = MakeShareable(new FJsonObject());
			Json->SetStringField(TEXT("pluginVersion"), TEXT("0.1.0"));
			Json->SetStringField(TEXT("engineVersion"), FEngineVersion::Current().ToString());
			Json->SetStringField(TEXT("projectName"), FApp::GetProjectName());

#if WITH_EDITOR
			Json->SetStringField(TEXT("editorState"), GEditor && GEditor->IsPlaySessionInProgress() ? TEXT("PIE") : TEXT("Editing"));
#else
			Json->SetStringField(TEXT("editorState"), TEXT("Unknown"));
#endif

			Json->SetNumberField(TEXT("manifestAssetCount"), Manifest && Manifest->IsReady() ? Manifest->GetManifest().AssetCount : 0);
			Json->SetBoolField(TEXT("manifestReady"), Manifest ? Manifest->IsReady() : false);

			auto Response = MakeJsonResponse(SerializeJsonObject(Json));
			AddCorsHeaders(*Response, CapturedOrigins);
			OnComplete(MoveTemp(Response));
			return true;
		}));

	// GET /pof/manifest
	HttpRouter->BindRoute(FHttpPath(TEXT("/pof/manifest")), EHttpServerRequestVerbs::VERB_GET,
		FHttpRequestHandler::CreateLambda([CapturedAuthToken, CapturedOrigins](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
		{
			if (!ValidateAuth(Request, CapturedAuthToken))
			{
				OnComplete(MakeErrorResponse(TEXT("Unauthorized"), 401));
				return true;
			}

			UPofAssetManifest* Manifest = FPillarsOfFortuneBridgeEditorModule::Get().GetManifest();
			if (!Manifest || !Manifest->IsReady())
			{
				OnComplete(MakeErrorResponse(TEXT("Manifest not ready"), 503));
				return true;
			}

			const FString* ChecksumParam = Request.QueryParams.Find(TEXT("checksum-only"));
			bool bChecksumOnly = (ChecksumParam != nullptr);
			FString Json = Manifest->GetManifestJson(bChecksumOnly);
			auto Response = MakeJsonResponse(Json);
			AddCorsHeaders(*Response, CapturedOrigins);
			OnComplete(MoveTemp(Response));
			return true;
		}));

	// GET /pof/manifest/blueprint
	HttpRouter->BindRoute(FHttpPath(TEXT("/pof/manifest/blueprint")), EHttpServerRequestVerbs::VERB_GET,
		FHttpRequestHandler::CreateLambda([CapturedAuthToken, CapturedOrigins](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
		{
			if (!ValidateAuth(Request, CapturedAuthToken))
			{
				OnComplete(MakeErrorResponse(TEXT("Unauthorized"), 401));
				return true;
			}

			UPofBlueprintIntrospector* Introspector = FPillarsOfFortuneBridgeEditorModule::Get().GetIntrospector();
			if (!Introspector)
			{
				OnComplete(MakeErrorResponse(TEXT("Introspector not available"), 503));
				return true;
			}

			const FString* PathParam = Request.QueryParams.Find(TEXT("path"));
			FString Path = PathParam ? *PathParam : FString();

			if (Path.IsEmpty())
			{
				OnComplete(MakeErrorResponse(TEXT("Missing 'path' query parameter"), 400));
				return true;
			}

			FString Json = Introspector->IntrospectBlueprintByPath(Path);
			auto Response = MakeJsonResponse(Json);
			AddCorsHeaders(*Response, CapturedOrigins);
			OnComplete(MoveTemp(Response));
			return true;
		}));

	// POST /pof/test/run
	HttpRouter->BindRoute(FHttpPath(TEXT("/pof/test/run")), EHttpServerRequestVerbs::VERB_POST,
		FHttpRequestHandler::CreateLambda([CapturedAuthToken, CapturedOrigins](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
		{
			if (!ValidateAuth(Request, CapturedAuthToken))
			{
				OnComplete(MakeErrorResponse(TEXT("Unauthorized"), 401));
				return true;
			}

			UPofTestRunner* TestRunner = FPillarsOfFortuneBridgeEditorModule::Get().GetTestRunner();
			if (!TestRunner)
			{
				OnComplete(MakeErrorResponse(TEXT("Test runner not available"), 503));
				return true;
			}

			if (TestRunner->IsRunning())
			{
				OnComplete(MakeErrorResponse(TEXT("Test already in progress"), 409));
				return true;
			}

			TSharedRef<FJsonObject> Json = MakeShareable(new FJsonObject());
			Json->SetStringField(TEXT("status"), TEXT("accepted"));
			Json->SetStringField(TEXT("message"), TEXT("Test spec received and queued"));

			auto Response = MakeJsonResponse(SerializeJsonObject(Json));
			AddCorsHeaders(*Response, CapturedOrigins);
			OnComplete(MoveTemp(Response));
			return true;
		}));

	// GET /pof/test/results
	HttpRouter->BindRoute(FHttpPath(TEXT("/pof/test/results")), EHttpServerRequestVerbs::VERB_GET,
		FHttpRequestHandler::CreateLambda([CapturedAuthToken, CapturedOrigins](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
		{
			if (!ValidateAuth(Request, CapturedAuthToken))
			{
				OnComplete(MakeErrorResponse(TEXT("Unauthorized"), 401));
				return true;
			}

			UPofTestRunner* TestRunner = FPillarsOfFortuneBridgeEditorModule::Get().GetTestRunner();
			if (!TestRunner)
			{
				OnComplete(MakeErrorResponse(TEXT("Test runner not available"), 503));
				return true;
			}

			TSharedRef<FJsonObject> Json = MakeShareable(new FJsonObject());
			TArray<TSharedPtr<FJsonValue>> ResultArray;

			for (const FPofTestResult& Result : TestRunner->GetAllResults())
			{
				TSharedRef<FJsonObject> Obj = MakeShareable(new FJsonObject());
				Obj->SetStringField(TEXT("testId"), Result.TestId);
				Obj->SetStringField(TEXT("status"), Result.Status == EPofTestStatus::Passed ? TEXT("passed") : TEXT("failed"));
				Obj->SetStringField(TEXT("startTime"), Result.StartTime);
				Obj->SetStringField(TEXT("endTime"), Result.EndTime);
				Obj->SetNumberField(TEXT("durationMs"), Result.DurationMs);
				ResultArray.Add(MakeShareable(new FJsonValueObject(Obj)));
			}

			Json->SetArrayField(TEXT("results"), ResultArray);

			auto Response = MakeJsonResponse(SerializeJsonObject(Json));
			AddCorsHeaders(*Response, CapturedOrigins);
			OnComplete(MoveTemp(Response));
			return true;
		}));

	// POST /pof/test/run-automation
	HttpRouter->BindRoute(FHttpPath(TEXT("/pof/test/run-automation")), EHttpServerRequestVerbs::VERB_POST,
		FHttpRequestHandler::CreateLambda([CapturedAuthToken, CapturedOrigins](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
		{
			if (!ValidateAuth(Request, CapturedAuthToken))
			{
				OnComplete(MakeErrorResponse(TEXT("Unauthorized"), 401));
				return true;
			}

			// Read the requested automation test name from the body.
			TSharedPtr<FJsonObject> Body = FPofHttpRouter::ParseJsonBody(Request.Body);
			FString Filter;
			if (Body.IsValid())
			{
				Body->TryGetStringField(TEXT("filter"), Filter);
			}
			if (Filter.IsEmpty())
			{
				OnComplete(MakeErrorResponse(TEXT("Missing 'filter' (automation test name)"), 400));
				return true;
			}

			UPofTestRunner* TestRunner = FPillarsOfFortuneBridgeEditorModule::Get().GetTestRunner();
			if (!TestRunner)
			{
				OnComplete(MakeErrorResponse(TEXT("Test runner not available"), 503));
				return true;
			}

			const FPofAutomationOutcome Outcome = TestRunner->RunAutomationTest(Filter);

			TSharedRef<FJsonObject> Json = MakeShareable(new FJsonObject());
			if (!Outcome.bFound)
			{
				// No matching test — report not_found so the caller keeps the gate deferred (never a false fail).
				Json->SetStringField(TEXT("status"), TEXT("not_found"));
				Json->SetStringField(TEXT("message"), Outcome.Message);
			}
			else
			{
				Json->SetStringField(TEXT("status"), Outcome.bPassed ? TEXT("passed") : TEXT("failed"));
				Json->SetStringField(TEXT("testId"), Outcome.MatchedName);
			}

			auto Response = MakeJsonResponse(SerializeJsonObject(Json));
			AddCorsHeaders(*Response, CapturedOrigins);
			OnComplete(MoveTemp(Response));
			return true;
		}));

	// POST /pof/snapshot/capture
	HttpRouter->BindRoute(FHttpPath(TEXT("/pof/snapshot/capture")), EHttpServerRequestVerbs::VERB_POST,
		FHttpRequestHandler::CreateLambda([CapturedAuthToken, CapturedOrigins](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
		{
			if (!ValidateAuth(Request, CapturedAuthToken))
			{
				OnComplete(MakeErrorResponse(TEXT("Unauthorized"), 401));
				return true;
			}

			UPofSnapshotCapture* SnapshotCapture = FPillarsOfFortuneBridgeEditorModule::Get().GetSnapshotCapture();
			if (!SnapshotCapture)
			{
				OnComplete(MakeErrorResponse(TEXT("Snapshot capture not available"), 503));
				return true;
			}

			// Build a camera preset from the body (id required; camera fields optional → current default).
			TSharedPtr<FJsonObject> Body = FPofHttpRouter::ParseJsonBody(Request.Body);
			FPofCameraPreset Preset;
			Preset.Id = TEXT("capture");
			if (Body.IsValid())
			{
				Body->TryGetStringField(TEXT("id"), Preset.Id);
				const TArray<TSharedPtr<FJsonValue>>* Loc = nullptr;
				if (Body->TryGetArrayField(TEXT("location"), Loc) && Loc->Num() == 3)
				{
					Preset.Location = FVector((*Loc)[0]->AsNumber(), (*Loc)[1]->AsNumber(), (*Loc)[2]->AsNumber());
				}
				const TArray<TSharedPtr<FJsonValue>>* Rot = nullptr;
				if (Body->TryGetArrayField(TEXT("rotation"), Rot) && Rot->Num() == 3)
				{
					Preset.Rotation = FRotator((*Rot)[0]->AsNumber(), (*Rot)[1]->AsNumber(), (*Rot)[2]->AsNumber());
				}
				double FovValue = 0.0;
				if (Body->TryGetNumberField(TEXT("fov"), FovValue))
				{
					Preset.FOV = static_cast<float>(FovValue);
				}
			}

			const int32 Before = SnapshotCapture->GetResults().Num();
			SnapshotCapture->CapturePreset(Preset, /*bSaveAsBaseline=*/false);
			const TArray<FPofSnapshotResult>& Results = SnapshotCapture->GetResults();

			TSharedRef<FJsonObject> Json = MakeShareable(new FJsonObject());
			if (Results.Num() > Before)
			{
				const FPofSnapshotResult& Shot = Results.Last();
				Json->SetStringField(TEXT("status"), TEXT("captured"));
				Json->SetStringField(TEXT("filePath"), Shot.FilePath);
				Json->SetNumberField(TEXT("width"), Shot.Width);
				Json->SetNumberField(TEXT("height"), Shot.Height);
			}
			else
			{
				Json->SetStringField(TEXT("status"), TEXT("failed"));
				Json->SetStringField(TEXT("message"), TEXT("Capture produced no image (no active editor viewport?)"));
			}

			auto Response = MakeJsonResponse(SerializeJsonObject(Json));
			AddCorsHeaders(*Response, CapturedOrigins);
			OnComplete(MoveTemp(Response));
			return true;
		}));

	// POST /pof/snapshot/baseline
	HttpRouter->BindRoute(FHttpPath(TEXT("/pof/snapshot/baseline")), EHttpServerRequestVerbs::VERB_POST,
		FHttpRequestHandler::CreateLambda([CapturedAuthToken, CapturedOrigins](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
		{
			if (!ValidateAuth(Request, CapturedAuthToken))
			{
				OnComplete(MakeErrorResponse(TEXT("Unauthorized"), 401));
				return true;
			}

			TSharedRef<FJsonObject> Json = MakeShareable(new FJsonObject());
			Json->SetNumberField(TEXT("saved"), 0);

			auto Response = MakeJsonResponse(SerializeJsonObject(Json));
			AddCorsHeaders(*Response, CapturedOrigins);
			OnComplete(MoveTemp(Response));
			return true;
		}));

	// GET /pof/snapshot/diff
	HttpRouter->BindRoute(FHttpPath(TEXT("/pof/snapshot/diff")), EHttpServerRequestVerbs::VERB_GET,
		FHttpRequestHandler::CreateLambda([CapturedAuthToken, CapturedOrigins](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
		{
			if (!ValidateAuth(Request, CapturedAuthToken))
			{
				OnComplete(MakeErrorResponse(TEXT("Unauthorized"), 401));
				return true;
			}

			TSharedRef<FJsonObject> Json = MakeShareable(new FJsonObject());
			Json->SetStringField(TEXT("generatedAt"), FDateTime::UtcNow().ToIso8601());
			Json->SetStringField(TEXT("overallStatus"), TEXT("no-captures"));
			Json->SetArrayField(TEXT("results"), TArray<TSharedPtr<FJsonValue>>());

			auto Response = MakeJsonResponse(SerializeJsonObject(Json));
			AddCorsHeaders(*Response, CapturedOrigins);
			OnComplete(MoveTemp(Response));
			return true;
		}));

	// POST /pof/compile/live
	HttpRouter->BindRoute(FHttpPath(TEXT("/pof/compile/live")), EHttpServerRequestVerbs::VERB_POST,
		FHttpRequestHandler::CreateLambda([CapturedAuthToken, CapturedOrigins](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
		{
			if (!ValidateAuth(Request, CapturedAuthToken))
			{
				OnComplete(MakeErrorResponse(TEXT("Unauthorized"), 401));
				return true;
			}

			UPofLiveCodingBridge* LiveCoding = FPillarsOfFortuneBridgeEditorModule::Get().GetLiveCoding();
			if (!LiveCoding)
			{
				OnComplete(MakeErrorResponse(TEXT("Live coding bridge not available"), 503));
				return true;
			}

			FString ResultJson = LiveCoding->TriggerCompile();
			auto Response = MakeJsonResponse(ResultJson);
			AddCorsHeaders(*Response, CapturedOrigins);
			OnComplete(MoveTemp(Response));
			return true;
		}));

	// GET /pof/compile/status
	HttpRouter->BindRoute(FHttpPath(TEXT("/pof/compile/status")), EHttpServerRequestVerbs::VERB_GET,
		FHttpRequestHandler::CreateLambda([CapturedAuthToken, CapturedOrigins](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
		{
			if (!ValidateAuth(Request, CapturedAuthToken))
			{
				OnComplete(MakeErrorResponse(TEXT("Unauthorized"), 401));
				return true;
			}

			UPofLiveCodingBridge* LiveCoding = FPillarsOfFortuneBridgeEditorModule::Get().GetLiveCoding();

			TSharedRef<FJsonObject> Json = MakeShareable(new FJsonObject());
			Json->SetStringField(TEXT("status"), LiveCoding ? LiveCoding->GetCurrentStatus() : TEXT("unavailable"));
			Json->SetStringField(TEXT("patchPhase"), LiveCoding ? LiveCoding->GetPatchPhase() : TEXT("idle"));

			auto Response = MakeJsonResponse(SerializeJsonObject(Json));
			AddCorsHeaders(*Response, CapturedOrigins);
			OnComplete(MoveTemp(Response));
			return true;
		}));

	// POST /pof/compile/hot-patch
	HttpRouter->BindRoute(FHttpPath(TEXT("/pof/compile/hot-patch")), EHttpServerRequestVerbs::VERB_POST,
		FHttpRequestHandler::CreateLambda([CapturedAuthToken, CapturedOrigins](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
		{
			if (!ValidateAuth(Request, CapturedAuthToken))
			{
				OnComplete(MakeErrorResponse(TEXT("Unauthorized"), 401));
				return true;
			}

			UPofLiveCodingBridge* LiveCoding = FPillarsOfFortuneBridgeEditorModule::Get().GetLiveCoding();
			if (!LiveCoding)
			{
				OnComplete(MakeErrorResponse(TEXT("Live coding bridge not available"), 503));
				return true;
			}

			TSharedPtr<FJsonObject> Body = FPofHttpRouter::ParseJsonBody(Request.Body);
			if (!Body.IsValid())
			{
				OnComplete(MakeErrorResponse(TEXT("Invalid JSON body"), 400));
				return true;
			}

			FString FilePath = Body->GetStringField(TEXT("filePath"));
			FString FileContent = Body->GetStringField(TEXT("fileContent"));
			FString VerifyObjectPath = Body->GetStringField(TEXT("verifyObjectPath"));
			FString VerifyFunctionName = Body->GetStringField(TEXT("verifyFunctionName"));

			if (FilePath.IsEmpty() || FileContent.IsEmpty())
			{
				OnComplete(MakeErrorResponse(TEXT("filePath and fileContent are required"), 400));
				return true;
			}

			FString ResultJson = LiveCoding->TriggerHotPatch(FilePath, FileContent, VerifyObjectPath, VerifyFunctionName);
			auto Response = MakeJsonResponse(ResultJson);
			AddCorsHeaders(*Response, CapturedOrigins);
			OnComplete(MoveTemp(Response));
			return true;
		}));

	// GET /pof/compile/hot-patch/status
	HttpRouter->BindRoute(FHttpPath(TEXT("/pof/compile/hot-patch/status")), EHttpServerRequestVerbs::VERB_GET,
		FHttpRequestHandler::CreateLambda([CapturedAuthToken, CapturedOrigins](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
		{
			if (!ValidateAuth(Request, CapturedAuthToken))
			{
				OnComplete(MakeErrorResponse(TEXT("Unauthorized"), 401));
				return true;
			}

			UPofLiveCodingBridge* LiveCoding = FPillarsOfFortuneBridgeEditorModule::Get().GetLiveCoding();
			if (!LiveCoding)
			{
				OnComplete(MakeErrorResponse(TEXT("Live coding bridge not available"), 503));
				return true;
			}

			FString StatusJson = LiveCoding->GetFullStatusJson();
			auto Response = MakeJsonResponse(StatusJson);
			AddCorsHeaders(*Response, CapturedOrigins);
			OnComplete(MoveTemp(Response));
			return true;
		}));

	// POST /pof/python/run  — dispatch a python module.function(args) on the editor thread.
	// Body shape: {"module": "x.y", "function": "run", "args": {...}}
	// Response shape: the parsed __POF_BRIDGE_RESULT__ JSON from the wrapper (ok/data|error/logs).
	HttpRouter->BindRoute(FHttpPath(TEXT("/pof/python/run")), EHttpServerRequestVerbs::VERB_POST,
		FHttpRequestHandler::CreateLambda([CapturedAuthToken, CapturedOrigins](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
		{
			if (!ValidateAuth(Request, CapturedAuthToken))
			{
				OnComplete(MakeErrorResponse(TEXT("Unauthorized"), 401));
				return true;
			}

			TSharedPtr<FJsonObject> Body = FPofHttpRouter::ParseJsonBody(Request.Body);
			if (!Body.IsValid())
			{
				OnComplete(MakeErrorResponse(TEXT("Invalid JSON body"), 400));
				return true;
			}

			FString Module, Function;
			if (!Body->TryGetStringField(TEXT("module"), Module) || Module.IsEmpty() ||
				!Body->TryGetStringField(TEXT("function"), Function) || Function.IsEmpty())
			{
				OnComplete(MakeErrorResponse(TEXT("Missing 'module' or 'function'"), 400));
				return true;
			}

			// Re-serialize the args sub-object so we can pass it as a JSON literal into Python.
			FString ArgsJson = TEXT("{}");
			const TSharedPtr<FJsonObject>* ArgsObj = nullptr;
			if (Body->TryGetObjectField(TEXT("args"), ArgsObj) && ArgsObj && (*ArgsObj).IsValid())
			{
				TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ArgsJson);
				FJsonSerializer::Serialize((*ArgsObj).ToSharedRef(), Writer);
			}

			UPofPythonRunner* Runner = FPillarsOfFortuneBridgeEditorModule::Get().GetPythonRunner();
			if (!Runner)
			{
				OnComplete(MakeErrorResponse(TEXT("Python runner not available"), 503));
				return true;
			}

			const FPofPythonOutcome Outcome = Runner->Run(Module, Function, ArgsJson);

			// On transport failure (no marker / Python plugin missing), wrap as a structured ok:false.
			FString ResponseJson;
			if (Outcome.ResultJson.IsEmpty())
			{
				TSharedRef<FJsonObject> J = MakeShareable(new FJsonObject());
				J->SetBoolField(TEXT("ok"), false);
				J->SetStringField(TEXT("error"), Outcome.ErrorMessage.IsEmpty()
					? TEXT("Python invocation produced no result")
					: Outcome.ErrorMessage);
				ResponseJson = SerializeJsonObject(J);
			}
			else
			{
				// Pass-through the wrapper's JSON verbatim.
				ResponseJson = Outcome.ResultJson;
			}

			auto Response = MakeJsonResponse(ResponseJson);
			AddCorsHeaders(*Response, CapturedOrigins);
			OnComplete(MoveTemp(Response));
			return true;
		}));

	HttpServerModule.StartAllListeners();
	bIsRunning = true;

	UE_LOG(LogPofBridge, Log, TEXT("PoF Bridge HTTP server started on port %d"), Port);
}

void FPofHttpServer::Stop()
{
	HttpRouter.Reset();
	bIsRunning = false;

	UE_LOG(LogPofBridge, Log, TEXT("PoF Bridge HTTP server stopped"));
}
