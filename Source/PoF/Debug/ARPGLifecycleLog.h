#pragma once

#include "Debug/ARPGLogCategories.h"

/**
 * Lifecycle marker — logs the calling function name to the greppable
 * LogARPGLifecycle category. Use at lifecycle edges (BeginPlay / PossessedBy /
 * Bind*) only, so "why didn't this run" diagnosis is:
 *     grep LogARPGLifecycle <log>
 *
 * Format: `[FunctionName] <message>`.
 *
 * Example:
 *     ARPG_LIFECYCLE_LOG(Log, TEXT("Possessed by %s"), *NewController->GetName());
 *
 * Do NOT use for runtime events (damage applied, item picked up, etc.) — those
 * belong in their own domain category (LogARPGCombat, LogARPGInventory, …).
 */
#define ARPG_LIFECYCLE_LOG(Verbosity, Format, ...) \
	UE_LOG(LogARPGLifecycle, Verbosity, TEXT("[%s] ") Format, ANSI_TO_TCHAR(__FUNCTION__), ##__VA_ARGS__)
