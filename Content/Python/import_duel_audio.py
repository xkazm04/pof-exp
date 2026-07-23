"""Import the generated duel audio (ambient + music catalogs) as SoundWaves.

    UnrealEditor-Cmd PoF.uproject -run=pythonscript -script=Content/Python/import_duel_audio.py -nullrhi

Source WAVs converted from the ElevenLabs mp3s (~/.pof/audio) under Import/Audio.
Targets /Game/Generated/Audio — consumed by the FeatureLab's ambient wiring.
"""
import unreal, os

PROJ = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
FILES = [
    (os.path.join(PROJ, "Import", "Audio", "GalacticArenaAmbience.wav"), "/Game/Generated/Audio"),
    (os.path.join(PROJ, "Import", "Audio", "GalacticDuelTheme.wav"), "/Game/Generated/Audio"),
]

tools = unreal.AssetToolsHelpers.get_asset_tools()
for path, dest in FILES:
    task = unreal.AssetImportTask()
    task.filename = path
    task.destination_path = dest
    task.automated = True
    task.replace_existing = True
    task.save = True
    tools.import_asset_tasks([task])
    unreal.log(f"[duel-audio] imported {os.path.basename(path)} -> {dest} objects={list(task.imported_object_paths)}")

unreal.log("[duel-audio] RESULT done")
