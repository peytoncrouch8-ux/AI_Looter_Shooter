#pragma once

#include "CoreMinimal.h"

/**
 * Every sound cue's name, which LooterSound plays. Art/Sounds/cues.json gives each cue its sounds and its mix (volume,
 * pitch jitter, sound class, attenuation, loop), and Tools/Unreal/build_sound_bank.py puts them in the sound bank. A
 * cue's files are named for it with its dots as underscores: Weapon.Rifle.Fire -> Weapon_Rifle_Fire_01.wav. A cue with no
 * sounds yet plays nothing, so code can call it before its sound is made. Add a cue here and in cues.json together.
 */
namespace LooterSoundCue
{
	// --- Guns (AWeaponBase, its reload; UBulletSubsystem's impacts) ---
	inline constexpr const TCHAR* RifleFire = TEXT("Weapon.Rifle.Fire");
	inline constexpr const TCHAR* ShotgunFire = TEXT("Weapon.Shotgun.Fire");
	/** The trigger on an empty magazine. */
	inline constexpr const TCHAR* DryFire = TEXT("Weapon.DryFire");
	/** A cursed iron's misfire (Unlucky): the hammer falls, a dull pop and a fizzle, no bullet. */
	inline constexpr const TCHAR* Misfire = TEXT("Weapon.Misfire");
	inline constexpr const TCHAR* RifleMagOut = TEXT("Weapon.Rifle.MagOut");
	inline constexpr const TCHAR* RifleMagIn = TEXT("Weapon.Rifle.MagIn");
	/** The charging handle at a reload's end. */
	inline constexpr const TCHAR* RifleBolt = TEXT("Weapon.Rifle.Bolt");
	inline constexpr const TCHAR* ShotgunShellIn = TEXT("Weapon.Shotgun.ShellIn");
	inline constexpr const TCHAR* ShotgunPump = TEXT("Weapon.Shotgun.Pump");
	/** A gun taken in hand. */
	inline constexpr const TCHAR* Equip = TEXT("Weapon.Equip");
	/** Raising the sights (right mouse). */
	inline constexpr const TCHAR* AimIn = TEXT("Weapon.AimIn");
	/** A bullet meeting the world: ground and stone; wood; metal; water. */
	inline constexpr const TCHAR* ImpactWorld = TEXT("Impact.World");
	inline constexpr const TCHAR* ImpactWood = TEXT("Impact.Wood");
	inline constexpr const TCHAR* ImpactMetal = TEXT("Impact.Metal");
	inline constexpr const TCHAR* ImpactWater = TEXT("Impact.Water");

	// --- The player's own feedback (2D) ---
	/** Every hit on something that can be hurt; a critical hit plays it pitched up (LooterSoundRules::CritPitch). */
	inline constexpr const TCHAR* HitMarker = TEXT("UI.HitMarker");
	inline constexpr const TCHAR* Kill = TEXT("UI.Kill");
	inline constexpr const TCHAR* LevelUp = TEXT("UI.LevelUp");
	inline constexpr const TCHAR* MissionStep = TEXT("UI.MissionStep");
	inline constexpr const TCHAR* MissionComplete = TEXT("UI.MissionComplete");
	/** A gun's notch milestone (Blooded, Named, Soul-forged). */
	inline constexpr const TCHAR* NotchMilestone = TEXT("UI.NotchMilestone");
	inline constexpr const TCHAR* CurseLifted = TEXT("UI.CurseLifted");

	// --- Menus and screens (2D) ---
	inline constexpr const TCHAR* Click = TEXT("UI.Click");
	inline constexpr const TCHAR* Hover = TEXT("UI.Hover");
	inline constexpr const TCHAR* Back = TEXT("UI.Back");
	/** A screen or page opening (inventory, station board, bench), and closing. */
	inline constexpr const TCHAR* Open = TEXT("UI.Open");
	inline constexpr const TCHAR* Close = TEXT("UI.Close");
	inline constexpr const TCHAR* Tab = TEXT("UI.Tab");
	/** Something the player can't do now (a full backpack, a part that doesn't fit). */
	inline constexpr const TCHAR* Denied = TEXT("UI.Denied");

	// --- The gunsmith's bench ---
	inline constexpr const TCHAR* BenchScrap = TEXT("Bench.Scrap");
	inline constexpr const TCHAR* BenchFit = TEXT("Bench.Fit");

