"""Fully-owned, code-authored animation engine for the UE5 Mannequin.

No Mixamo clip drives the motion: we read a neutral standing base pose once, then author
custom per-bone rotation keyframes through IAnimationDataController. A 'pose' is a dict of
bone-name -> (rx, ry, rz) LOCAL euler offset (degrees) applied on top of the base. An
'animation' is a list of keyposes with hold/ease timing, interpolated into bone tracks.

This is the test of the world's-hard problem: can a machine author a believable combat
swing in code, reviewed by a rendered frame? Engine here; swings authored on top.
"""
import unreal

IDLE_BASE = "/Game/Mixamo/Retargeted/SKM_Manny/Standard_Idle_RT"  # neutral STANCE only (motion is ours)


def _asset_tools():
    return unreal.AssetToolsHelpers.get_asset_tools()


def euler_quat(rx, ry, rz):
    """Local single/multi-axis euler (deg) -> Quat. UE Rotator: roll=X, pitch=Y, yaw=Z."""
    return unreal.Rotator(roll=rx, pitch=ry, yaw=rz).quaternion()


def load_base_pose(idle_path=IDLE_BASE):
    """Return (skeleton, [bone_names], {bone: (pos Vector, rot Quat, scale Vector)}) from frame 0."""
    idle = unreal.load_asset(idle_path)
    if not idle:
        raise RuntimeError("base idle missing: %s" % idle_path)
    skel = idle.get_skeleton()

    # Data model is exposed as an attribute (data_model / data_model_interface), not a get_*().
    model = None
    for acc in ("data_model", "data_model_interface"):
        try:
            m = getattr(idle, acc)
        except Exception as e:
            unreal.log("POSE: attr .%s raised %s" % (acc, e)); m = None
        if m is not None and hasattr(m, "get_bone_track_names"):
            model = m
            unreal.log("POSE: data model via attr .%s -> %s" % (acc, type(m).__name__))
            break
    if model is None:
        # last resort: method forms in case this build exposes them
        for meth in ("get_data_model", "get_data_model_interface"):
            if hasattr(idle, meth):
                try:
                    m = getattr(idle, meth)()
                    if m is not None and hasattr(m, "get_bone_track_names"):
                        model = m
                        unreal.log("POSE: data model via %s()" % meth)
                        break
                except Exception as e:
                    unreal.log("POSE: %s() raised %s" % (meth, e))
    if model is None:
        raise RuntimeError("could not acquire AnimDataModel from %s" % idle_path)
    raw = model.get_bone_track_names()
    names = [str(n) for n in raw]
    # Retargeted clips don't expose raw tracks; evaluate the local pose at frame 0 instead.
    base = {}
    for n in raw:
        bn = str(n)
        try:
            t = unreal.AnimationLibrary.get_bone_pose_for_frame(idle, n, 0, False)
            base[bn] = (t.translation, t.rotation, t.scale3d)
        except Exception as e:
            unreal.log_warning("POSE: base read %s failed (%s)" % (bn, e))
    unreal.log("POSE: base pose read — %d / %d bones" % (len(base), len(names)))
    return skel, names, base


def _get_controller(anim):
    # 5.8: the controller is the `controller` attribute; there is no get_controller() method,
    # and get_editor_property("controller") returns a non-writing handle.
    return anim.controller


def author_anim(name, dest, skel, base, num_frames, fps, offset_fn):
    """Create an AnimSequence; every bone holds its base pose, plus offset_fn(frame)->{bone:(rx,ry,rz)}.

    offset_fn returns local euler offsets (deg) per bone for the given frame index (0..num_frames).
    """
    full = "%s/%s" % (dest, name)
    if unreal.EditorAssetLibrary.does_asset_exist(full):
        unreal.EditorAssetLibrary.delete_asset(full)

    factory = unreal.AnimSequenceFactory()
    factory.set_editor_property("target_skeleton", skel)
    anim = _asset_tools().create_asset(name, dest, unreal.AnimSequence, factory)
    if not anim:
        raise RuntimeError("create_asset failed for %s" % full)

    nkeys = num_frames + 1
    ctrl = _get_controller(anim)
    ctrl.open_bracket("author %s" % name, True)
    try:
        ctrl.set_frame_rate(unreal.FrameRate(fps, 1))
        ctrl.set_number_of_frames(unreal.FrameNumber(num_frames))
        for bone, (bpos, brot, bscale) in base.items():
            pos_keys, rot_keys, scale_keys = [], [], []
            for k in range(nkeys):
                off = offset_fn(k).get(bone)
                if off and (off[0] or off[1] or off[2]):
                    r = brot * euler_quat(off[0], off[1], off[2])  # local-space compose
                else:
                    r = brot
                pos_keys.append(unreal.Vector(bpos.x, bpos.y, bpos.z))
                rot_keys.append(r)
                scale_keys.append(unreal.Vector(bscale.x, bscale.y, bscale.z))
            # The track MUST exist before keys can be written (set_bone_track_keys returns
            # False otherwise — this build does not auto-create the track).
            ctrl.add_bone_track(bone)
            if not ctrl.set_bone_track_keys(bone, pos_keys, rot_keys, scale_keys, True):
                unreal.log_warning("POSE: set_bone_track_keys FAILED for %s" % bone)
    finally:
        ctrl.close_bracket(True)

    unreal.EditorAssetLibrary.save_asset(full)
    unreal.log("POSE: authored %s (%d frames @ %dfps)" % (full, num_frames, fps))
    return full


