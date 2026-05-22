using UnrealBuildTool;
using System.Collections.Generic;

public class PoFEditorTarget : TargetRules
{
	public PoFEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.AddRange(new string[] { "PoF", "PoFEditor" });
	}
}
