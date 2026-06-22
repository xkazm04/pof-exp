#include "AbilitySystem/ARPGGameplayTags.h"

namespace ARPGGameplayTags
{
	// --- Ability ---
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Melee_LightAttack, "Ability.Melee.LightAttack", "Fast melee combo attack");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Melee_HeavyAttack, "Ability.Melee.HeavyAttack", "Slow high-damage melee attack");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Parry,            "Ability.Parry",              "Saber parry/block — deflects an incoming melee attack");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Ranged_Fireball,   "Ability.Ranged.Fireball",   "Ranged fireball projectile");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Dodge,             "Ability.Dodge",              "Dodge roll ability");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Charge,           "Ability.Charge",             "Brute charge attack ability");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Fireball,         "Ability.Fireball",           "Player fireball projectile ability");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_GroundSlam,       "Ability.GroundSlam",         "Player ground slam AoE ability");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_DashStrike,       "Ability.DashStrike",         "Player dash strike ability");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_WarCry,           "Ability.WarCry",             "Player war cry self-buff ability");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_ArcaneBeam,       "Ability.ArcaneBeam",         "Player channeled arcane beam ability");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_ForcePush,        "Ability.ForcePush",          "Force Push — telekinetic directional knockback + light damage (bound to slot 1)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_UsePotion,        "Ability.UsePotion",          "Player potion consumption ability");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_HitReact,         "Ability.HitReact",           "Hit reaction ability triggered by incoming damage");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Enemy_Melee,      "Ability.Enemy.Melee",        "Enemy melee attack ability");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Enemy_Ranged,     "Ability.Enemy.Ranged",       "Enemy ranged attack ability");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Enemy_Charge,     "Ability.Enemy.Charge",       "Enemy charge attack ability");

	// --- State ---
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Dead,         "State.Dead",         "Character is dead");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Stunned,      "State.Stunned",      "Character is stunned and cannot act");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Invulnerable, "State.Invulnerable", "Character is invulnerable to damage");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Attacking,    "State.Attacking",    "Character is performing an attack");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Charging,     "State.Charging",     "Enemy is mid-charge attack");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Vulnerable,   "State.Vulnerable",   "Enemy is in post-charge vulnerability window");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Buffed_WarCry,"State.Buffed.WarCry","Player has active War Cry buff");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Dashing,      "State.Dashing",      "Player is mid-dash-strike");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Burning,      "State.Burning",      "On fire — taking fire damage over time (Burning status effect)");

	// --- Damage ---
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_Physical,  "Damage.Physical",  "Physical damage type");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_Fire,      "Damage.Fire",      "Fire damage type");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_Ice,       "Damage.Ice",       "Ice damage type");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_Lightning, "Damage.Lightning", "Lightning damage type");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_Poison,   "Damage.Poison",   "Poison damage type");

	// --- Data (SetByCaller keys for damage) ---
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Damage_Base,    "Data.Damage.Base",    "SetByCaller: base damage value for execution calc");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Damage_Scaling, "Data.Damage.Scaling", "SetByCaller: AttackPower scaling factor (default 1.0)");

	// --- Data (SetByCaller key for difficulty scaling) ---
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_DifficultyMultiplier, "Data.DifficultyMultiplier", "SetByCaller: multiplier for enemy difficulty scaling (1.0 = no change)");

	// --- Data (SetByCaller keys for attribute init) ---
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Init_Health,         "Data.Init.Health",         "Init: Health base value");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Init_MaxHealth,      "Data.Init.MaxHealth",      "Init: MaxHealth base value");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Init_Mana,           "Data.Init.Mana",           "Init: Mana base value");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Init_MaxMana,        "Data.Init.MaxMana",        "Init: MaxMana base value");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Init_Strength,       "Data.Init.Strength",       "Init: Strength base value");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Init_Dexterity,      "Data.Init.Dexterity",      "Init: Dexterity base value");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Init_Intelligence,   "Data.Init.Intelligence",   "Init: Intelligence base value");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Init_Armor,          "Data.Init.Armor",          "Init: Armor base value");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Init_AttackPower,    "Data.Init.AttackPower",    "Init: AttackPower base value");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Init_CriticalChance, "Data.Init.CriticalChance", "Init: CriticalChance base value");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Init_CriticalDamage, "Data.Init.CriticalDamage", "Init: CriticalDamage base value");

	// --- Data (SetByCaller keys for resistance init) ---
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Init_FireResistance,      "Data.Init.FireResistance",      "Init: Fire resistance (0-0.9)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Init_IceResistance,       "Data.Init.IceResistance",       "Init: Ice resistance (0-0.9)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Init_LightningResistance, "Data.Init.LightningResistance", "Init: Lightning resistance (0-0.9)");

	// --- Data (SetByCaller keys for progression init) ---
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Init_CurrentXP,      "Data.Init.CurrentXP",      "Init: CurrentXP base value");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Init_XPToNextLevel,  "Data.Init.XPToNextLevel",  "Init: XPToNextLevel from XP curve");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Init_CharacterLevel, "Data.Init.CharacterLevel", "Init: CharacterLevel base value");

	// --- Data (SetByCaller key for XP award) ---
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_XP_Amount, "Data.XP.Amount", "SetByCaller: XP amount to award to killer");

	// --- Data (SetByCaller key for mana cost) ---
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_ManaCost, "Data.ManaCost", "SetByCaller: mana cost amount for ability cost GE");

	// --- Event ---
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_EnemyKilled, "Event.EnemyKilled", "Sent to killer ASC when an enemy dies");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Combo_Open,  "Event.Combo.Open",  "Sent by AnimNotify when combo window opens");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Combo_Close, "Event.Combo.Close", "Sent by AnimNotify when combo window closes");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Combo_Input, "Event.Combo.Input", "Sent by controller when player presses attack during combo");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_MeleeHit,   "Event.MeleeHit",   "Sent by hit detection notify when weapon hits a target");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_HitReact,  "Event.HitReact",  "Sent to target ASC after taking non-lethal damage");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Death,     "Event.Death",     "Sent to target ASC when health reaches zero");

	// --- Cooldown ---
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Potion,     "Cooldown.Potion",     "Cooldown tag applied while potion use is on cooldown");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Fireball,   "Cooldown.Fireball",   "Cooldown tag for Fireball ability");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_GroundSlam, "Cooldown.GroundSlam", "Cooldown tag for Ground Slam ability");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_DashStrike, "Cooldown.DashStrike", "Cooldown tag for Dash Strike ability");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_WarCry,     "Cooldown.WarCry",     "Cooldown tag for War Cry ability");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_ArcaneBeam,"Cooldown.ArcaneBeam", "Cooldown tag for Arcane Beam ability");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_ForcePush, "Cooldown.ForcePush",  "Cooldown tag for Force Push ability");

	// --- Data (SetByCaller key for affix magnitude) ---
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Affix_Magnitude, "Data.Affix.Magnitude", "SetByCaller: affix magnitude value applied by equip GE");

	// --- Item ---
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Weapon,              "Item.Weapon",              "Base weapon item tag");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Weapon_Sword,        "Item.Weapon.Sword",        "Sword weapon type");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Weapon_Staff,        "Item.Weapon.Staff",        "Staff weapon type");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Weapon_Bow,          "Item.Weapon.Bow",          "Bow weapon type");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Armor,               "Item.Armor",               "Base armor item tag");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Armor_Head,          "Item.Armor.Head",          "Head armor slot");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Armor_Chest,         "Item.Armor.Chest",         "Chest armor slot");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Armor_Legs,          "Item.Armor.Legs",          "Legs armor slot");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Armor_Feet,          "Item.Armor.Feet",          "Feet armor slot");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Accessory,           "Item.Accessory",           "Base accessory item tag");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Accessory_Ring,      "Item.Accessory.Ring",      "Ring accessory slot");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Accessory_Amulet,    "Item.Accessory.Amulet",    "Amulet accessory slot");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Consumable,          "Item.Consumable",          "Base consumable item tag");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Consumable_Potion,   "Item.Consumable.Potion",   "Potion consumable type");

	// --- Input ---
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_PrimaryAttack, "InputTag.PrimaryAttack", "Input binding for primary attack");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Dodge,         "InputTag.Dodge",         "Input binding for dodge");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Sprint,        "InputTag.Sprint",        "Input binding for sprint");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Interact,      "InputTag.Interact",      "Input binding for interact");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Ability1,      "InputTag.Ability1",      "Input binding for ability slot 1");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Ability2,      "InputTag.Ability2",      "Input binding for ability slot 2");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Ability3,      "InputTag.Ability3",      "Input binding for ability slot 3");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Ability4,      "InputTag.Ability4",      "Input binding for ability slot 4");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_UsePotion,     "InputTag.UsePotion",     "Input binding for using a potion");

	// --- Cooldown (Dodge) ---
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Dodge, "Cooldown.Dodge", "Cooldown tag for Dodge ability");

	// --- GameplayCue (visual/audio feedback) ---
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_HitImpact,           "GameplayCue.HitImpact",           "Physical hit impact VFX/SFX");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_HitImpact_Fire,      "GameplayCue.HitImpact.Fire",      "Fire hit impact VFX/SFX");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_HitImpact_Ice,       "GameplayCue.HitImpact.Ice",       "Ice hit impact VFX/SFX");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_HitImpact_Lightning, "GameplayCue.HitImpact.Lightning", "Lightning hit impact VFX/SFX");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Death,               "GameplayCue.Death",               "Death VFX/SFX cue");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Heal,                "GameplayCue.Heal",                "Heal VFX/SFX cue");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Buff_WarCry,         "GameplayCue.Buff.WarCry",         "War Cry buff activation VFX/SFX");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Stun,                "GameplayCue.Stun",                "Stun state VFX cue (looping)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Dodge,               "GameplayCue.Dodge",               "Dodge roll VFX/SFX cue");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_LevelUp,             "GameplayCue.LevelUp",             "Level-up celebration VFX/SFX");
}
