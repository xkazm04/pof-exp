#pragma once

#include "CoreMinimal.h"

// =============================================================================
// Structured Log Categories
// Usage: UE_LOG(LogARPGCombat, Warning, TEXT("..."));
// Filter in editor: OutputLog > Filters, or -LogCmds="LogARPGCombat Verbose"
// =============================================================================

DECLARE_LOG_CATEGORY_EXTERN(LogARPGCombat,    Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogARPGAI,        Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogARPGInventory,  Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogARPGAbility,    Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogARPGQuest,      Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogARPGWorld,      Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogARPGSave,       Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogARPGUI,         Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogARPGAudio,      Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogARPGNetwork,    Log, All);
// Lifecycle marker channel — use the ARPG_LIFECYCLE_LOG macro from
// ARPGLifecycleLog.h at BeginPlay / PossessedBy / Bind* edges so a "why didn't
// this run" diagnosis starts with `grep LogARPGLifecycle` in the live log.
DECLARE_LOG_CATEGORY_EXTERN(LogARPGLifecycle,  Log, All);
