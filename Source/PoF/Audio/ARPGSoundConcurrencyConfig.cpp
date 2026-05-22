#include "Audio/ARPGSoundConcurrencyConfig.h"

const FARPGConcurrencyPreset& UARPGSoundConcurrencyConfig::GetPreset(FName PresetName) const
{
	if (PresetName == WeaponConcurrency.PresetName) return WeaponConcurrency;
	if (PresetName == FootstepConcurrency.PresetName) return FootstepConcurrency;
	if (PresetName == AmbientConcurrency.PresetName) return AmbientConcurrency;
	if (PresetName == UIConcurrency.PresetName) return UIConcurrency;
	if (PresetName == AbilityConcurrency.PresetName) return AbilityConcurrency;
	if (PresetName == ImpactConcurrency.PresetName) return ImpactConcurrency;

	// Return weapon as default fallback
	return WeaponConcurrency;
}
