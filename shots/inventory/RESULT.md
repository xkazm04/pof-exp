# Inventory Stream — E2E Headless Proof (Stream 4)

Scenario: `shots/inventory/inv-loot-heal.json` on `/Game/Maps/Test_Inventory`, run headless
via the Observation Spine (`-game -RenderOffScreen -PoFScenario=…`). Output:
`Saved/Observations/inv/` (observations.json + shot_NN.png + frame_NN.png).

Timeline: at settle the player's Health is set to 50; `loot_chest`@0.3s opens the potion
chest; `collect_loot`@1.2s picks the dropped potion into the player's inventory (real
`AARPGWorldItem::TryPickup` → `UARPGInventoryComponent::AddItem`); `use_item`@2.6s drinks it.

## Observed (observations.json) — all three gates pass on real output

| t (s) | health | health_gas | inventory_count | has_potion | inventory |
|------:|-------:|-----------:|----------------:|:----------:|-----------|
| 0.50  | 50     | 50         | 0               | false      | — |
| 1.02  | 50     | 50         | 0               | false      | — |
| 1.52  | 50     | 50         | **1**           | **true**   | **Minor Health Potion ×1** |
| 2.02  | 50     | 50         | 1               | true       | Minor Health Potion ×1 |
| 2.52  | 50     | 50         | 1               | true       | Minor Health Potion ×1 |
| 3.02  | **100**| **100**    | **0**           | false      | — (consumed) |
| 3.52  | 100    | 100        | 0               | false      | — |
| 4.02  | 100    | 100        | 0               | false      | — |

- **Gate A — loot:** after `collect_loot`, the inventory holds **"Minor Health Potion" ×1**
  (`has_potion=true`, `inventory_count=1`).
- **Gate B — heal:** Health holds at **50** through t=2.52, then **100** at t=3.02 after
  `use_item` — confirmed by both the mirrored `health` and the authoritative GAS `health_gas`.
  The potion is consumed (`inventory_count` → 0).
- **Gate C — UI frame (read):** the Slate inventory overlay shows **"(empty)"** at t=0.5
  (`shot_00.png`) and **"Minor Health Potion  x1"** at t=2.0 (`shot_03.png`) over the lit
  Arena_Ancient view — the UI is data-driven and reflects the looted contents. Both frames
  were read directly (the T4 perceptual authority).

No "done" without a frame/observation: every gate above is ground truth from the headless run.
