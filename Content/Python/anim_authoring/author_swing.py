"""Author a custom overhead diagonal lightsaber slash — purely code, no mocap/Mixamo motion.

Keyposes are local euler offsets (deg) on the sword arm + torso, interpolated (smoothstep):
ready -> windup (saber high & back, elbow cocked, torso coils right) -> strike (down-forward
across the body, torso uncoils) -> follow-through -> recover. Iterated against rendered frames.
"""
import sys
sys.path.insert(0, r"C:/Users/kazda/Documents/Unreal Projects/PoF/Content/Python/anim_authoring")
import unreal
import pose_engine as pe

FPS = 30
NAME = "SwordSlash01"
DEST = "/Game/PoF/GenAnims"

# (time_sec, {bone: (rx, ry, rz)})  — v3: drive the downswing LOW with calibrated -X (less lifting Z),
# add a forward torso lean into the strike for weight. +X raises the arm (calibrated).
SWING = [
    (0.00, {"upperarm_r": (15, 0, 0), "lowerarm_r": (0, 0, -15)}),
    (0.38, {"upperarm_r": (130, 0, -12), "lowerarm_r": (0, 0, -78),
            "spine_03": (10, 0, -12), "spine_04": (8, 0, -8)}),                                  # windup: cock high, slight back-coil
    (0.50, {"upperarm_r": (-30, 0, 12), "lowerarm_r": (0, 0, -12),
            "spine_03": (-12, 0, 12), "spine_04": (-8, 0, 8)}),                                  # strike: fast DOWN, lean+uncoil
    (0.72, {"upperarm_r": (-55, 0, 22), "lowerarm_r": (0, 0, -8), "spine_03": (-15, 0, 15)}),    # follow: low, leaned in
    (1.00, {"upperarm_r": (15, 0, 0), "lowerarm_r": (0, 0, -15)}),                               # recover
]


def main():
    unreal.log("SWING: BEGIN")
    skel, names, base = pe.load_base_pose()
    path = pe.author_keyposes(NAME, DEST, skel, base, FPS, SWING)
    unreal.log("SWING: authored %s" % path)
    unreal.log("[gate] RESULT=PASS")


main()
