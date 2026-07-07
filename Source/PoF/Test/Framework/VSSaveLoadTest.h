#pragma once

#include "CoreMinimal.h"

/**
 * Save Points automation gate — see VSSaveLoadTest.cpp.
 *
 * The gate is an IMPLEMENT_SIMPLE_AUTOMATION_TEST (no map / no PIE) that
 * round-trips a fully populated UARPGSaveGame through
 * UGameplayStatics::SaveGameToMemory / LoadGameFromMemory and asserts every
 * written field survives byte-for-byte, plus the anti-corruption behaviors
 * (version stamp, empty-buffer load returns nullptr).
 *
 * No UCLASS helpers needed — the save round-trip involves no dynamic
 * delegates, so this header only anchors the include convention shared with
 * the other VS* gates (see Test/Economy/VSCurrencyWalletTest.h).
 */