def author_abs(name, dest, skel, base, num_frames, fps, abs_fn):
    """Like author_anim, but abs_fn(frame) -> {bone: Quat ABSOLUTE local rotation}. Bones not
    returned hold their idle base rotation. Used for component-space authoring (caller converts
    component-space intent to absolute local quats)."""
    full = "%s/%s" % (dest, name)
    if unreal.EditorAssetLibrary.does_asset_exist(full):
        unreal.EditorAssetLibrary.delete_asset(full)
    factory = unreal.AnimSequenceFactory()
    factory.set_editor_property("target_skeleton", skel)
    anim = _asset_tools().create_asset(name, dest, unreal.AnimSequence, factory)
    nkeys = num_frames + 1
    ctrl = _get_controller(anim)
    ctrl.open_bracket("author %s" % name, True)
    try:
        ctrl.set_frame_rate(unreal.FrameRate(fps, 1))
        ctrl.set_number_of_frames(unreal.FrameNumber(num_frames))
        for bone, (bpos, brot, bscale) in base.items():
            pos_keys, rot_keys, scale_keys = [], [], []
            for k in range(nkeys):
                r = abs_fn(k).get(bone, brot)
                pos_keys.append(unreal.Vector(bpos.x, bpos.y, bpos.z))
                rot_keys.append(r)
                scale_keys.append(unreal.Vector(bscale.x, bscale.y, bscale.z))
            ctrl.add_bone_track(bone)
            if not ctrl.set_bone_track_keys(bone, pos_keys, rot_keys, scale_keys, True):
                unreal.log_warning("POSE: set_bone_track_keys FAILED for %s" % bone)
    finally:
        ctrl.close_bracket(True)
    unreal.EditorAssetLibrary.save_asset(full)
    unreal.log("POSE: authored(abs) %s (%d frames @ %dfps)" % (full, num_frames, fps))
    return full


def _ease(a):
    """smoothstep ease-in/out for natural acceleration."""
    a = 0.0 if a < 0.0 else (1.0 if a > 1.0 else a)
    return a * a * (3.0 - 2.0 * a)


def author_keyposes(name, dest, skel, base, fps, keyposes, ease=True):
    """keyposes = [(time_sec, {bone: (rx, ry, rz) local euler offset}), ...] sorted by time.

    Interpolates (smoothstep) between keyposes to build a full swing AnimSequence.
    """
    bones = set()
    for _, p in keyposes:
        bones.update(p.keys())
    total_t = keyposes[-1][0]
    num_frames = int(round(total_t * fps))

    def offset_fn(k):
        t = k / float(fps)
        seg = 0
        for i in range(len(keyposes) - 1):
            if t >= keyposes[i][0]:
                seg = i
        t0, p0 = keyposes[seg]
        t1, p1 = keyposes[min(seg + 1, len(keyposes) - 1)]
        a = (t - t0) / max(t1 - t0, 1e-5)
        a = _ease(a) if ease else (0.0 if a < 0 else (1.0 if a > 1 else a))
        out = {}
        for b in bones:
            v0 = p0.get(b, (0.0, 0.0, 0.0))
            v1 = p1.get(b, (0.0, 0.0, 0.0))
            out[b] = (v0[0] + (v1[0] - v0[0]) * a,
                      v0[1] + (v1[1] - v0[1]) * a,
                      v0[2] + (v1[2] - v0[2]) * a)
        return out

    return author_anim(name, dest, skel, base, num_frames, fps, offset_fn)

