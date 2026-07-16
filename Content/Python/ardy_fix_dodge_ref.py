"""BP_VSPlayer's DodgeMontage_* refs followed the rename to the backup — re-point
DodgeMontage_Forward (and Default if set) at the current AM_Dodge_Forward (ARDY v2)."""
import unreal
eal = unreal.EditorAssetLibrary
m = eal.load_asset("/Game/Characters/Player/Animations/Montages/AM_Dodge_Forward")
print(f"POF_DODGE_MONTAGE len={float(m.get_play_length()):.2f}")
bp = eal.load_asset("/Game/VerticalSlice/BP_VSPlayer")
print(f"POF_DODGE_BP {bool(bp)}")
cdo = unreal.get_default_object(bp.generated_class())
for prop in ["dodge_montage_forward", "dodge_montage_backward", "dodge_montage_left", "dodge_montage_right", "dodge_montage_default"]:
    try:
        cur = cdo.get_editor_property(prop)
        print(f"POF_DODGE {prop} was {cur.get_name() if cur else None}")
    except Exception as e:
        print(f"POF_DODGE {prop} ERR {e}")
for prop in ["dodge_montage_forward", "dodge_montage_backward", "dodge_montage_left", "dodge_montage_right", "dodge_montage_default"]:
    cdo.set_editor_property(prop, m)
eal.save_asset("/Game/VerticalSlice/BP_VSPlayer")
cdo2 = unreal.get_default_object(eal.load_asset("/Game/VerticalSlice/BP_VSPlayer").generated_class())
print(f"POF_DODGE now {cdo2.get_editor_property('dodge_montage_forward').get_name()}")
print("POF_DODGE_DONE")
