"""Headless Blender: build a combat-arena mesh and export Arena.fbx.
Run: blender --background --python build_arena.py
Authored in metres; the UE import applies a x100 scale (m -> cm)."""
import bpy, os, math

OUT = os.path.join(os.path.dirname(bpy.data.filepath) or os.path.dirname(__file__), "Arena.fbx")
ARENA = 20.0           # floor is 20 m square
WALL_H = 5.0           # wall height (m)
WALL_T = 0.5           # wall thickness (m)
TILE_METERS = 8.0      # world metres per texture repeat (world-aligned UVs)

# --- clean scene -----------------------------------------------------------
bpy.ops.wm.read_factory_settings(use_empty=True)

def new_material(name):
    m = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    return m

def add_box(name, size, location, material):
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=location)
    obj = bpy.context.active_object
    obj.name = name
    obj.scale = (size[0], size[1], size[2])
    bpy.ops.object.transform_apply(scale=True)
    obj.data.materials.append(material)
    return obj

mat_floor  = new_material("M_Floor")
mat_wall   = new_material("M_Wall")
mat_pillar = new_material("M_Pillar")

parts = []
# floor: 20x20x0.2, top at z=0
parts.append(add_box("Floor", (ARENA, ARENA, 0.2), (0, 0, -0.1), mat_floor))
# four perimeter walls
half = ARENA / 2.0
parts.append(add_box("Wall_N", (ARENA, WALL_T, WALL_H), (0,  half, WALL_H/2), mat_wall))
parts.append(add_box("Wall_S", (ARENA, WALL_T, WALL_H), (0, -half, WALL_H/2), mat_wall))
parts.append(add_box("Wall_E", (WALL_T, ARENA, WALL_H), ( half, 0, WALL_H/2), mat_wall))
parts.append(add_box("Wall_W", (WALL_T, ARENA, WALL_H), (-half, 0, WALL_H/2), mat_wall))
# four corner pillars
for i, (sx, sy) in enumerate([(1,1),(1,-1),(-1,1),(-1,-1)]):
    bpy.ops.mesh.primitive_cylinder_add(radius=0.6, depth=WALL_H,
        location=(sx*(half-1.5), sy*(half-1.5), WALL_H/2))
    p = bpy.context.active_object
    p.name = f"Pillar_{i}"
    p.data.materials.append(mat_pillar)
    parts.append(p)

# --- UV unwrap ------------------------------------------------------------
# World-aligned planar UVs: project each face's world position onto the plane
# perpendicular to its dominant world-normal axis, divided by TILE_METERS, so
# one texture repeat == TILE_METERS of world space everywhere. This replaces
# the per-face cube projection that produced a repeating grid. Pillars are
# curved, so they keep a smart unwrap.
def world_aligned_uv(obj, tile):
    mesh = obj.data
    if not mesh.uv_layers:
        mesh.uv_layers.new(name="UVMap")
    uv = mesh.uv_layers.active.data
    mw = obj.matrix_world
    rot = mw.to_3x3()
    for poly in mesh.polygons:
        n = rot @ poly.normal
        ax, ay, az = abs(n.x), abs(n.y), abs(n.z)
        for li in poly.loop_indices:
            co = mw @ mesh.vertices[mesh.loops[li].vertex_index].co
            if az >= ax and az >= ay:      # floor / ceiling -> world XY
                u, v = co.x, co.y
            elif ay >= ax:                  # N/S walls -> world XZ
                u, v = co.x, co.z
            else:                           # E/W walls -> world YZ
                u, v = co.y, co.z
            uv[li].uv = (u / tile, v / tile)

for obj in parts:
    if obj.name.startswith("Pillar"):
        bpy.context.view_layer.objects.active = obj
        bpy.ops.object.select_all(action='DESELECT')
        obj.select_set(True)
        bpy.ops.object.mode_set(mode='EDIT')
        bpy.ops.mesh.select_all(action='SELECT')
        bpy.ops.uv.smart_project(angle_limit=1.15, island_margin=0.02)
        bpy.ops.object.mode_set(mode='OBJECT')
    else:
        world_aligned_uv(obj, TILE_METERS)

# --- join into one object "Arena" -----------------------------------------
bpy.ops.object.select_all(action='DESELECT')
for obj in parts:
    obj.select_set(True)
bpy.context.view_layer.objects.active = parts[0]
bpy.ops.object.join()
arena = bpy.context.active_object
arena.name = "Arena"

# --- export FBX ------------------------------------------------------------
bpy.ops.export_scene.fbx(filepath=OUT, use_selection=True,
    apply_unit_scale=True, global_scale=1.0, object_types={'MESH'},
    mesh_smooth_type='FACE', path_mode='COPY')
assert os.path.isfile(OUT), "FBX export failed: " + OUT
print("ARENA_EXPORTED", OUT)
