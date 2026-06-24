using UnrealBuildTool;
using System.Collections.Generic;

public class PoFEditorTarget : TargetRules
{
	public PoFEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		// Pinned during the 5.7->5.8 upgrade (see PoF.Target.cs): hold IWYU
		// include-order at Unreal5_7 to preserve known-good build behavior.
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
		ExtraModuleNames.AddRange(new string[] { "PoF", "PoFEditor" });
	}
}
