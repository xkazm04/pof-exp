"""
retexture_arena_ue.py
=====================
PS-3 / Task 2: Re-textures the PoF combat arena with the new Leonardo-generated
dungeon-stone albedos and corrects the tiling scale so the arena no longer reads
as a ~10x repeating industrial grid.

What it does (each section is idempotent and wrapped in try/except):
  1. Reimport the 3 new albedo PNGs from Content/ArenaBuild/textures_v2/ OVER the
     EXISTING PS-2 texture assets /Game/ArenaBuild/Textures/T_{floor,wall,pillar}
     _albedo (replace_existing = True). Because M_Arena_* already sample those
     asset paths, the albedo swap propagates automatically -- no material edit is
     needed just for the swap.
  2. Rebuild the 3 M_Arena_{Floor,Wall,Pillar} materials fresh (the sanctioned
     fallback over fragile MaterialEditingLibrary graph edits): new albedo +
     the EXISTING PS-2 T_*_normal / T_*_rough assets + a single shared
     TextureCoordinate node feeding the UV input of every TextureSample. The
     TextureCoordinate UTiling/VTiling are set to TILING (see below), ~0.3x of
     PS-2's implicit ~10x tiling -> roughly 3x across the 20 m arena.
  3. Save the textures and materials.

SM_Arena, the level and all gameplay assets are UNTOUCHED -- M_Arena_* are
already assigned to the mesh slots, and rebuilding a material asset in place
keeps that assignment (same object path).

Run headless:
    "C:\\Program Files\\Epic Games\\UE_5.7\\Engine\\Binaries\\Win64\\UnrealEditor-Cmd.exe" ^
        "C:\\Users\\kazda\\Documents\\Unreal Projects\\PoF\\PoF.uproject" ^
        -run=pythonscript -script="<abs path to this file>" -unattended -nopause
"""

import os
import unreal

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

PROJECT_DIR = unreal.Paths.project_dir()
ARENA_BUILD_DIR = os.path.normpath(os.path.join(PROJECT_DIR, "Content", "ArenaBuild"))
NEW_TEXTURES_DIR = os.path.join(ARENA_BUILD_DIR, "textures_v2")

# UE content paths
TEXTURES_FOLDER = "/Game/ArenaBuild/Textures"

# Texture surface key (file prefix) -> material asset path
SURFACE_TO_MATERIAL = {
    "floor": "/Game/ArenaBuild/M_Arena_Floor",
    "wall": "/Game/ArenaBuild/M_Arena_Wall",
    "pillar": "/Game/ArenaBuild/M_Arena_Pillar",
}

SURFACES = ["floor", "wall", "pillar"]

# Tiling correction.
#   PS-2 built the materials with no explicit TextureCoordinate, so the
#   arena's cube-projection UVs tiled the textures ~10x across the 20 m space
#   (the "industrial repeating grid" look PS-3 fixes).
#   A TextureCoordinate UTiling/VTiling of N multiplies the incoming UVs by N.
#   To go from an effective ~10x repeat down to ~3x we want ~0.3x of PS-2's
#   tiling -> UTiling = VTiling = 3.0 yields roughly 3 repeats over the 20 m
#   arena (a ~6-7 m stone span per repeat), which reads as natural masonry
#   rather than a grid.
TILING = 3.0

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
asset_lib = unreal.EditorAssetLibrary
mat_lib = unreal.MaterialEditingLibrary


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _log(msg):
    unreal.log("[retexture_arena_ue] " + msg)


def split_path(package_path):
    """'/Game/Foo/Bar' -> ('/Game/Foo', 'Bar')"""
    idx = package_path.rfind("/")
    return package_path[:idx], package_path[idx + 1:]


def load_object(object_path):
    if not asset_lib.does_asset_exist(object_path):
        return None
    return asset_lib.load_asset(object_path)


# ---------------------------------------------------------------------------
# Section 1: Reimport the new albedo PNGs over the existing texture assets
# ---------------------------------------------------------------------------

def reimport_albedos():
    """Import textures_v2/{surface}_albedo.png OVER /Game/.../T_{surface}_albedo."""
    try:
        if not os.path.isdir(NEW_TEXTURES_DIR):
            raise RuntimeError("new textures dir not found: " + NEW_TEXTURES_DIR)

        tasks = []
        keys = []
        for surface in SURFACES:
            fname = surface + "_albedo.png"
            src = os.path.join(NEW_TEXTURES_DIR, fname)
            if not os.path.isfile(src):
                raise RuntimeError("new albedo PNG not found: " + src)
            key = surface + "_albedo"
            keys.append(key)

            task = unreal.AssetImportTask()
            task.set_editor_property("filename", src)
            task.set_editor_property("destination_path", TEXTURES_FOLDER)
            # Same destination name as PS-2 -> imports OVER the existing asset.
            task.set_editor_property("destination_name", "T_" + key)
            task.set_editor_property("replace_existing", True)
            task.set_editor_property("automated", True)
            task.set_editor_property("save", True)
            tasks.append(task)

        asset_tools.import_asset_tasks(tasks)

        imported = {}
        for key in keys:
            asset_path = "%s/T_%s" % (TEXTURES_FOLDER, key)
            tex = load_object(asset_path)
            if tex is None:
                raise RuntimeError("albedo not found after reimport: " + asset_path)
            # Albedo is color data -> sRGB on (matches PS-2 default for albedo).
            tex.set_editor_property("srgb", True)
            asset_lib.save_asset(asset_path)
            imported[key] = tex
            _log("Reimported albedo: " + asset_path)
        return imported
    except Exception as exc:
        unreal.log_error("[retexture_arena_ue] reimport_albedos FAILED: " + str(exc))
        raise


