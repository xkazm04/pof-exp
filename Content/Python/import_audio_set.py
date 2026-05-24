"""Imports an audio set into UE: USoundWaves + a randomising USoundCue.

Inputs (env vars):
  AUDIO_SET_NAME    - e.g. "footstep-stone"
  AUDIO_EVENT_KEY   - e.g. "footstep" (optional)
  AUDIO_SURFACE     - e.g. "stone"    (optional)
  AUDIO_SOURCES     - semicolon-separated absolute source paths (mp3 or wav)

Outputs (printed line, parsed by the CLI):
  [import_audio_set] DONE assetsImported=N cuePath=/Game/Audio/<set>/SC_<set> wiredEvent=<name|null>
"""

import os
import shutil
import subprocess
import tempfile
import unreal


def _env(k, default=""):
    v = os.environ.get(k, default)
    return v.strip() if isinstance(v, str) else v


def _expand(p):
    return os.path.expanduser(os.path.expandvars(p))


def _convert_to_wav(src):
    """Returns (wav_path, tempfile_to_cleanup_or_None). mp3 -> tempfile via ffmpeg; wav -> src itself."""
    src = _expand(src)
    if src.lower().endswith(".wav"):
        return src, None
    if not shutil.which("ffmpeg"):
        unreal.log_warning("[import_audio_set] ffmpeg not on PATH; skipping mp3 source: " + src)
        return None, None
    tmp = tempfile.NamedTemporaryFile(suffix=".wav", delete=False)
    tmp.close()
    cmd = ["ffmpeg", "-y", "-i", src, "-ac", "1", "-ar", "44100", "-sample_fmt", "s16", tmp.name]
    r = subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    if r.returncode != 0:
        try: os.unlink(tmp.name)
        except Exception: pass
        return None, None
    return tmp.name, tmp.name


def main():
    set_name = _env("AUDIO_SET_NAME", "untitled")
    event_key = _env("AUDIO_EVENT_KEY")
    surface = _env("AUDIO_SURFACE")
    sources_raw = _env("AUDIO_SOURCES")
    sources = [s for s in sources_raw.split(";") if s.strip()]

    dest_dir = "/Game/Audio/" + set_name
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

    wave_paths = []
    cleanup = []
    for src in sources:
        wav, tmp = _convert_to_wav(src)
        if tmp:
            cleanup.append(tmp)
        if not wav or not os.path.exists(wav):
            continue
        task = unreal.AssetImportTask()
        task.filename = wav
        task.destination_path = dest_dir
        task.replace_existing = True
        task.automated = True
        task.save = True
        asset_tools.import_asset_tasks([task])
        for p in task.imported_object_paths:
            wave_paths.append(p.split(".")[0])  # /Game/Audio/<set>/<name>

    # Build SC_<set> USoundCue
    cue_name = "SC_" + set_name.replace("-", "_")
    cue_path = None
    if wave_paths:
        cue_factory = unreal.SoundCueFactoryNew()
        cue = asset_tools.create_asset(
            asset_name=cue_name,
            package_path=dest_dir,
            asset_class=unreal.SoundCue,
            factory=cue_factory,
        )
        if cue:
            # Best-effort node graph: create a Random node + WavePlayer per wave.
            # The Python API for wiring SoundCue nodes differs across UE versions;
            # the cue + waves exist regardless and an operator can finalise wiring.
            try:
                cue.construct_sound_node(unreal.SoundNodeRandom)
                for wp in wave_paths:
                    wave = unreal.load_asset(wp)
                    if wave:
                        wn = cue.construct_sound_node(unreal.SoundNodeWavePlayer)
                        try:
                            wn.set_editor_property("sound_wave", wave)
                        except Exception:
                            pass
            except Exception as e:
                unreal.log_warning("[import_audio_set] node graph wiring partial: " + str(e))
            unreal.EditorAssetLibrary.save_loaded_asset(cue)
            cue_path = dest_dir + "/" + cue_name

    # Cleanup tempfiles
    for f in cleanup:
        try: os.unlink(f)
        except Exception: pass

    wired = "null"  # AnimNotify auto-wiring is a follow-up dispatch.
    print("[import_audio_set] DONE assetsImported={} cuePath={} wiredEvent={}".format(
        len(wave_paths), cue_path or "null", wired))


if __name__ == "__main__":
    try:
        main()
    finally:
        try:
            if unreal.is_editor():
                unreal.SystemLibrary.quit_editor()
        except Exception:
            pass
