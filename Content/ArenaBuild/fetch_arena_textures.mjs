// PS-3: generate 3 seamless tileable arena textures via the Leonardo API.
// Every generation is DOWNLOAD-THEN-DELETE — no working assets are left on the
// Leonardo account (the local PNG is the only retained copy).
import { writeFile, readFile, mkdir } from 'node:fs/promises';
import { join, dirname } from 'node:path';
import { fileURLToPath } from 'node:url';

const API = 'https://cloud.leonardo.ai/api/rest/v1';
const MODEL_LUCID_REALISM = '05ce0082-2d80-4a2d-8653-4d1c85e2418e';
const OUT = join(dirname(fileURLToPath(import.meta.url)), 'textures_v2');

const PROMPTS = {
  floor:  'seamless tileable texture, dark fantasy dungeon stone floor, weathered fitted flagstones, even top-down lighting, no shadows, no vignette, flat material albedo',
  wall:   'seamless tileable texture, dark fantasy dungeon wall, mossy carved stone blocks, even lighting, no shadows, no vignette, flat material albedo',
  pillar: 'seamless tileable texture, carved dark stone column surface, weathered granite, even lighting, no shadows, no vignette, flat material albedo',
};

async function loadKey() {
  const env = await readFile('C:/Users/kazda/kiro/personas/.env', 'utf-8');
  const m = env.match(/^LEONARDO_API_KEY=(.+)$/m);
  if (!m) throw new Error('LEONARDO_API_KEY not found in personas/.env');
  return m[1].trim();
}
function headers(key) {
  return { accept: 'application/json', 'content-type': 'application/json',
           authorization: `Bearer ${key}` };
}
async function generate(key, prompt) {
  const r = await fetch(`${API}/generations`, {
    method: 'POST', headers: headers(key),
    body: JSON.stringify({ prompt, modelId: MODEL_LUCID_REALISM,
      width: 1024, height: 1024, num_images: 1, tiling: true }),
  });
  if (!r.ok) throw new Error(`POST /generations ${r.status}: ${await r.text()}`);
  const id = (await r.json())?.sdGenerationJob?.generationId;
  if (!id) throw new Error('no generationId in POST response');
  return id;
}
async function pollImage(key, id) {
  for (let i = 0; i < 60; i++) {
    await new Promise((res) => setTimeout(res, 4000));
    const r = await fetch(`${API}/generations/${id}`, { headers: headers(key) });
    if (!r.ok) throw new Error(`GET /generations/${id} ${r.status}`);
    const g = (await r.json())?.generations_by_pk;
    if (g?.status === 'COMPLETE') {
      const url = g.generated_images?.[0]?.url;
      if (!url) throw new Error('COMPLETE but no image url');
      return url;
    }
    if (g?.status === 'FAILED') throw new Error('generation FAILED');
  }
  throw new Error('generation timed out');
}
async function deleteGeneration(key, id) {
  const r = await fetch(`${API}/generations/${id}`, { method: 'DELETE', headers: headers(key) });
  if (r.ok) console.log(`  deleted generation ${id}`);
  else console.log(`  WARN: DELETE /generations/${id} -> ${r.status} (not cleaned up)`);
}
async function download(url, path) {
  const r = await fetch(url);
  if (!r.ok) throw new Error(`download ${r.status}`);
  await writeFile(path, Buffer.from(await r.arrayBuffer()));
}

const key = await loadKey();
await mkdir(OUT, { recursive: true });
for (const [slot, prompt] of Object.entries(PROMPTS)) {
  console.log(`[${slot}] generating...`);
  const id = await generate(key, prompt);
  try {
    const url = await pollImage(key, id);
    const out = join(OUT, `${slot}_albedo.png`);
    await download(url, out);
    console.log(`  saved ${out}`);
  } finally {
    await deleteGeneration(key, id);   // download-then-delete — always clean up
  }
}
console.log('ARENA_TEXTURES_DONE');
