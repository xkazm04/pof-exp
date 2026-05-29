"""Download Mixamo locomotion clips via the internal API — no browser clicking.

Auth: pass the bearer token via the MIXAMO_TOKEN env var (read from the logged-in
session's localStorage.access_token). The token is never logged.

Flow per clip:
  1. GET  /products?query=<name>&type=Motion,MotionPack   → pick best product match
  2. GET  /products/{id}?character_id={char}              → details.gms_hash
  3. POST /animations/export  {character_id, gms_hash, preferences, product_name, type}
  4. GET  /characters/{char}/monitor  (poll)             → status + job_result URL
  5. download the FBX → Content/Source/Mixamo/Raw/<Name>.fbx

Idempotent: existing FBX files are skipped. Standalone (uses `requests`); run with:
  MIXAMO_TOKEN=<jwt> python mixamo_download.py
"""

from __future__ import annotations

import os
import re
import sys
import time

import requests

API = "https://www.mixamo.com/api/v1"
RAW_DIR = r"C:\Users\kazda\Documents\Unreal Projects\PoF\Content\Source\Mixamo\Raw"

# Canonical retarget-source mannequins (Mixamo system characters). X Bot is the
# standard neutral rig everyone retargets to the UE5 Manny skeleton from.
XBOT_UUID = "2dee24f8-3b49-48af-b735-c6377509eaac"
YBOT_UUID = "4f5d21e1-4ccc-41f1-b35b-fb2547bd8493"

# (target filename stem, Mixamo search query, in_place)
# Forward_Roll uses in_place=False so its root motion drives the dodge displacement.
CLIPS = [
    ("Standard_Idle", "idle", True),
    ("Walking", "walking", True),
    ("Walking_Backwards", "walking backwards", True),
    ("Left_Strafe_Walking", "left strafe walking", True),
    ("Right_Strafe_Walking", "right strafe walking", True),
    ("Running", "running", True),
    ("Running_Backward", "running backward", True),
    ("Left_Strafe", "left strafe", True),
    ("Right_Strafe", "right strafe", True),
    ("Forward_Roll", "forward roll", False),
]


def headers(token: str) -> dict:
    return {
        "Authorization": f"Bearer {token}",
        "X-Api-Key": "mixamo2",
        "Accept": "application/json",
        "Content-Type": "application/json",
        "Origin": "https://www.mixamo.com",
        "Referer": "https://www.mixamo.com/",
    }


def get_default_character(token: str) -> str:
    """Resolve the X Bot system character uuid (canonical retarget source).

    Verifies X Bot is in the account's character list; falls back to the pinned
    constant, then Y Bot, so a list-shape change can't break the run.
    """
    try:
        r = requests.get(f"{API}/characters", headers=headers(token), timeout=30)
        r.raise_for_status()
        chars = r.json()
        if isinstance(chars, list):
            for c in chars:
                if (c.get("name") or "").lower() == "x bot":
                    return c["uuid"]
            for c in chars:
                if (c.get("name") or "").lower() == "y bot":
                    return c["uuid"]
    except Exception:  # noqa: BLE001
        pass
    return XBOT_UUID


def find_product(token: str, query: str) -> dict | None:
    """Search products; return the closest Motion match to `query`."""
    r = requests.get(
        f"{API}/products",
        headers=headers(token),
        params={"page": 1, "limit": 24, "type": "Motion,MotionPack", "query": query},
        timeout=30,
    )
    r.raise_for_status()
    results = r.json().get("results", [])
    motions = [p for p in results if p.get("type") == "Motion"] or results
    if not motions:
        return None

    q = query.strip().lower()
    # 1. Exact name match wins outright (e.g. "Idle" over "Fight Idle To Standing Idle").
    for p in motions:
        if p.get("name", "").strip().lower() == q:
            return p

    # 2. Otherwise rank by token overlap, penalising extra/missing tokens heavily so a
    #    short exact-ish name beats a long transition clip that merely contains the words.
    q_tokens = set(re.findall(r"[a-z]+", q))

    def score(p: dict) -> int:
        name = p.get("name", "").lower()
        n_tokens = set(re.findall(r"[a-z]+", name))
        return len(q_tokens & n_tokens) - 2 * len(n_tokens - q_tokens)

    return max(motions, key=score)


