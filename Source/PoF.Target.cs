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
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
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
