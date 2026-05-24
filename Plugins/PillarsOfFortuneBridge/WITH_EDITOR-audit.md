# PillarsOfFortuneBridge Runtime — WITH_EDITOR Audit

**Date:** 2026-05-24
**Auditor:** generation-quality §01 closure session
**Scope:** the `PillarsOfFortuneBridge` **runtime** module (not `…BridgeEditor`),
under `Plugins/PillarsOfFortuneBridge/Source/PillarsOfFortuneBridge/`.

## Background

`docs/improvements/01-generation-quality/game.md` §5 — "A small `WITH_EDITOR`
audit pass for the bridge plugin": `PofTestRunner.cpp` had one unguarded
`FEditorDelegates` call in a Runtime module — caught only when SP-C cooked a
Shipping build. This audit records the current state of every editor-only API
the runtime module touches.

## Methodology

Read-only grep over `Plugins/PillarsOfFortuneBridge/Source/PillarsOfFortuneBridge/`
for: `FEditorDelegates`, `GEditor`, `UnrealEd`, `FAssetRegistryModule`,
`GIsEditor`, `WITH_EDITOR`, `EditorEngine`, `FEditor`, `ULevelEditor`,
`EditorViewport`, `EditorAsset`, `FPropertyEditor`, `FBlueprintEditor`,
`UAssetEditorSubsystem`, `UToolMenus`, `UPackage::Save`, `SourceControl`.

## Per-site findings (every site)

| Site | API | Disposition |
|---|---|---|
| `PofTestRunner.cpp:10–14` | `#include UnrealEdGlobals.h / Editor/UnrealEdEngine.h` | ✅ Inside `#if WITH_EDITOR` |
| `PofTestRunner.cpp:36` | `#if WITH_EDITOR` opens implementation block | ✅ Guarded |
| `PofTestRunner.cpp:37–43` | `GEditor->IsPlaySessionInProgress`, `GEditor->RequestPlaySession`, `FEditorDelegates::PostPIEStarted.AddUObject` | ✅ Inside the line-36 guard |
| `PofTestRunner.cpp:53–54` | `FEditorDelegates::PostPIEStarted.RemoveAll` | ✅ Inside `#if WITH_EDITOR` block (line 53) |
| `PofTestRunner.cpp:252` | `#if WITH_EDITOR` block | ✅ Guarded |
| `PofSnapshotCapture.cpp:8, 10–11` | `#include Editor/UnrealEdEngine.h`, `LevelEditorViewport.h` | ✅ Inside `#if WITH_EDITOR` |
| `PofSnapshotCapture.cpp:16–30` | `GEditor->GetLevelViewportClients`, `FLevelEditorViewportClient*` | ✅ Inside `#if WITH_EDITOR` (line 16) |
| `PofAssetManifest.cpp:20, 42, 95` | `FAssetRegistryModule` via `FModuleManager` | ✅ Runtime-safe (the `AssetRegistry` module ships) |
| `PofBridgeSettings.h:4` | `#include Engine/DeveloperSettings.h` | ✅ Runtime-safe (`DeveloperSettings` module ships) |
| `PillarsOfFortuneBridge.Build.cs:24–29` | `PrivateDependencyModuleNames += "UnrealEd"` | ✅ Inside `if (Target.bBuildEditor)` guard |

**Conclusion (source-level):** every editor-only symbol in the bridge runtime is
inside a `#if WITH_EDITOR` block or an `if (Target.bBuildEditor)` gate. The
PS-1 / SP-C unguarded `FEditorDelegates` call has been fully addressed. No
fixes needed for the bridge runtime as part of this audit.

## Shipping-compile proof — attempted 2026-05-24

Spec's stronger gate is a Shipping compile of the runtime target:

```
Build.bat PoF Win64 Shipping -Project=…\PoF.uproject -WaitMutex -NoHotReloadFromIDE -abslog=…
```

**Result: Failed (OtherCompilationError) — but the failure is outside this
audit's scope.** Verbatim error excerpt:

> Missing precompiled manifest for 'UnrealEdMessages',
> 'C:\Program Files\Epic Games\UE_5.7\Engine\Intermediate\Build\Win64\x64\UnrealGame\Shipping\UnrealEdMessages\UnrealEdMessages.precompiled'.
> This module can not be referenced in a monolithic precompiled build, remove
> this reference or migrate to a fully compiled source build.
> Dependent modules 'AutomationController'

`AutomationController` is pulled in transitively from `FunctionalTesting`,
which is listed unconditionally in **`Source/PoF/PoF.Build.cs`** (the game
module, **not** the bridge plugin). `FunctionalTesting` → `AutomationController`
→ `UnrealEdMessages` (editor-only). So the Shipping build of the
runtime target fails on a **game-module** Build.cs issue, not on anything in
the bridge plugin. Source-level audit of the bridge (above) stands.

## Follow-ups (NOT part of this §5 audit)

1. **Game module `FunctionalTesting` gate.** `Source/PoF/PoF.Build.cs` should
   wrap `"FunctionalTesting"` in `if (Target.Configuration != UnrealTargetConfiguration.Shipping || Target.bBuildEditor)`
   (or move all `Source/PoF/Test/*` + the dependency into a separate test module
   that is excluded from Shipping). This is a §07 (packaging-build) concern,
   not §01 §5.
2. **Re-run the Shipping-compile gate** once #1 is fixed — that will give the
   end-to-end proof that the bridge runtime is editor-clean against a real
   Shipping link, not just source-level greps.

## Audit guidance for the next pass

When adding new code to the bridge runtime module:
- Every `GEditor`, `FEditorDelegates`, `FLevelEditorViewport*`, `UEditor*`,
  `UnrealEd*` reference goes inside `#if WITH_EDITOR`.
- Editor-only `#include`s go inside `#if WITH_EDITOR` too — the precompiled
  Shipping headers don't ship those.
- New editor-only module deps in `PillarsOfFortuneBridge.Build.cs` go inside
  `if (Target.bBuildEditor)` (as `UnrealEd` does today).
- Prefer: move editor-only logic into the `PillarsOfFortuneBridgeEditor` module
  entirely. The runtime module should be runtime-pure.
