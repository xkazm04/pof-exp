# Source/PoF/Test — functional & automation tests

Conventions for the in-engine test suite. Infrastructure here; per-system
gameplay tests live in their system's folder (authored per the matching
`docs/improvements/0X-*/tests.md`).

## Base class

`AARPGFunctionalTestBase` (`ARPGFunctionalTestBase.{h,cpp}`) provides a phased
Tick driver + GAS/character helpers. A new per-system functional test:

```cpp
UCLASS()
class POF_API AVSCharacterShapeTest : public AARPGFunctionalTestBase
{
    GENERATED_BODY()
public:
    AVSCharacterShapeTest() { Phases = { "ResolveActors", "AssertShape" }; }
protected:
    virtual EARPGPhaseResult RunPhase(int32 Idx, FName Name, float Dt) override;
};
```

Fill `Phases` (a `TArray<FName>`) in the constructor and implement `RunPhase`,
returning `EARPGPhaseResult::{Running, Advance, Fail}`. The base advances
phases, enforces `TimeLimit` + the optional `PhaseTimeout`, finishes the test,
and ignores warning-level log (gray-box anim warnings are not failures).
Helpers: `GetPlayerCharacter()`, `GetFirstEnemy()`, `GetPlayerASC()`,
`GetEnemyASC()`, `ApplyDamage(target, amount)` (real GE_Damage pipeline),
`GetHealth(actor)`, `WaitForCondition(predicate, timeout)`,
`IsFirstTickOfPhase(dt)`, `GetPhaseTime()`.

## Folder layout

| Folder         | System              | Tests source                         |
|----------------|---------------------|--------------------------------------|
| `Character/`   | character mesh/shape | `02-*/tests.md`                      |
| `Combat/`      | GAS abilities/damage | `03-*/tests.md`                      |
| `HUD/`         | HUD presence/binding | `04-*/tests.md`                      |
| `Environment/` | arena collision/nav  | `05-*/tests.md`                      |
| `Materials/`   | material binding     | `06-*/tests.md`                      |
| `HealthCheck/` | project invariants   | this folder (`PoFHealthCheckTest`)   |

Map-bound checks subclass `AARPGFunctionalTestBase` and are placed in the
slice map (auto-discovered by the automation framework). Map-free checks (pure
GAS execution, material-graph connection, project structure) use
`IMPLEMENT_SIMPLE_AUTOMATION_TEST` in the same folder. Both run via one
`Automation RunTests` invocation.

## Running

```
UnrealEditor-Cmd PoF.uproject /Game/Maps/VerticalSlice \
  -ExecCmds="Automation RunTests Project.Functional Tests.PoF.HealthCheck;Quit" \
  -unattended -nopause -nullrhi -log
```

Run `PoF.HealthCheck` first in CI: it fails fast and deterministically when the
project *structure* breaks, before per-system tests fail in confusing ways.
