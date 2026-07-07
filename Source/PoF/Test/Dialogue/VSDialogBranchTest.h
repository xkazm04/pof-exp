#pragma once

#include "CoreMinimal.h"

/**
 * Dialog Trees automation gate — see VSDialogBranchTest.cpp.
 *
 * The gate is an IMPLEMENT_SIMPLE_AUTOMATION_TEST (no map / no PIE) that
 * builds an in-memory UARPGDialogueTree with a real branch and asserts the
 * structural integrity rules the data model supports: every choice/next
 * reference resolves, a terminal is reachable from the root, no node
 * self-loops, GetNode lookup returns the right node per choice, and
 * IsValid() actually rejects dangling references and bad start indices.
 *
 * No UCLASS helpers needed — the tree is a plain data asset with no dynamic
 * delegates, so this header only anchors the include convention shared with
 * the other VS* gates (see Test/Economy/VSCurrencyWalletTest.h).
 */