# ---------------------------------------------------------------------------
# Section 2: Rebuild the three materials with corrected tiling
# ---------------------------------------------------------------------------

def build_material(material_path, surface):
    """Rebuild one material: new albedo + existing normal/rough + TextureCoordinate.

    Rebuilding in place (delete + create_asset at the same object path) keeps the
    material's assignment to SM_Arena's slot intact -- the slot stores the asset
    path, which is unchanged.
    """
    folder, name = split_path(material_path)

    albedo = load_object("%s/T_%s_albedo" % (TEXTURES_FOLDER, surface))
    normal = load_object("%s/T_%s_normal" % (TEXTURES_FOLDER, surface))
    rough = load_object("%s/T_%s_rough" % (TEXTURES_FOLDER, surface))

    if albedo is None:
        raise RuntimeError("albedo asset missing for surface " + surface)
    # normal / rough are kept from PS-2; warn (not fatal) if absent.
    if normal is None:
        unreal.log_warning("[retexture_arena_ue] normal missing for " + surface)
    if rough is None:
        unreal.log_warning("[retexture_arena_ue] rough missing for " + surface)

    if asset_lib.does_asset_exist(material_path):
        # Idempotent re-run: delete and rebuild so the graph is clean.
        asset_lib.delete_asset(material_path)

    factory = unreal.MaterialFactoryNew()
    material = asset_tools.create_asset(name, folder, unreal.Material, factory)
    if material is None:
        raise RuntimeError("failed to create material " + material_path)

    # --- Shared TextureCoordinate node for the corrected tiling --------------
    tex_coord = mat_lib.create_material_expression(
        material, unreal.MaterialExpressionTextureCoordinate, -800, 0)
    tex_coord.set_editor_property("u_tiling", TILING)
    tex_coord.set_editor_property("v_tiling", TILING)

    # --- Albedo -> Base Color -------------------------------------------------
    albedo_node = mat_lib.create_material_expression(
        material, unreal.MaterialExpressionTextureSample, -400, -200)
    albedo_node.set_editor_property("texture", albedo)
    # Feed the TextureCoordinate into the sampler's UVs input.
    mat_lib.connect_material_expressions(tex_coord, "", albedo_node, "UVs")
    mat_lib.connect_material_property(
        albedo_node, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)

    # --- Normal -> Normal -----------------------------------------------------
    if normal is not None:
        normal_node = mat_lib.create_material_expression(
            material, unreal.MaterialExpressionTextureSample, -400, 50)
        normal_node.set_editor_property("texture", normal)
        normal_node.set_editor_property(
            "sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
        mat_lib.connect_material_expressions(tex_coord, "", normal_node, "UVs")
        mat_lib.connect_material_property(
            normal_node, "RGB", unreal.MaterialProperty.MP_NORMAL)

    # --- Roughness -> Roughness ----------------------------------------------
    if rough is not None:
        rough_node = mat_lib.create_material_expression(
            material, unreal.MaterialExpressionTextureSample, -400, 300)
        rough_node.set_editor_property("texture", rough)
        rough_node.set_editor_property(
            "sampler_type",
            unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_GRAYSCALE)
        # Roughness uses the same corrected tiling for consistency.
        mat_lib.connect_material_expressions(tex_coord, "", rough_node, "UVs")
        mat_lib.connect_material_property(
            rough_node, "R", unreal.MaterialProperty.MP_ROUGHNESS)

    mat_lib.recompile_material(material)
    asset_lib.save_asset(material_path)
    _log("Rebuilt material (tiling %.2f): %s" % (TILING, material_path))
    return material


def rebuild_materials():
    try:
        for surface in SURFACES:
            build_material(SURFACE_TO_MATERIAL[surface], surface)
    except Exception as exc:
        unreal.log_error("[retexture_arena_ue] rebuild_materials FAILED: " + str(exc))
        raise


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    _log("=== Arena re-texture START ===")
    reimport_albedos()
    rebuild_materials()
    _log("=== Arena re-texture COMPLETE ===")


if __name__ == "__main__":
    main()
    # When launched through the full editor (-ExecutePythonScript), there is no
    # commandlet to end the process. Quit the editor so the headless run exits.
    # -run=pythonscript ignores this (no editor loop) and exits on its own.
    try:
        if unreal.is_editor():
            unreal.SystemLibrary.quit_editor()
    except Exception:
        pass
