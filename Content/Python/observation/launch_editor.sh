#!/usr/bin/env bash
# Hardened UE editor launch for bridge-driven work (SP1 Observation Spine).
#
# A force-kill of the editor (used to release a DLL lock for rebuilds) leaves a
# crash-recovery MODAL that hangs the next launch and blocks the bridge's game
# thread. This clears the recovery state and launches -unattended so dialogs
# auto-dismiss. Then poll http://localhost:30040/pof/status until ready.
set -euo pipefail
UE="/c/Program Files/Epic Games/UE_5.7/Engine/Binaries/Win64/UnrealEditor.exe"
PROJ='C:\Users\kazda\Documents\Unreal Projects\PoF\PoF.uproject'
SAVED="/c/Users/kazda/Documents/Unreal Projects/PoF/Saved"
INTER="/c/Users/kazda/Documents/Unreal Projects/PoF/Intermediate"
LOG_WIN="${1:-C:\\Users\\kazda\\AppData\\Local\\Temp\\pof_editor.log}"

rm -rf "$SAVED/Autosaves" "$SAVED/Crashes" "$SAVED/SaveRecovery" "$INTER/DisasterRecovery" 2>/dev/null || true

"$UE" "$PROJ" -unattended -abslog="$LOG_WIN" &
echo "launched UE editor (log: $LOG_WIN)"
echo "poll: for i in \$(seq 1 75); do curl -s http://localhost:30040/pof/status && break; sleep 4; done"
