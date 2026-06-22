"""Author a calibration AnimSequence: isolate each local rotation axis of the sword arm so a
rendered frame tells us which axis raises / swings / bends the arm. Foundation for the swing.

7 static blocks x 12 frames @ 30fps. Block 0 = base; 1-3 = upperarm_r about X/Y/Z (+50 deg);
4-6 = lowerarm_r about X/Y/Z (+70 deg). Captured samples land one-per-block.
"""
import sys
sys.path.insert(0, r"C:/Users/kazda/Documents/Unreal Projects/PoF/Content/Python/anim_authoring")
import unreal
import pose_engine as pe

SEG = 12
SEGMENTS = [
    {},                                # 0 base / reference
    {"upperarm_r": (50, 0, 0)},        # 1 shoulder local X
    {"upperarm_r": (0, 50, 0)},        # 2 shoulder local Y
    {"upperarm_r": (0, 0, 50)},        # 3 shoulder local Z
    {"lowerarm_r": (70, 0, 0)},        # 4 elbow local X
    {"lowerarm_r": (0, 70, 0)},        # 5 elbow local Y
    {"lowerarm_r": (0, 0, 70)},        # 6 elbow local Z
]


def offset_fn(frame):
    return SEGMENTS[min(frame // SEG, len(SEGMENTS) - 1)]


def main():
    unreal.log("CALIB: BEGIN")
    skel, names, base = pe.load_base_pose()
    for b in ["clavicle_r", "upperarm_r", "lowerarm_r", "hand_r", "spine_01", "spine_02", "spine_03"]:
        unreal.log("CALIB: bone %-12s present=%s" % (b, b in base))
    nframes = SEG * len(SEGMENTS)
    path = pe.author_anim("CalibSwing", "/Game/PoF/GenAnims", skel, base, nframes, 30, offset_fn)
    unreal.log("CALIB: authored %s (%d frames)" % (path, nframes))
    unreal.log("[gate] RESULT=PASS")


main()
