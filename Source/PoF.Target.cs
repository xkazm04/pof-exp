using UnrealBuildTool;
using System.Collections.Generic;

public class PoFTarget : TargetRules
{
	public PoFTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		// This target customizes GlobalDefinitions below. A unique build
		// environment is not possible with an installed engine, so force the
		// modified settings onto the shared environment instead.
		bOverrideBuildEnvironment = true;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		// Pinned during the 5.7->5.8 upgrade: 5.8's Latest (Unreal5_8) tightens
		// IWYU include-order enforcement. Hold at Unreal5_7 to preserve the
		// behavior the code already builds against; do IWYU cleanup separately.
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
		ExtraModuleNames.AddRange(new string[] { "PoF" });

		// Shipping hardening
		if (Configuration == UnrealTargetConfiguration.Shipping)
		{
			bUseLoggingInShipping = false;
			GlobalDefinitions.Add("UE_BUILD_SHIPPING_WITH_EDITOR=0");
			GlobalDefinitions.Add("POF_SHIPPING=1");
		}

		// Development convenience
		if (Configuration == UnrealTargetConfiguration.Development)
		{
			GlobalDefinitions.Add("POF_SHIPPING=0");
			GlobalDefinitions.Add("POF_DEV_TOOLS=1");
		}
	}
}