	// --- Loot ---
	inline constexpr const TCHAR* AmmoPickup = TEXT("Loot.AmmoPickup");
	inline constexpr const TCHAR* GunPickup = TEXT("Loot.GunPickup");
	/** Loot landing on the ground after a toss. */
	inline constexpr const TCHAR* LootLand = TEXT("Loot.Land");
	inline constexpr const TCHAR* ChestOpen = TEXT("Loot.ChestOpen");
	/** The Strongbox's vault wheel spinning before its lid lifts. */
	inline constexpr const TCHAR* StrongboxWheel = TEXT("Loot.StrongboxWheel");

	// --- The player ---
	inline constexpr const TCHAR* FootstepDirt = TEXT("Player.Footstep.Dirt");
	inline constexpr const TCHAR* FootstepGrass = TEXT("Player.Footstep.Grass");
	inline constexpr const TCHAR* FootstepWood = TEXT("Player.Footstep.Wood");
	inline constexpr const TCHAR* FootstepStone = TEXT("Player.Footstep.Stone");
	inline constexpr const TCHAR* Jump = TEXT("Player.Jump");
	inline constexpr const TCHAR* Land = TEXT("Player.Land");
	inline constexpr const TCHAR* PlayerHurt = TEXT("Player.Hurt");
	/** The low-health heartbeat (a loop). */
	inline constexpr const TCHAR* LowHealth = TEXT("Player.LowHealth");
	inline constexpr const TCHAR* PlayerDeath = TEXT("Player.Death");
	/** The slide's start (a rush of cloth and grit), and its scrape along the ground (a loop). */
	inline constexpr const TCHAR* Slide = TEXT("Player.Slide");
	inline constexpr const TCHAR* SlideLoop = TEXT("Player.SlideLoop");

	// --- Creatures (a bigger one plays its kind's cues lower: PitchScale) ---
	/** A creature's body taking a bullet, for a kind with no hit of its own; each kind's is below (its Hit). */
	inline constexpr const TCHAR* CreatureHit = TEXT("Creature.Hit");
	/** A bullet cracking a spider's chitin. */
	inline constexpr const TCHAR* SpiderHit = TEXT("Creature.Spider.Hit");
	inline constexpr const TCHAR* SpiderAlert = TEXT("Creature.Spider.Alert");
	inline constexpr const TCHAR* SpiderAttack = TEXT("Creature.Spider.Attack");
	inline constexpr const TCHAR* SpiderHurt = TEXT("Creature.Spider.Hurt");
	inline constexpr const TCHAR* SpiderDeath = TEXT("Creature.Spider.Death");
	/** A bullet into a slime's jelly: a wet squelch. */
	inline constexpr const TCHAR* SlimeHit = TEXT("Creature.Slime.Hit");
	inline constexpr const TCHAR* SlimeHop = TEXT("Creature.Slime.Hop");
	inline constexpr const TCHAR* SlimeAttack = TEXT("Creature.Slime.Attack");
	inline constexpr const TCHAR* SlimeHurt = TEXT("Creature.Slime.Hurt");
	inline constexpr const TCHAR* SlimeDeath = TEXT("Creature.Slime.Death");
	/** A bullet through a ghost: a hollow thump and a puff of grave dust. */
	inline constexpr const TCHAR* UnpaidHit = TEXT("Creature.Unpaid.Hit");
	inline constexpr const TCHAR* UnpaidAlert = TEXT("Creature.Unpaid.Alert");
	inline constexpr const TCHAR* UnpaidShriek = TEXT("Creature.Unpaid.Shriek");
	inline constexpr const TCHAR* UnpaidLunge = TEXT("Creature.Unpaid.Lunge");
	inline constexpr const TCHAR* UnpaidHurt = TEXT("Creature.Unpaid.Hurt");
	inline constexpr const TCHAR* UnpaidDeath = TEXT("Creature.Unpaid.Death");

	// --- The world ---
	inline constexpr const TCHAR* ChapelBellToll = TEXT("World.ChapelBell.Toll");
	inline constexpr const TCHAR* JettyBell = TEXT("World.JettyBell");
	inline constexpr const TCHAR* ShutterSlam = TEXT("World.ShutterSlam");
}