def get_gms_hash(token: str, product_id: str, character_id: str) -> list:
    r = requests.get(
        f"{API}/products/{product_id}",
        headers=headers(token),
        params={"similar": 0, "character_id": character_id},
        timeout=30,
    )
    r.raise_for_status()
    details = r.json().get("details", {})
    gms = details.get("gms_hash")
    if not gms:
        raise RuntimeError(f"no gms_hash for product {product_id}")
    return gms if isinstance(gms, list) else [gms]


def prepare_gms(gms_hash: list, in_place: bool) -> list:
    """Transform product-details gms_hash into the export shape.

    The details endpoint returns params as nested pairs, e.g. [["Overdrive", 0.0]];
    the export endpoint requires params as a comma-joined string of the *values*
    ("0.0"). All other fields (model-id, mirror, trim, arm-space) pass through.
    Without this transform the export job fails server-side
    ("Error while generating the animation").
    """
    out = []
    for model in gms_hash:
        m = dict(model)
        params = m.get("params")
        if isinstance(params, list):
            m["params"] = ",".join(str(p[1]) for p in params if isinstance(p, (list, tuple)) and len(p) >= 2)
        m["inplace"] = in_place
        out.append(m)
    return out


def export_and_download(token: str, character_id: str, stem: str, query: str,
                        in_place: bool, with_skin: bool) -> str:
    product = find_product(token, query)
    if not product:
        raise RuntimeError(f"no product found for '{query}'")
    product_id = product["id"]
    product_name = product.get("name", stem)

    gms_hash = prepare_gms(get_gms_hash(token, product_id, character_id), in_place)

    export_body = {
        "character_id": character_id,
        "gms_hash": gms_hash,
        "preferences": {
            "format": "fbx7",
            "skin": "true" if with_skin else "false",
            "fps": "30",
            "reducekf": "0",
        },
        "product_name": product_name,
        "type": "Motion",
    }
    r = requests.post(f"{API}/animations/export", headers=headers(token),
                      json=export_body, timeout=60)
    r.raise_for_status()

    # Poll the monitor endpoint
    deadline = time.time() + 180
    job_url = None
    while time.time() < deadline:
        mr = requests.get(f"{API}/characters/{character_id}/monitor",
                          headers=headers(token), timeout=30)
        mr.raise_for_status()
        mon = mr.json()
        status = mon.get("status")
        if status == "completed":
            job_url = mon.get("job_result")
            break
        if status == "failed":
            raise RuntimeError(f"export failed for {stem}: {mon.get('message')}")
        time.sleep(2)
    if not job_url:
        raise RuntimeError(f"export timed out for {stem}")

    # Download the FBX
    dst = os.path.join(RAW_DIR, f"{stem}.fbx")
    with requests.get(job_url, timeout=120, stream=True) as dr:
        dr.raise_for_status()
        with open(dst, "wb") as fh:
            for chunk in dr.iter_content(chunk_size=1 << 16):
                fh.write(chunk)
    return dst


def main() -> int:
    token = os.environ.get("MIXAMO_TOKEN", "").strip()
    if not token:
        print("ERROR: set MIXAMO_TOKEN env var with the bearer token")
        return 2

    os.makedirs(RAW_DIR, exist_ok=True)
    try:
        character_id = get_default_character(token)
    except Exception as e:  # noqa: BLE001
        print(f"ERROR resolving character: {e}")
        return 3
    print(f"character_id={character_id}")

    created, skipped, failed = [], [], []
    for i, (stem, query, in_place) in enumerate(CLIPS):
        dst = os.path.join(RAW_DIR, f"{stem}.fbx")
        if os.path.exists(dst) and os.path.getsize(dst) > 20_000:
            skipped.append(stem)
            print(f"[skip] {stem} (exists)")
            continue
        try:
            with_skin = (i == 0)  # first clip brings the rig + mesh
            export_and_download(token, character_id, stem, query, in_place, with_skin)
            created.append(stem)
            print(f"[ok]   {stem}")
        except Exception as e:  # noqa: BLE001
            failed.append(f"{stem}: {e}")
            print(f"[fail] {stem}: {e}")

    print(f"\nSUMMARY created={len(created)} skipped={len(skipped)} failed={len(failed)}")
    if failed:
        for f in failed:
            print(f"  FAIL {f}")
    return 0 if not failed else 1


if __name__ == "__main__":
    sys.exit(main())
