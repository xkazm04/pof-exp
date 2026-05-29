"""Ground-truth observation verbs (SP1 Observation Spine).

Each verb's run(args) returns an Observation envelope:
    {kind, data, captured_at, scenario_id?}
mirrored by src/lib/observation/types.ts. Verbs are dispatched via the
/pof/python/run bridge route (module=observation.<verb>, function=run).
"""
from __future__ import annotations

import datetime


def make_observation(kind: str, data: dict, scenario_id: str | None = None) -> dict:
    obs = {
        "kind": kind,
        "data": data,
        "captured_at": datetime.datetime.now(datetime.timezone.utc).isoformat(),
    }
    if scenario_id is not None:
        obs["scenario_id"] = scenario_id
    return obs
