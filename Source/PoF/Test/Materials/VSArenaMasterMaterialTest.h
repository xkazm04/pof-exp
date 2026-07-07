#pragma once

#include "CoreMinimal.h"

/**
 * Materials Test Gate — arena master materials packaging truth.
 *
 * No UObject helpers are needed here (unlike the wallet/quest gates, nothing
 * binds to a dynamic delegate), so this header intentionally carries no
 * UCLASS / .generated.h. The test class itself is declared by
 * IMPLEMENT_SIMPLE_AUTOMATION_TEST in VSArenaMasterMaterialTest.cpp.
 *
 * Gate contract: the three ArenaBuild master materials
 * (/Game/ArenaBuild/M_Arena_Floor, M_Arena_Pillar, M_Arena_Wall) must load,
 * be genuinely parameterized, and carry a valid blend/shading configuration.
 * A missing asset is a hard failure — that IS the packaging truth.
 */
