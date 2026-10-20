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
	/** A long gun put away as a swap begins: the sleeve and sling swishing, the hardware's jingle, the gun settling on the back. */
	inline constexpr const TCHAR* Holster = TEXT("Weapon.Holster");
	/** Raising the sights (right mouse). */
	inline constexpr const TCHAR* AimIn = TEXT("Weapon.AimIn");
	/** A bullet meeting the world: ground and stone; wood; metal; water. */
	inline constexpr const TCHAR* ImpactWorld = TEXT("Impact.World");
	inline constexpr const TCHAR* ImpactWood = TEXT("Impact.Wood");
	inline constexpr const TCHAR* ImpactMetal = TEXT("Impact.Metal");
	inline constexpr const TCHAR* ImpactWater = TEXT("Impact.Water");

	// --- Bullet impacts by surface (2026-10-13, wave 3a: UBulletSubsystem through ImpactSurface::Cue, and a melee blow on a
	//     wall; Art/Sounds/recipes/impacts.py). All 3D at the hit, short and punchy, dry under the gun's own report;
	//     several takes each (a long burst lands on one surface), varied in pitch and texture. Until a cue has its sounds
	//     the old Impact.World, .Wood, .Metal or .Water plays in its place (ImpactSurface::FallbackCue). ---
	namespace Impact
	{
		/** Packed dry earth (roads, paths, bare ground): a dull, heavy thud and a gritty spray of soil pattering down. */
		inline constexpr const TCHAR* Dirt = TEXT("Impact.Dirt");
		/** Grass over soil: a softer, muffled thud with a quick tearing swish of blades and a little soil thrown. */
		inline constexpr const TCHAR* Grass = TEXT("Impact.Grass");
		/** Gravel, scree and rail ballast: a sharp crunch and small stones skittering and clicking apart. */
		inline constexpr const TCHAR* Gravel = TEXT("Impact.Gravel");
		/** Dry sand and dust: a soft, breathy puff with a fine hiss of grains settling. */
		inline constexpr const TCHAR* Sand = TEXT("Impact.Sand");
		/** Rock and masonry (cliffs, outcrops, boulders, stone walls, chimneys): a hard, bright crack and chip, a short rattle of grit. */
		inline constexpr const TCHAR* Stone = TEXT("Impact.Stone");
		/** Timber (buildings, fences, crates, the depot, trunks): a solid knock with a splintering crackle (made already: 6 takes). */
		inline constexpr const TCHAR* Wood = TEXT("Impact.Wood");
		/** Heavy iron and steel (rails, the lift, the windmill, straps): a bright clank with a short ring (made already: 6 takes). */
		inline constexpr const TCHAR* Metal = TEXT("Impact.Metal");
		/** Thin sheet metal (tin roofs, drums, mailboxes, buckets, wire screens): a hollow, tinny bang, a rattle, a wobbling ring. */
		inline constexpr const TCHAR* SheetMetal = TEXT("Impact.SheetMetal");
		/** Window glass: the pane's sharp crack and a spill of shards tinkling down after it. */
		inline constexpr const TCHAR* Glass = TEXT("Impact.Glass");
		/** Water: a quick plop and splash, droplets falling back (made already: 6 takes). */
		inline constexpr const TCHAR* Water = TEXT("Impact.Water");
		/** Cloth, canvas, rope and leather: a muffled thwip, the fabric snapping taut and a soft flutter. */
		inline constexpr const TCHAR* Cloth = TEXT("Impact.Cloth");
		/** Hay and straw (bales, stacks): a soft, rustling thump and straws scattering. */
		inline constexpr const TCHAR* Hay = TEXT("Impact.Hay");
		/** Bone and skulls: a dry, hollow crack, brittle and light. */
		inline constexpr const TCHAR* Bone = TEXT("Impact.Bone");
		/** A glancing round off stone or metal: a ricochet's high, falling whine tumbling away (layered on about half the
		 *  grazing hits, never two within 0.35 s); 6 takes, varied in pitch and length. */
		inline constexpr const TCHAR* Ricochet = TEXT("Impact.Ricochet");
	}

	/** The Drover revolver's own sounds (WeaponSounds picks them by the gun's kind and its reload's steps). */
	namespace Revolver
	{
		/** A heavy six-gun's shot: the hammer's fall, a big blast with the cylinder gap's spit, the land answering. */
		inline constexpr const TCHAR* Fire = TEXT("Weapon.Revolver.Fire");
		/** The hammer falling on a spent chamber, and the cylinder's hand clicking it round. */
		inline constexpr const TCHAR* DryFire = TEXT("Weapon.Revolver.DryFire");
		/** The latch pushed and the cylinder swung out on its crane. */
		inline constexpr const TCHAR* CylinderOut = TEXT("Weapon.Revolver.CylinderOut");
		/** The ejector rod punched: six empties tumble out and ring on the ground. */
		inline constexpr const TCHAR* Eject = TEXT("Weapon.Revolver.Eject");
		/** A speedloader's rounds dropped into the chambers and let go. */
		inline constexpr const TCHAR* RoundsIn = TEXT("Weapon.Revolver.RoundsIn");
		/** The cylinder slapped home and latched. */
		inline constexpr const TCHAR* CylinderIn = TEXT("Weapon.Revolver.CylinderIn");
		/** Drawn from the holster: leather, the grip in the palm, the hammer thumbed back. */
		inline constexpr const TCHAR* Equip = TEXT("Weapon.Revolver.Equip");
		/** Put back in the holster as a swap begins: the hammer let down, the barrel into the leather, the gun seated. */
		inline constexpr const TCHAR* Holster = TEXT("Weapon.Revolver.Holster");
	}

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
	/** A control hint coming up on the HUD (played quietly: a nudge, not an event), and its key used. */
	inline constexpr const TCHAR* Hint = TEXT("UI.Hint");
	inline constexpr const TCHAR* HintDone = TEXT("UI.HintDone");
	/** A posting turned in at a notice board: a pin pulled and a stamp. */
	inline constexpr const TCHAR* BoardTurnIn = TEXT("UI.BoardTurnIn");
	/** Fast travel between respawn graves (the inventory's map): a rush of grave-wind under the fade to black, settling
	 *  as the screen comes back (UGraveTravelSubsystem). */
	inline constexpr const TCHAR* TravelWhoosh = TEXT("UI.TravelWhoosh");

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

	/** A soul-mote's sounds (ASoulMotePickup), heard where it is. */
	namespace SoulMote
	{
		/** A kill lets go of its mote or motes: a soft rising chime at the body. */
		inline constexpr const TCHAR* Drop = TEXT("Loot.SoulMote.Drop");
		/** A mote taken into the player: a warm little swell. */
		inline constexpr const TCHAR* Pickup = TEXT("Loot.SoulMote.Pickup");
	}

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
	/** A mantle's and a vault's own sounds, as the move starts: cloth, effort, the hands taking the weight (the top's
	 *  surface is a footstep cue on its own). */
	inline constexpr const TCHAR* Mantle = TEXT("Player.Mantle");
	inline constexpr const TCHAR* Vault = TEXT("Player.Vault");

	/** The melee strike (UPlayerMeleeComponent; MeleeCue in Player/PlayerMeleeRules.h is this namespace). A blow that
	 *  lands plays Hit and, on a creature, the layer for what its body is made of. */
	namespace Melee
	{
		/** 2D: the swing's whoosh as it starts, a short rush of air past the ear (every strike; a fist's a touch higher). */
		inline constexpr const TCHAR* Swing = TEXT("Player.Melee.Swing");
		/** The blow landing: a heavy, dull thud with a low body to it (every strike that lands; a wall's at half volume). */
		inline constexpr const TCHAR* Hit = TEXT("Player.Melee.Hit");
		/** Layered on a creature of flesh (an Unpaid, anything not shell or gel): a meaty slap with a wet edge. */
		inline constexpr const TCHAR* HitFlesh = TEXT("Player.Melee.HitFlesh");
		/** Layered on a spider: a hard crack of chitin with a brittle click. */
		inline constexpr const TCHAR* HitShell = TEXT("Player.Melee.HitShell");
		/** Layered on a slime: a springy, squelching smack. */
		inline constexpr const TCHAR* HitGel = TEXT("Player.Melee.HitGel");
	}

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
	/** 2D: "a high rank appears", a creature of rank (Restless or better) turning on the player (UCreaturePackComponent). */
	inline constexpr const TCHAR* RankSting = TEXT("Creature.RankSting");
	/** An ambush's entrances (AAmbushSpawner): the Unpaid rising from their graves, a spider dropping on its silk. */
	inline constexpr const TCHAR* UnpaidRise = TEXT("Creature.Unpaid.Rise");
	inline constexpr const TCHAR* SpiderDrop = TEXT("Creature.Spider.Drop");

	// --- The world ---
	inline constexpr const TCHAR* ChapelBellToll = TEXT("World.ChapelBell.Toll");
	inline constexpr const TCHAR* JettyBell = TEXT("World.JettyBell");
	inline constexpr const TCHAR* ShutterSlam = TEXT("World.ShutterSlam");

	// --- Bosses (UBossComponent's show, ABossLootShower, the Gravemother's and Abel's own moments) ---
	/** 2D: the sting as a boss's bar sweeps in. */
	inline constexpr const TCHAR* BossIntro = TEXT("Boss.Intro");
	/** 2D: the sting at a boss's later phase. */
	inline constexpr const TCHAR* BossPhase = TEXT("Boss.Phase");
	/** At the boss: its weak spot broken, it staggers. */
	inline constexpr const TCHAR* BossStagger = TEXT("Boss.Stagger");
	/** 2D: the low boom under a boss's slow-motion death. */
	inline constexpr const TCHAR* BossDeath = TEXT("Boss.Death");
	/** At the shower: a boss's loot bursting out, and each piece popping out of it. */
	inline constexpr const TCHAR* BossLootBurst = TEXT("Boss.Loot.Burst");
	inline constexpr const TCHAR* BossLootPop = TEXT("Boss.Loot.Pop");
	/** The Gravemother: her scream as she rears, her forelegs slamming down, the ground cracking before her quake. */
	inline constexpr const TCHAR* GravemotherRoar = TEXT("Boss.Gravemother.Roar");
	inline constexpr const TCHAR* GravemotherSlam = TEXT("Boss.Gravemother.Slam");
	inline constexpr const TCHAR* GravemotherQuake = TEXT("Boss.Gravemother.Quake");
	/** Her old venom and burning crack (the Barrow fight replaced them, 2026-10-20: unused, kept until the bank drops them). */
	inline constexpr const TCHAR* GravemotherGurgle = TEXT("Boss.Gravemother.Gurgle");
	inline constexpr const TCHAR* GravemotherSpit = TEXT("Boss.Gravemother.Spit");
	inline constexpr const TCHAR* GravemotherCrackBurn = TEXT("Boss.Gravemother.CrackBurn");
	inline constexpr const TCHAR* GravemotherDeath = TEXT("Boss.Gravemother.Death");
	/** Her Grave Clod: the tell (legs tearing into the floor, the clod wrenched up, 1.2 s, kept and cut short when she's
	 *  interrupted), the throw's heave and whoosh (follows the clod), its landing burst, and its burst on her when it's shot
	 *  in her grip (her pained shriek). */
	inline constexpr const TCHAR* GravemotherClodDig = TEXT("Boss.Gravemother.ClodDig");
	inline constexpr const TCHAR* GravemotherClodThrow = TEXT("Boss.Gravemother.ClodThrow");
	inline constexpr const TCHAR* GravemotherClodBurst = TEXT("Boss.Gravemother.ClodBurst");
	inline constexpr const TCHAR* GravemotherClodShot = TEXT("Boss.Gravemother.ClodShot");
	/** Her headstone: a bullet on granite (often), reaching a cracked state, cracking off. */
	inline constexpr const TCHAR* GravemotherStoneHit = TEXT("Boss.Gravemother.StoneHit");
	inline constexpr const TCHAR* GravemotherStoneCrack = TEXT("Boss.Gravemother.StoneCrack");
	inline constexpr const TCHAR* GravemotherStoneBreak = TEXT("Boss.Gravemother.StoneBreak");
	/** Her Furrow: the 2 s tell (the ground heaving and splitting; kept), the plough (kept), a row of berms crumbling. */
	inline constexpr const TCHAR* GravemotherHeave = TEXT("Boss.Gravemother.Heave");
	inline constexpr const TCHAR* GravemotherPlough = TEXT("Boss.Gravemother.Plough");
	inline constexpr const TCHAR* GravemotherBermCrumble = TEXT("Boss.Gravemother.BermCrumble");
	/** Her Unearthed: a mound stirring as it bulges (1 s, kept), bursting as its spiderling digs out, caved in by a shot. */
	inline constexpr const TCHAR* GravemotherMoundBulge = TEXT("Boss.Gravemother.MoundBulge");
	inline constexpr const TCHAR* GravemotherMoundBurst = TEXT("Boss.Gravemother.MoundBurst");
	inline constexpr const TCHAR* GravemotherMoundPop = TEXT("Boss.Gravemother.MoundPop");
	/** Abel: his moan as he raises his lantern, his wail at a phase, the lanterns guttering out, his flare in the fog. */
	inline constexpr const TCHAR* AbelIntro = TEXT("Boss.Abel.Intro");
	inline constexpr const TCHAR* AbelWail = TEXT("Boss.Abel.Wail");
	inline constexpr const TCHAR* AbelLanternsOut = TEXT("Boss.Abel.LanternsOut");
	inline constexpr const TCHAR* AbelFogFlare = TEXT("Boss.Abel.FogFlare");
	/** 2D: the Gravewind rising off the point. */
	inline constexpr const TCHAR* AbelWindRise = TEXT("Boss.Abel.WindRise");
	/** His gasp as he's staggered to a knee, and his last breath. */
	inline constexpr const TCHAR* AbelStagger = TEXT("Boss.Abel.Stagger");
	inline constexpr const TCHAR* AbelDeath = TEXT("Boss.Abel.Death");

	// --- Feedback (the hit and kill pass, the loot fanfare) ---
	/** A gun dropped by a kill or a chest as it lands, by rarity (ULootFanfareSubsystem::DropCue; a Common has only Loot.Land). */
	inline constexpr const TCHAR* DropUncommon = TEXT("Loot.Drop.Uncommon");
	inline constexpr const TCHAR* DropRare = TEXT("Loot.Drop.Rare");
	/** 2D, like the Legendary's: an Epic or a Legendary landing is heard wherever the player is looking. */
	inline constexpr const TCHAR* DropEpic = TEXT("Loot.Drop.Epic");
	inline constexpr const TCHAR* DropLegendary = TEXT("Loot.Drop.Legendary");
	/** A creature's body as it dies (its death burst), under its death cry: a spider's shell, a slime's splat, an Unpaid's soul-light. */
	inline constexpr const TCHAR* SpiderBurst = TEXT("Creature.Spider.Burst");
	inline constexpr const TCHAR* SlimeSplat = TEXT("Creature.Slime.Splat");
	inline constexpr const TCHAR* UnpaidDissolve = TEXT("Creature.Unpaid.Dissolve");

	// --- Ambience (UAmbienceSubsystem, UAmbientEmitterComponent; Art/Sounds/recipes/ambience.py and world.py) ---
	/** An area's bed in each light: the air (wind, grass; a 19 s loop) and the life (insects; 13 s), played together. */
	inline constexpr const TCHAR* SkyreachAir = TEXT("Ambience.Skyreach.Air");
	inline constexpr const TCHAR* SkyreachLife = TEXT("Ambience.Skyreach.Life");
	inline constexpr const TCHAR* RansomsRestDayAir = TEXT("Ambience.RansomsRest.DayAir");
	inline constexpr const TCHAR* RansomsRestDayLife = TEXT("Ambience.RansomsRest.DayLife");
	inline constexpr const TCHAR* RansomsRestDuskAir = TEXT("Ambience.RansomsRest.DuskAir");
	inline constexpr const TCHAR* RansomsRestDuskLife = TEXT("Ambience.RansomsRest.DuskLife");
	/** Sweeteners: single calls placed round the listener at random distances and times (AmbienceRules::SweetenersFor). */
	inline constexpr const TCHAR* Songbird = TEXT("Ambience.Bird.Songbird");
	inline constexpr const TCHAR* Crow = TEXT("Ambience.Bird.Crow");
	inline constexpr const TCHAR* Hawk = TEXT("Ambience.Bird.Hawk");
	inline constexpr const TCHAR* Owl = TEXT("Ambience.Bird.Owl");
	inline constexpr const TCHAR* InsectChirp = TEXT("Ambience.Insect.Chirp");
	inline constexpr const TCHAR* Bee = TEXT("Ambience.Insect.Bee");
	inline constexpr const TCHAR* CoyoteFar = TEXT("Ambience.Coyote.Far");
	/** Far thunder rolls in from everywhere: the one sweetener heard flat (2D). */
	inline constexpr const TCHAR* ThunderFar = TEXT("Ambience.Thunder.Far");
	/** Places that make sound (loops on emitters; a creek's follows the point on it nearest the listener). */
	inline constexpr const TCHAR* Creek = TEXT("World.Creek");
	inline constexpr const TCHAR* Pond = TEXT("World.Pond");
	inline constexpr const TCHAR* Waterfall = TEXT("World.Waterfall");
	/** The windmill's fan turning (a loop its speed bends) and its head swinging round on the post (one-shots). */
	inline constexpr const TCHAR* WindmillFan = TEXT("World.Windmill.Fan");
	inline constexpr const TCHAR* WindmillCreak = TEXT("World.Windmill.Creak");
	/** The chapel bell at rest, stirred by the wind in the belfry. */
	inline constexpr const TCHAR* ChapelBellHum = TEXT("World.ChapelBell.Hum");
	/** The train's locomotive standing in steam, while it's warm. */
	inline constexpr const TCHAR* TrainHiss = TEXT("World.Train.Hiss");
	/** The canyon's hollow wind along the Rim. */
	inline constexpr const TCHAR* RimWind = TEXT("World.Rim.Wind");
	/** The Sink's damp drips (a loop) and its webs creaking (one-shots). */
	inline constexpr const TCHAR* SinkDrip = TEXT("World.Sink.Drip");
	inline constexpr const TCHAR* SinkCreak = TEXT("World.Sink.Creak");
	/** Main Street holding its breath (the wind in its gaps, a loop) and its one-shots: boards and signs, a loose
	 *  shutter, the living muffled behind their walls. */
	inline constexpr const TCHAR* TownHush = TEXT("World.Town.Hush");
	inline constexpr const TCHAR* TownCreak = TEXT("World.Town.Creak");
	inline constexpr const TCHAR* ShutterTap = TEXT("World.Shutter.Tap");
	inline constexpr const TCHAR* TownMurmur = TEXT("World.Town.Murmur");

	/** The ambient fauna (World/Fauna*; Art/Sounds/recipes/fauna.py; FaunaCue in World/FaunaCues.h is this namespace),
	 *  all 3D. */
	namespace Fauna
	{
		/** A crow's harsh caw, two or three in a row, from a perched or circling crow (heard to about 60 m). */
		inline constexpr const TCHAR* CrowCaw = TEXT("World.Fauna.Crow.Caw");
		/** A crow flock bursting off its perches: heavy wingbeats overlapping, a startled caw. */
		inline constexpr const TCHAR* CrowTakeOff = TEXT("World.Fauna.Crow.TakeOff");
		/** A few sparrows chirping and twittering on a fence or roof. */
		inline constexpr const TCHAR* SparrowChirp = TEXT("World.Fauna.Sparrow.Chirp");
		/** A small flock's flurry of quick wingbeats as it flushes, a soft "frrrt". */
		inline constexpr const TCHAR* SparrowTakeOff = TEXT("World.Fauna.Sparrow.TakeOff");
		/** A swallow's liquid twitter as it swoops past. */
		inline constexpr const TCHAR* SwallowTwitter = TEXT("World.Fauna.Swallow.Twitter");
		/** A hawk's thin, falling scream from high overhead. */
		inline constexpr const TCHAR* HawkCry = TEXT("World.Fauna.Hawk.Cry");
		/** Loop: a cloud of flies buzzing over an outhouse or the den's larder (about 10 m). */
		inline constexpr const TCHAR* Flies = TEXT("World.Fauna.Flies");
		/** A dragonfly's short dry wing rattle as it darts past close by. */
		inline constexpr const TCHAR* DragonflyBuzz = TEXT("World.Fauna.Dragonfly.Buzz");
		/** A tumbleweed's dry twiggy scrape and bounce on the ground. */
		inline constexpr const TCHAR* TumbleweedBounce = TEXT("World.Fauna.Tumbleweed.Bounce");
		/** Loop: a dust devil's whirl, a hissing gust with grit in it (about 40 m). */
		inline constexpr const TCHAR* DustDevil = TEXT("World.Fauna.DustDevil");
		/** Washing snapping and flapping on its line in a gust. */
		inline constexpr const TCHAR* ClothFlap = TEXT("World.Fauna.Cloth.Flap");
	}

	// --- The town's small animals (World/ChickenYard, World/BarnCat; Art/Sounds/recipes/animals.py), all 3D ---
	namespace Animals
	{
		/** A hen's contented "buk-buk-buk": two to four short, low, throaty pulses (about 15 m, two at once). */
		inline constexpr const TCHAR* ChickenCluck = TEXT("Creature.Chicken.Cluck");
		/** The yard scattering: ragged alarm squawks over a flurry of wingbeats, about 1.4 s. */
		inline constexpr const TCHAR* ChickenSquawk = TEXT("Creature.Chicken.Squawk");
		/** The rooster's crow in four parts, about 1.8 s plus the town's echo (about 60 m, one at a time). */
		inline constexpr const TCHAR* ChickenCrow = TEXT("Creature.Chicken.Crow");
		/** A barn cat's rare meow: "mi-aou", a short "mew" or a rolled "mrow". */
		inline constexpr const TCHAR* CatMeow = TEXT("Creature.Cat.Meow");
		/** The cat's hiss at a player running up to it: a spat start, breath through the teeth, a throat growl. */
		inline constexpr const TCHAR* CatHiss = TEXT("Creature.Cat.Hiss");
	}

	// --- Creature reactions (2026-10-14; Art/Sounds/recipes/reactions.py): 3D, through the creature's own voice ---
	namespace React
	{
		/** A spider rearing at the sight of you: forelegs tapping, chitin scraping, a dry hiss. */
		inline constexpr const TCHAR* SpiderNotice = TEXT("Creature.Spider.Notice");
		/** A spider coming up out of the ground: soil splitting, legs scrabbling for a hold. */
		inline constexpr const TCHAR* SpiderEmerge = TEXT("Creature.Spider.Emerge");
		/** A dead spider's leg twitching: a faint dry chitin tick. */
		inline constexpr const TCHAR* SpiderTwitch = TEXT("Creature.Spider.Twitch");
		/** A dying spider's body slamming down at the end of its throe, on its death cry's knock (ASpiderCreature::LandDeadBody):
		 *  a soft, heavy thud into dirt with the shell's hollow chitin clack, legs knocking down round it, grit thrown up. */
		inline constexpr const TCHAR* SpiderThud = TEXT("Creature.Spider.Thud");
		/** A slime shivering as it senses you: a fast wet tremble with tiny pops. */
		inline constexpr const TCHAR* SlimeNotice = TEXT("Creature.Slime.Notice");
		/** A bubble rising through gel: a soft gloop and a small pop. */
		inline constexpr const TCHAR* SlimeBubble = TEXT("Creature.Slime.Bubble");
		/** A slime drawing itself up out of a puddle: a wet suck, then the plop. */
		inline constexpr const TCHAR* SlimeSwell = TEXT("Creature.Slime.Swell");
		/** An Unpaid's coal flaring as it spots you: an ember whoomph and a breath drawn in. */
		inline constexpr const TCHAR* UnpaidNotice = TEXT("Creature.Unpaid.Notice");
		/** An Unpaid breaking into ash: a puff, a dry papery crumble, flakes ticking, a soft hiss. */
		inline constexpr const TCHAR* UnpaidAsh = TEXT("Creature.Unpaid.Ash");
		/** A dead miner looking up from its work: iron dragged on stone and a gravelly grunt. */
		inline constexpr const TCHAR* DeadMinerNotice = TEXT("Creature.DeadMiner.Notice");
		/** The Bell-Ringer noticing you: one muffled clapper knock on the bronze hand bell. */
		inline constexpr const TCHAR* BellRingerNotice = TEXT("Creature.BellRinger.Notice");
	}

	// --- Spent brass by surface (FWeaponBrass; Art/Sounds/recipes/brass.py): 3D where the case lands, about 15 m, mixed low ---
	namespace Casing
	{
		/** A case into dirt, grass or sand: a dull muffled tick. */
		inline constexpr const TCHAR* Dirt = TEXT("Weapon.Casing.Dirt");
		/** Brass on rock or gravel: a bright high tink-tink with quick bounces. */
		inline constexpr const TCHAR* Stone = TEXT("Weapon.Casing.Stone");
		/** Brass on boards: a hollow woody tock, then a softer tick. */
		inline constexpr const TCHAR* Wood = TEXT("Weapon.Casing.Wood");
		/** Brass on steel or tin: a clear ringing ting with a short shimmer. */
		inline constexpr const TCHAR* Metal = TEXT("Weapon.Casing.Metal");
		/** A case into water: a tiny plip. */
		inline constexpr const TCHAR* Water = TEXT("Weapon.Casing.Water");
		/** A paper shotgun hull on hard ground: a hollow low clack, a brass-head clink, a short roll. */
		inline constexpr const TCHAR* Hull = TEXT("Weapon.Casing.Hull");
	}

	// --- Fly-bys (NearMiss::Play, at the shot's closest point to the head; Art/Sounds/recipes/brass.py) ---
	namespace FlyBy
	{
		/** A bullet past the ear: a sharp crack and a whine falling in pitch. */
		inline constexpr const TCHAR* Whiz = TEXT("Weapon.NearMiss.Whiz");
		/** A venom glob rushing past: a wet sputtering hiss, pitch falling. */
		inline constexpr const TCHAR* Hiss = TEXT("Weapon.NearMiss.Hiss");
	}

	// --- The player's feet (UPlayerFootfallComponent's FootfallCue; Art/Sounds/recipes): 3D at the feet ---
	namespace Footfall
	{
		/** A hard landing's heavy low thud with a little dirt scatter, under Player.Land. */
		inline constexpr const TCHAR* LandHard = TEXT("Player.Land.Hard");
		/** A foot or a body coming down in shallow water. */
		inline constexpr const TCHAR* Splash = TEXT("Player.Splash");
	}

	// --- Music (UMusicDirectorSubsystem; Art/Sounds/recipes/music.py; bars and tempos in MusicRules) ---
	/** The exploration themes (32 bars of 6/8 in D at 75), and the combat layer laid over either (16 bars on a D pedal). */
	inline constexpr const TCHAR* MusicSkyreachExplore = TEXT("Music.Skyreach.Explore");
	inline constexpr const TCHAR* MusicRansomsRestExplore = TEXT("Music.RansomsRest.Explore");
	inline constexpr const TCHAR* MusicCombat = TEXT("Music.Combat");
	/** The boss themes: Abel's (4/4 at 128) and the Gravemother's (4/4 at 120), 16 bars each. */
	inline constexpr const TCHAR* MusicBossKeeper = TEXT("Music.Boss.Keeper");
	inline constexpr const TCHAR* MusicBossGravemother = TEXT("Music.Boss.Gravemother");
	/** Stingers: a creature of high rank comes for the player; a boss's fight begins or turns (its hit lands 0.75 s in,
	 *  MusicRules::PhaseStingHit); a fight is won. */
	inline constexpr const TCHAR* StingElite = TEXT("Music.Sting.Elite");
	inline constexpr const TCHAR* StingPhase = TEXT("Music.Sting.Phase");
	inline constexpr const TCHAR* StingVictory = TEXT("Music.Sting.Victory");

	// --- Inventory (2D: the loadout screen's gun moves, ULoadoutWidget; LoadoutParts::Sounds is this namespace) ---
	namespace Inventory
	{
		/** A gun seated in an equip slot (from the backpack, or two slots trading places): a firm metal clack. */
		inline constexpr const TCHAR* Equip = TEXT("UI.Equip");
		/** A gun put away into the backpack: a slide and a soft thump. */
		inline constexpr const TCHAR* Stow = TEXT("UI.Stow");
		/** A gun dropped from the inventory: a heavy drop on boards. */
		inline constexpr const TCHAR* Drop = TEXT("UI.Drop");
		/** Inspect opening: a short rising whoosh. */
		inline constexpr const TCHAR* Inspect = TEXT("UI.Inspect");
	}

	// --- The lootable world (ABreakableProp, AChest's graves, coffins, mailboxes and footlockers; Art/Sounds/recipes/props.py) ---
	namespace Lootables
	{
		/** A crate bursting: the boards' crack and splinter, the nails' squeal, the pieces clattering down. */
		inline constexpr const TCHAR* BreakCrate = TEXT("Loot.Break.Crate");
		/** A barrel bursting: the staves' crack, a hoop's ring as it springs off, the staves clattering down. */
		inline constexpr const TCHAR* BreakBarrel = TEXT("Loot.Break.Barrel");
		/** A grave dug up: spade bites into packed earth, dirt thrown on the heap, the spade striking the coffin's lid. */
		inline constexpr const TCHAR* GraveDig = TEXT("Loot.Grave.Dig");
		/** A coffin pried open: a crowbar's bite, old nails squealing out of the pine, the wood groaning. */
		inline constexpr const TCHAR* CoffinPry = TEXT("Loot.Coffin.Pry");
		/** A coffin's lid shoved off: a dry scrape across the box and its knock coming down. */
		inline constexpr const TCHAR* CoffinLidOff = TEXT("Loot.Coffin.LidOff");
		/** A mailbox's tin door dropped open: a rusty squeak and a light clank on its stop. */
		inline constexpr const TCHAR* MailboxOpen = TEXT("Loot.Mailbox.Open");
		/** A footlocker opened: its hasp flipped, the hinges' creak, the lid knocking back on its strap. */
		inline constexpr const TCHAR* FootlockerOpen = TEXT("Loot.Footlocker.Open");
	}

	// --- The grave-salt grenade (UPlayerThrowComponent, AGraveSaltGrenade, GraveSaltBurst, AGrenadePickup;
	//     Art/Sounds/recipes/throwables.py; ThrowCue in Player/PlayerThrowRules.h is this namespace) ---
	namespace Throwable
	{
		/** 2D: the tin swung up out of the off hand and slung, salt shifting inside it and a rush of air (every throw). */
		inline constexpr const TCHAR* Throw = TEXT("Throwable.SaltGrenade.Throw");
		/** The tin knocking off ground or a wall as it bounces: a hollow tin clink, salt rattling (played louder the harder). */
		inline constexpr const TCHAR* Bounce = TEXT("Throwable.SaltGrenade.Bounce");
		/** The waxed fuse burning down (a loop, on the tin): a fizz and sputter that spits. */
		inline constexpr const TCHAR* Fuse = TEXT("Throwable.SaltGrenade.Fuse");
		/** The burst: a punchy thump with the salt's bright crystalline crackle over it, carried far like a gunshot. */
		inline constexpr const TCHAR* Burst = TEXT("Throwable.SaltGrenade.Burst");
		/** Salt searing one of the Unpaid it burned: a hiss and spit, with a ghostly gasp under it. */
		inline constexpr const TCHAR* Sear = TEXT("Throwable.SaltGrenade.Sear");
		/** A grenade taken from a pickup: the tin picked up with a slosh of salt and a tick of its stopper. */
		inline constexpr const TCHAR* Pickup = TEXT("Throwable.SaltGrenade.Pickup");
	}

	// --- Voices (Art/Sounds/recipes/voices.py): the Unpaid's barks murmured a syllable at a time (UCreatureVoiceComponent,
	//     CreatureBarks::BuildMurmur), the spiders' and slimes' idle calls, all 3D ---
	namespace Voice
	{
		/** One breathy voiced syllable of an Unpaid's bark (a spot, a hurt, a fallen packmate), pitched per Unpaid and per syllable. */
		inline constexpr const TCHAR* UnpaidMurmur = TEXT("Creature.Unpaid.Murmur");
		/** One whispered syllable: an Unpaid muttering to itself, or its last words. */
		inline constexpr const TCHAR* UnpaidMutter = TEXT("Creature.Unpaid.Mutter");
		/** A spider at rest, rarely: its mandibles working slowly, a short dry rasp, a leg's tap. */
		inline constexpr const TCHAR* SpiderIdle = TEXT("Creature.Spider.Idle");
		/** A slime at rest, rarely: a contented gurgle, bubbles rising through it, a lazy wobble. */
		inline constexpr const TCHAR* SlimeIdle = TEXT("Creature.Slime.Idle");
	}

	// --- Town life (UTownLifeSubsystem; Art/Sounds/recipes/voices.py): the living heard through their walls on Ransom's
	//     Rest, quietly, as the player passes (3D, muffled through a wall, fading out by about 35 m as the street's own) ---
	namespace TownLife
	{
		/** Two hushed voices behind a wall, trading a few words: only their shape gets out. */
		inline constexpr const TCHAR* Voices = TEXT("World.TownLife.Voices");
		/** A cough behind a wall: a man's, a woman's, an old woman's, a child's. */
		inline constexpr const TCHAR* Cough = TEXT("World.TownLife.Cough");
		/** A child's music box behind the shutters: a slow little tune on its comb, one take winding down. */
		inline constexpr const TCHAR* MusicBox = TEXT("World.TownLife.MusicBox");
		/** A dog barking far off across town, a few barks and the echo off the ridges. */
		inline constexpr const TCHAR* DogFar = TEXT("World.TownLife.DogFar");
		/** A door's latch inside: a thumb latch lifted and dropped, a bolt shot home, a chain put on. */
		inline constexpr const TCHAR* Latch = TEXT("World.TownLife.Latch");
		/** A floorboard giving under somebody's weight inside, or a rocking chair. */
		inline constexpr const TCHAR* Creak = TEXT("World.TownLife.Creak");
		/** A mother's "shh", and a child's small whimper quieted. */
		inline constexpr const TCHAR* Hush = TEXT("World.TownLife.Hush");
		/** Tilly at her work: a handsaw's strokes through a board, then a few hammer taps. */
		inline constexpr const TCHAR* Workshop = TEXT("World.TownLife.Workshop");
	}

	// --- Gusts on Ransom's Rest (UGustSubsystem; Art/Sounds/recipes/gusts.py): the wind passing and what it knocks loose ---
	namespace Gust
	{
		/** 2D, one at a time: wind swelling past grass and boards, up in about a second and falling away over three to five
		 *  (the code sets volume 0.45-1 by the gust's strength, pitch +-6%). */
		inline constexpr const TCHAR* Pass = TEXT("World.Gust.Pass");
		/** 3D (about 35 m, two at once): a loose shutter slammed two or three times with its slats chattering, or a hanging
		 *  sign's chain clinking and its bracket creaking. */
		inline constexpr const TCHAR* Rattle = TEXT("World.Gust.Rattle");
	}

	// --- Lit windows (UWindowLifeSubsystem; Art/Sounds/recipes/windows.py): the living behind the glass, 3D, to about 15 m ---
	namespace Window
	{
		/** Somebody crossing a room behind a lit window: five or six muffled footfalls on a plank floor, a board creaking. */
		inline constexpr const TCHAR* Steps = TEXT("World.Window.Steps");
		/** A curtain drawn on its rod behind the glass: cloth swishing and the rings sliding and ticking. */
		inline constexpr const TCHAR* Curtain = TEXT("World.Window.Curtain");
	}

	// --- The economy (ASilverPickup, the vendors' screen UVendorWidget; Art/Sounds/recipes/economy.py): silver found,
	//     a purchase, a sale and a refusal at the counter, all 2D (the player's own) ---
	namespace Economy
	{
		/** Silver picked up as it reaches the player: a bright little chime of silver coins landing in a palm (a big find
		 *  plays a touch lower and fuller). */
		inline constexpr const TCHAR* SilverPickup = TEXT("Loot.Silver.Pickup");
		/** Something bought: coins counted out onto a wooden counter, a small bell over the till, the goods slid across. */
		inline constexpr const TCHAR* Buy = TEXT("Vendor.Buy");
		/** A gun sold: set down on the counter with a solid knock, then coins counted into the hand. */
		inline constexpr const TCHAR* Sell = TEXT("Vendor.Sell");
		/** Not enough silver: a light purse shaken (two lonely coins), a coin pushed back across the counter. */
		inline constexpr const TCHAR* NotEnough = TEXT("Vendor.NotEnough");
	}

	// --- The Venom Spitter (ASpitterSpider, ASpitterGlob, ASpitterPuddle; Art/Sounds/recipes/creatures.py), all 3D ---
	namespace Spitter
	{
		/** The spit's tell, on its body: venom gurgling up through the sac, quickening, and a hiss rising to its fangs. */
		inline constexpr const TCHAR* Tell = TEXT("Creature.Spitter.Tell");
		/** The glob spat: a wet hawk out of its fangs, a slap of air, a spray of droplets. */
		inline constexpr const TCHAR* Spit = TEXT("Creature.Spitter.Spit");
		/** A glob bursting where it lands (and a sac bursting: pitched up): a thick wet splat, a slosh, a hiss of acid. */
		inline constexpr const TCHAR* Splat = TEXT("Creature.Spitter.Splat");
		/** A puddle as it lands: venom sizzling on the ground, popping bubbles, fading as it settles. */
		inline constexpr const TCHAR* Sizzle = TEXT("Creature.Spitter.Sizzle");
		/** A puddle burning the player standing in it (every half second): a short sharp sizzle under the feet. */
		inline constexpr const TCHAR* Burn = TEXT("Creature.Spitter.Burn");
		/** Its hurt and death cries: the spider's hiss and rasp, wetter, with the sac sloshing (UCreatureVoiceComponent's
		 *  CriesFor plays them once it knows the Spitter; until then it cries as a brown spider). */
		inline constexpr const TCHAR* Hurt = TEXT("Creature.Spitter.Hurt");
		inline constexpr const TCHAR* Death = TEXT("Creature.Spitter.Death");
	}

	// --- The Bell-Ringer (ABellRingerCreature; Art/Sounds/recipes/bells.py), all 3D ---
	namespace BellRinger
	{
		/** The toll's tell, on the bell: the clapper rattling as the bell comes up and is shaken, a breath drawn in. */
		inline constexpr const TCHAR* Lift = TEXT("Creature.BellRinger.Lift");
		/** The toll: a bronze hand bell swung and struck, ringing out across the fight (carried like a gunshot). */
		inline constexpr const TCHAR* Toll = TEXT("Creature.BellRinger.Toll");
		/** A Gravebound ringer's toll: the bell cracked, sour and buzzing, choked short. */
		inline constexpr const TCHAR* TollCracked = TEXT("Creature.BellRinger.TollCracked");
		/** A bullet ringing off the bell (and, low and soft, its last knock as the ringer falls): a bright clang, the clapper clattering. */
		inline constexpr const TCHAR* Struck = TEXT("Creature.BellRinger.Struck");
		/** At a fallen one's grave as a toll raises it: many whispering voices rising over the bell's hum. */
		inline constexpr const TCHAR* Raise = TEXT("Creature.BellRinger.Raise");
		/** The bell swung as a club when it's run down: its rush through the air and a dull, choked clank. */
		inline constexpr const TCHAR* Swipe = TEXT("Creature.BellRinger.Swipe");
	}

	// --- The vigil stones (ABossShrine; Art/Sounds/recipes/shrines.py): a boss's stone by its arena ---
	namespace Shrine
	{
		/** The stone rising out of the ground the first time (3D): stone grinding up through earth, dirt pattering off it,
		 *  the candles catching with a soft breath of flame. */
		inline constexpr const TCHAR* Rise = TEXT("World.Shrine.Rise");
		/** Kneeling at it, its menu opening (3D, near): the candle flames gusting, a low hum in the stone, the bell's clapper
		 *  stirring once. */
		inline constexpr const TCHAR* Activate = TEXT("World.Shrine.Activate");
		/** The call refused (2D, the player's own): the iron bell's clapper caught in a hand, a dull choked knock. */
		inline constexpr const TCHAR* Unavailable = TEXT("World.Shrine.Unavailable");
		/** The boss called back (3D, carried like a gunshot): the iron bell rung three times, the candles flaring, the
		 *  grave-wind rising under it with whispering voices, a deep thud as the dead answer. */
		inline constexpr const TCHAR* Summon = TEXT("World.Shrine.Summon");
	}

	// --- Mission givers (2026-10-13: UMissionOfferWidget, mission artifacts; Art/Sounds/recipes/givers.py) ---
	namespace Giver
	{
		/** The offer card opening (2D): paper drawn across wood, a soft low chime under it. */
		inline constexpr const TCHAR* Offer = TEXT("UI.Mission.Offer");
		/** A mission accepted (2D, rewarding): a stamp pressed onto paper, a bright two-note rise. */
		inline constexpr const TCHAR* Accept = TEXT("UI.Mission.Accept");
		/** "Not now" (2D, quiet): the paper set back down. */
		inline constexpr const TCHAR* Decline = TEXT("UI.Mission.Decline");
		/** A new mission offered nearby for the first time (2D, light): a short distant chime, as the ! appears. */
		inline constexpr const TCHAR* NewOffer = TEXT("UI.Mission.NewOffer");
		/** Picking up a mission artifact (3D, near): a letter unfolded, a tag or a journal lifted. */
		inline constexpr const TCHAR* ArtifactRead = TEXT("World.Artifact.Read");
	}

	// --- Respawn graves as save points (2026-10-13: ARespawnMarker marked on approach; Art/Sounds/recipes/graves.py) ---
	namespace GraveMark
	{
		/** A grave marked as the save point (3D, near, carried): its lantern catching with a breath of flame, a low warm
		 *  bell-like swell, a whisper of the Gravewind settling. */
		inline constexpr const TCHAR* Marked = TEXT("World.Grave.Marked");
		/** Walking past a grave already marked elsewhere, making it the save point again (3D, quieter): the flame lifting. */
		inline constexpr const TCHAR* Remarked = TEXT("World.Grave.Remarked");
	}

	// --- Hold to interact (2026-10-13: the round hold ring, HudInteractPromptWidget, the scene skip) ---
	namespace Hold
	{
		/** The ring filling all the way (2D, short, crisp): a soft click with a bright tick on top. */
		inline constexpr const TCHAR* Complete = TEXT("UI.Hold.Complete");
	}

	// --- The Providence's dead (2026-10-13, wave 2c: ADeadMinerCreature, AForemanCreature and his fuse sticks, rock falls and
	//     timber props; Hiram Coyle and Thaddeus Bright). A silver mine: iron on rock, carbide hiss, old pine, black powder ---
	namespace DeadMiner
	{
		/** The swing's wind-up (3D, on it): a dead man's grunt heaving a pick overhead, the haft creaking, its lamp's hiss rising. */
		inline constexpr const TCHAR* Heave = TEXT("Creature.DeadMiner.Heave");
		/** The pick coming down (3D, on it): a heavy whoosh through the air, short. */
		inline constexpr const TCHAR* Swing = TEXT("Creature.DeadMiner.Swing");
		/** The pick biting the floor (3D, carried): a heavy iron chunk into rock, stone chips skittering. */
		inline constexpr const TCHAR* PickBite = TEXT("Creature.DeadMiner.PickBite");
		/** Working a wall unaware (3D, quieter, heard across a gallery): one pick strike ringing on rock, a dull iron ping. */
		inline constexpr const TCHAR* PickWall = TEXT("Creature.DeadMiner.PickWall");
		/** Its head lamp shot out (3D, near): a small glass pop, a hiss of carbide gas dying, a tinkle of glass. */
		inline constexpr const TCHAR* LampBreak = TEXT("Creature.DeadMiner.LampBreak");
		/** A bullet off a lamp already out (3D, near): a small bright tink on brass. */
		inline constexpr const TCHAR* LampStruck = TEXT("Creature.DeadMiner.LampStruck");
		/** Its swing broken by a shot on the lamp (3D): a choked gasp, the pick's haft clattering in its hands. */
		inline constexpr const TCHAR* Reel = TEXT("Creature.DeadMiner.Reel");
		/** At its lunch pail (3D, quiet): a tin lid lifted and set back down. */
		inline constexpr const TCHAR* Pail = TEXT("Creature.DeadMiner.Pail");
		/** Its pick falling as it dies (3D): iron clattering onto stone, a bounce. */
		inline constexpr const TCHAR* Drop = TEXT("Creature.DeadMiner.Drop");
	}

	namespace Foreman
	{
		/** His fight's start, as his bar sweeps in (3D, carried): a mine whistle shrieking down the stope under a big man's bellow. */
		inline constexpr const TCHAR* Intro = TEXT("Boss.Foreman.Intro");
		/** A later phase (3D, carried): his roar of anger echoing off rock, a hammer rung on a steel plate. */
		inline constexpr const TCHAR* Phase = TEXT("Boss.Foreman.Phase");
		/** Knocked to a knee (3D): a heavy grunt, his coat's buckles rattling, the pick handle scraping stone. */
		inline constexpr const TCHAR* Stagger = TEXT("Boss.Foreman.Stagger");
		/** His last breath (3D, carried): a long exhale that breaks, the mine settling round it, a distant drip. */
		inline constexpr const TCHAR* Death = TEXT("Boss.Foreman.Death");
		/** His pick handle slammed on the floor (3D, carried): a heavy wooden crack on stone, grit pattering down after. */
		inline constexpr const TCHAR* Slam = TEXT("Boss.Foreman.Slam");
		/** His coal bared (3D, on him): a low ember roar with a crackle, the weak spot opening. */
		inline constexpr const TCHAR* CoalOpen = TEXT("Boss.Foreman.CoalOpen");
		/** A fuse lit (3D, near): a match scratched hard, then the fuse catching with a spit of sparks. */
		inline constexpr const TCHAR* FuseLight = TEXT("Boss.Foreman.FuseLight");
		/** A lit fuse (3D loop, on the stick): a steady angry hiss, sparks spitting in it now and then. */
		inline constexpr const TCHAR* FuseHiss = TEXT("Boss.Foreman.FuseHiss");
		/** The stick lobbed (3D): a grunt and a short whoosh. */
		inline constexpr const TCHAR* FuseThrow = TEXT("Boss.Foreman.FuseThrow");
		/** The stick landing (3D): a wooden clack on rock, a skitter. */
		inline constexpr const TCHAR* FuseLand = TEXT("Boss.Foreman.FuseLand");
		/** A powder stick going off in a tunnel (3D, carried far): a hard crack, a deep boom, rock rattling down after it. */
		inline constexpr const TCHAR* FuseBurst = TEXT("Boss.Foreman.FuseBurst");
		/** His pick handle hammered into a timber prop (3D, carried): a deep wooden thud, the prop groaning under it. */
		inline constexpr const TCHAR* RoofStrike = TEXT("Boss.Foreman.RoofStrike");
		/** The roof answering (3D, carried, about 2 s, building): timbers creaking, stone grinding overhead, dust starting to sift. */
		inline constexpr const TCHAR* RoofGroan = TEXT("Boss.Foreman.RoofGroan");
		/** Rock falling from the roof onto the floor (3D, carried): a heavy crash and a slide of gravel. */
		inline constexpr const TCHAR* RockFall = TEXT("Boss.Foreman.RockFall");
		/** A timber prop splintering (3D, carried): a loud crack of splitting pine, splinters clattering down. */
		inline constexpr const TCHAR* PropSplinter = TEXT("Boss.Foreman.PropSplinter");
		/** The stope's lanterns blown out at once (3D, carried): a gust through the drift, flames guttering, a sudden hush. */
		inline constexpr const TCHAR* LanternsOut = TEXT("Boss.Foreman.LanternsOut");
		/** The lanterns catching again (3D, carried): a soft whump of flame, glass ticking as it warms. */
		inline constexpr const TCHAR* LanternsBack = TEXT("Boss.Foreman.LanternsBack");
		/** His shift rising out of the rubble (3D, carried): rocks shoved aside, several voices groaning low under them. */
		inline constexpr const TCHAR* ShiftRise = TEXT("Boss.Foreman.ShiftRise");
	}

	namespace DeadFriends
	{
		/** Thaddeus at his bench (3D, quiet; heard through the cold-room door as the cellar's knocking): two knocks of knuckles
		 *  on a coffin board. */
		inline constexpr const TCHAR* BenchKnock = TEXT("World.Thaddeus.Knock");
		/** Thaddeus's tape (3D, quiet): a cloth measuring tape drawn out, then let slide back. */
		inline constexpr const TCHAR* Tape = TEXT("World.Thaddeus.Tape");
		/** Hiram leaning to his speaking tube (3D, quiet): the tube's whistle cap pulled, a hollow breath down the pipe. */
		inline constexpr const TCHAR* Tube = TEXT("World.Hiram.Tube");
	}

	// --- Lost Property (2026-10-13, wave 2e: the depot's LOST PROPERTY window, ALostPropertyWindow and ULostLootWidget;
	//     guns handed in by ULostLootSubsystem). An old railway baggage office: wood, brass, paper tickets, a rubber stamp ---
	namespace LostLoot
	{
		/** The window opened (3D at the window, near): a wooden shutter run up in its grooves, then one small brass counter
		 *  bell, dinged by a palm. */
		inline constexpr const TCHAR* WindowOpen = TEXT("World.LostProperty.Open");
		/** The window closed (3D at the window, near): the shutter drawn down on its runners and a latch dropping. */
		inline constexpr const TCHAR* WindowClose = TEXT("World.LostProperty.Close");
		/** A gun claimed (2D, rewarding): a rubber stamp thumped onto a paper ticket (CLAIMED), the stub torn off along its
		 *  perforation, the gun lifted off a wooden shelf with a light metal knock. */
		inline constexpr const TCHAR* Claim = TEXT("UI.LostProperty.Claim");
		/** A claim refused, no room (2D, short): the ticket slid back across the counter and a dull knuckle-rap on wood. */
		inline constexpr const TCHAR* NoRoom = TEXT("UI.LostProperty.NoRoom");
		/** More shelving bought (2D, rewarding): coins counted onto the counter, then three hammer blows and a plank set down
		 *  on its brackets. */
		inline constexpr const TCHAR* Upgrade = TEXT("UI.LostProperty.Upgrade");
		/** Guns sent to Lost Property, with the HUD's line (2D, soft, under the line): a ledger page turned, a quick pen
		 *  scratch, and a faint station bell far off. */
		inline constexpr const TCHAR* Sent = TEXT("UI.LostProperty.Sent");
	}

	// --- Mission features (2026-10-13, wave 2a: optional objectives, mission items, the turn-in's choice, readables, the
	//     porch plate, the minimap's underground plate) ---
	namespace MissionFeatures
	{
		/** An optional objective ticked on the tracker (2D, short, rewarding, lighter than a step's chime): a crisp pencil
		 *  tick on paper with a small bright two-note sparkle rising after it. */
		inline constexpr const TCHAR* OptionalDone = TEXT("UI.Mission.OptionalDone");
		/** A mission item picked up (3D, near): a small weighty thing lifted and pocketed, cloth and leather, with a soft
		 *  bright metallic glint on top so it reads as the mission's. */
		inline constexpr const TCHAR* ItemTaken = TEXT("World.MissionItem.Take");
		/** A mission item falling from the creature that carried it (3D): a small hard thing thudding into dirt, then a
		 *  soft rising shimmer as its gold beam comes up. */
		inline constexpr const TCHAR* ItemDrop = TEXT("World.MissionItem.Drop");
		/** The turn-in's choice card opening (2D): heavier than the offer card, paper drawn across wood under a low held
		 *  string note that leaves the question hanging. */
		inline constexpr const TCHAR* ChoiceOpen = TEXT("UI.Mission.ChoiceOpen");
		/** Holding a choice down (2D, about a second, faded out if let go): a low swell rising in pitch as the ring fills,
		 *  like a breath drawn in. */
		inline constexpr const TCHAR* ChoiceHold = TEXT("UI.Mission.ChoiceHold");
		/** A choice made (2D, weighty, final): a heavy stamp on paper, then a deep resonant chord settling, a debt paid. */
		inline constexpr const TCHAR* ChoiceMade = TEXT("UI.Mission.ChoiceMade");
		/** Reading something (3D, near): a page turned, a board brushed off, a little chalk dust. */
		inline constexpr const TCHAR* Read = TEXT("World.Readable.Read");
		/** The reading card put away (2D, quiet): the paper set back down. */
		inline constexpr const TCHAR* ReadClose = TEXT("UI.Readable.Close");
		/** The jars taken off the porch plate (3D, near): two glass jars of salt clinking off a tin plate, a folded slip of
		 *  paper opened. */
		inline constexpr const TCHAR* PorchPlateTake = TEXT("World.PorchPlate.Take");
		/** The biscuits eaten (3D, near, slow): a cold dry biscuit broken in two, chewed without hurry, a swallow. */
		inline constexpr const TCHAR* PorchPlateEat = TEXT("World.PorchPlate.Eat");
		/** The minimap going underground (2D, soft, under the music): a muffled low hush, like a door shut on the wind. */
		inline constexpr const TCHAR* Underground = TEXT("UI.Minimap.Underground");
	}

	// --- The ore lift (2026-10-13, wave 2b: AOreLift, its bell pulls and the ride; a steam winch, a haul rope over one
	//     timber tower, an open iron ore cage; all 3D) ---
	namespace OreLift
	{
		/** A bell pull rung to ride or call (near): a bronze signal bell struck twice on its spring, the cord creaking. */
		inline constexpr const TCHAR* BellSignal = TEXT("World.OreLift.BellSignal");
		/** The far landing's bell answering a call (carried a long way, thin with distance): one bright stroke. */
		inline constexpr const TCHAR* BellAnswer = TEXT("World.OreLift.BellAnswer");
		/** The landing's bell as the cage is set down (near): one clear stroke, ringing on a moment. */
		inline constexpr const TCHAR* BellArrive = TEXT("World.OreLift.BellArrive");
		/** The closed lift's cord tugged (near, short): the cord jerked, a cold bell's dull clunk, nothing answering. */
		inline constexpr const TCHAR* BellDead = TEXT("World.OreLift.BellDead");
		/** The winch house's steam whistle as the winch takes up (carried like a gunshot across the valley): two short
		 *  blasts, a little breathy, the second lower. */
		inline constexpr const TCHAR* Whistle = TEXT("World.OreLift.Whistle");
		/** Loop at the winch house while the cage runs: the steam engine's steady chuff, the drum's gear whine, the cable
		 *  paying in or out over its sheave. */
		inline constexpr const TCHAR* WinchRun = TEXT("World.OreLift.WinchRun");
		/** Loop on the cage while it runs (what the rider hears): the rope singing over the grip in a low hum, the splices
		 *  ticking past, chains and planks creaking with the sway, a high wind. */
		inline constexpr const TCHAR* CableRun = TEXT("World.OreLift.CableRun");
		/** The cage lifting off its landing (on the cage): iron straps and timbers groaning as the cable takes the strain,
		 *  a heavy jolt, the chains snapping taut. */
		inline constexpr const TCHAR* TakeUp = TEXT("World.OreLift.TakeUp");
		/** The grip over one of the tower's three sheaves (at the tower, carried): a heavy iron clank and the wheel's short
		 *  ring, a shudder down the cable; heard three times in about a second, so vary each take. */
		inline constexpr const TCHAR* SheaveClank = TEXT("World.OreLift.SheaveClank");
		/** The cage set down on its landing (on the cage): a heavy wooden thump, chains slackening, an iron latch dropping. */
		inline constexpr const TCHAR* CageLand = TEXT("World.OreLift.CageLand");
		/** The cage's gate swung shut behind the rider and latched (near, short): an iron gate's squeal and clack. */
		inline constexpr const TCHAR* GateLatch = TEXT("World.OreLift.GateLatch");
	}

	// --- The lift's fittings and the mine's manway gate (2026-10-13: the player's hands on the iron; all 3D, near) ---
	namespace Fittings
	{
		/** The winch's crank handle fitted onto its square shaft and turned (near, ~1.2 s): the socket sliding home, a heavy
		 *  iron clunk as it seats, then three to five ratchet pawl clicks over a little gear grind. */
		inline constexpr const TCHAR* Crank = TEXT("World.LiftFitting.Crank");
		/** The winch boiler fed (near, ~2 s): the fire door swung open (hinge squeal, a clang), coal shoveled in with a gritty
		 *  rattle, the fire catching with a low whoomp and a short steam hiss. */
		inline constexpr const TCHAR* Boiler = TEXT("World.LiftFitting.Boiler");
		/** The cable clamp's iron pin driven home (near, ~1 s): two or three hammer knocks climbing in pitch as it seats, a
		 *  final ringing clink. */
		inline constexpr const TCHAR* Pin = TEXT("World.LiftFitting.Pin");
		/** A rusty iron grate gate opened in a mine passage (near, ~1.7 s): the latch bar lifted with a clack, a long low
		 *  hinge groan, the bars rattling, ending in a dull knock against rock. */
		inline constexpr const TCHAR* ManwayGate = TEXT("World.ManwayGate.Open");
	}

	// --- The watch fire on Calder's Post's tower (2026-10-13, wave 2b: AWatchFire; all 3D) ---
	namespace WatchFire
	{
		/** The oiled brazier catching (near, carried a little): a breathy whump of lamp oil going up, the fire rushing up
		 *  through the kindling, crackling. */
		inline constexpr const TCHAR* Light = TEXT("World.WatchFire.Light");
		/** Loop while it burns: a big open fire in an iron brazier on a windy tower: crackle and pops over a wind-blown roar
		 *  that rises and falls. */
		inline constexpr const TCHAR* Burn = TEXT("World.WatchFire.Burn");
		/** The dry brazier tried (near, short): a knuckle on an empty iron bowl, cold ash shifting. */
		inline constexpr const TCHAR* Dry = TEXT("World.WatchFire.Dry");
	}

	// --- Hob's roosts (2026-10-13, wave 3: ARespawnMarker as the roost, URoostCarrySubsystem's carry): the one save, respawn
	//     and fast-travel point on every map, its beacon, its crows; a weathered post, an old iron bell, crows, a cold light ---
	namespace RoostMark
	{
		/** A roost found (3D at its bell, carried far): the bell struck once and ringing on, a deep warm swell rushing upward as
		 *  the beacon's light climbs into the sky, the roost's crows bursting off in a clatter of wings and a chorus of caws. */
		inline constexpr const TCHAR* Found = TEXT("World.Roost.Found");
		/** A roost found before, made the save point again (3D, near, quieter): one soft tap of the bell, a low caw, the light's
		 *  hum lifting for a moment. */
		inline constexpr const TCHAR* Remarked = TEXT("World.Roost.Remarked");
		/** Loop near a roost (3D, about 25 m): crows muttering and shuffling on the perches, a low caw now and then, the bell's
		 *  clapper ticking in the wind, the beacon's faint low hum. */
		inline constexpr const TCHAR* Ambient = TEXT("World.Roost.Ambient");
		/** Fast travel leaving (2D, the player's own): a rush of wings closing in from every side, caws all round, a gust as the
		 *  crows lift Ellis off the ground and away. */
		inline constexpr const TCHAR* CarryOut = TEXT("UI.Roost.CarryOut");
		/** Set down at a roost by the crows (3D at the roost, near): wings bursting outward, crows scattering with caws, boots
		 *  landing on dirt. */
		inline constexpr const TCHAR* CarryIn = TEXT("World.Roost.CarryIn");
		/** A death, Hob's crows coming for Ellis (2D, under the dying): wingbeats gathering and circling closer, harsh caws, then a
		 *  heavy drag through dirt and gravel under a rush of wings. */
		inline constexpr const TCHAR* DeathDrag = TEXT("UI.Roost.DeathDrag");
		/** Woken at a roost after a death (3D at the roost, near): the flock exploding off Ellis, a sharp breath drawn in, Hob's
		 *  single dry croak. */
		inline constexpr const TCHAR* DeathWake = TEXT("World.Roost.DeathWake");
	}

	// --- Ember powers (2026-10-15: Embers/Powers/*, UEmberComponent, the HUD's socket; Art/Sounds/recipes/embers.py). The
	//     player's own, so 2D: Ellis's three (Dust Devil, Sundown, Borrowed Time) and the socket's two ---
	namespace Ember
	{
		/** Dust Devil's dash (2D, ~0.8 s): a gust ripped past both ears from behind, a low whump of air shoved aside, dry
		 *  grit hissing up off the ground and a little whirlwind spinning away into the distance as it settles. */
		inline constexpr const TCHAR* DustDevilUse = TEXT("Ember.DustDevil.Use");
		/** Sundown falling (2D, ~1.6 s): a deep tape-stop groan as the world's sound winds down, a low bell struck once and
		 *  bending flat, a warm swelling drone under it, the heartbeat-slow thud at its end. */
		inline constexpr const TCHAR* SundownStart = TEXT("Ember.Sundown.Start");
		/** Sundown lifting (2D, ~1 s): the groan run backwards, winding up to speed, a bright shimmer as time catches up. */
		inline constexpr const TCHAR* SundownEnd = TEXT("Ember.Sundown.End");
		/** Borrowed Time opened (2D, ~1.3 s): a ledger's heavy cover thrown open, a nib scratching a line, a deep clock tick and
		 *  a cold breathy chord swelling behind it. */
		inline constexpr const TCHAR* BorrowedTimeStart = TEXT("Ember.BorrowedTime.Start");
		/** Borrowed Time's debt called in (2D, ~1.4 s): the ledger slammed shut, the clock's tick turned slow and heavy, a low
		 *  chord sinking flat as the debt is paid in blood. */
		inline constexpr const TCHAR* BorrowedTimeSettle = TEXT("Ember.BorrowedTime.Settle");
		/** A power off its cooldown (2D, short and quiet): a warm ember catching, a soft bright chime with a breath of air. */
		inline constexpr const TCHAR* Ready = TEXT("Ember.Ready");
		/** The key pressed and nothing happened (2D, short): a dull ember's dry fizzle, a muted wooden click. */
		inline constexpr const TCHAR* Denied = TEXT("Ember.Denied");

		// --- The four new heroes' twelve (2026-10-20: Embers/Powers/*; Art/Sounds/recipes/embers_odessa.py, embers_crane.py,
		//     embers_gauge.py, embers_pike.py). The player's own feedback is 2D; what happens out in the world is placed ---

		// Odessa's: Lucky Streak, Fifty-Two Pickup, Double or Nothing (Fortune, Risk)
		/** Lucky Streak dealt (2D, ~1 s): a deck riffled in one hand, a coin's bright spin and a run of small warm chimes. */
		inline constexpr const TCHAR* LuckyStreakStart = TEXT("Ember.LuckyStreak.Start");
		/** A lucky round leaving the gun, every third shot (2D, short, quiet: it rides on the shot): a gold coin's tiny ring. */
		inline constexpr const TCHAR* LuckyStreakLucky = TEXT("Ember.LuckyStreak.Lucky");
		/** A ricochet off a hit into the next creature (3D, short): a bright glancing whistle and a card's flick. */
		inline constexpr const TCHAR* LuckyStreakRicochet = TEXT("Ember.LuckyStreak.Ricochet");
		/** Fifty-Two Pickup flung (2D, ~0.7 s): the whole deck sprung from the hand, a fast flutter of cards and a fan of air. */
		inline constexpr const TCHAR* FiftyTwoPickupThrow = TEXT("Ember.FiftyTwoPickup.Throw");
		/** Cards slicing into a creature (3D, short): paper whips and the thin cuts of their edges. */
		inline constexpr const TCHAR* FiftyTwoPickupHit = TEXT("Ember.FiftyTwoPickup.Hit");
		/** Double or Nothing called (2D, ~1 s): a coin flicked high, spinning, and slapped down on the back of a glove. */
		inline constexpr const TCHAR* DoubleOrNothingStart = TEXT("Ember.DoubleOrNothing.Start");
		/** A shot that came up double (2D, short, on the shot): a coin's bright ring, heads. */
		inline constexpr const TCHAR* DoubleOrNothingDouble = TEXT("Ember.DoubleOrNothing.Double");
		/** A shot that came up nothing (2D, short, on the shot): a dull coin dropped flat on felt. */
		inline constexpr const TCHAR* DoubleOrNothingNothing = TEXT("Ember.DoubleOrNothing.Nothing");

		// Crane's: Toll Chain, Grave Ward, Last Rites (Debt, Mercy, Judgment)
		/** The toll chain thrown (2D, ~0.9 s): iron links rattling out fast off the shoulder, a whip of air, a far low toll. */
		inline constexpr const TCHAR* TollChainThrow = TEXT("Ember.TollChain.Throw");
		/** The hook biting and the haul (3D, ~0.7 s): a heavy iron bite, the chain snapping taut and rattling home. */
		inline constexpr const TCHAR* TollChainHook = TEXT("Ember.TollChain.Hook");
		/** The lantern planted (3D, ~1.3 s): its iron foot set down, the glass's rattle, the flame catching, a soft warm chord. */
		inline constexpr const TCHAR* GraveWardPlant = TEXT("Ember.GraveWard.Plant");
		/** The lantern burning (3D loop): a soft flame's flutter over a low warm hum. */
		inline constexpr const TCHAR* GraveWardHum = TEXT("Ember.GraveWard.Hum");
		/** The lantern going out (3D, ~0.8 s): the flame snuffed in a puff, the glass's small tink, the hum let go. */
		inline constexpr const TCHAR* GraveWardOut = TEXT("Ember.GraveWard.Out");
		/** A creature marked for its rites (2D, ~1.6 s): one low funeral bell and a whispered chord under it. */
		inline constexpr const TCHAR* LastRitesMark = TEXT("Ember.LastRites.Mark");
		/** The marked one's soul bursting on its pack (3D, ~1.4 s): a choir's breath cut off, a bright bell's crash, souls rushing out. */
		inline constexpr const TCHAR* LastRitesBurst = TEXT("Ember.LastRites.Burst");
		/** The mark striking one that lived (3D, ~1.3 s): a heavy bell's blow, a crack of light, a deep boom rolling off. */
		inline constexpr const TCHAR* LastRitesStrike = TEXT("Ember.LastRites.Strike");

		// Gauge's: Slag Bomb, Iron Hide, Rail Spike (Forge, Steam)
		/** The bomb lobbed (2D, ~0.8 s): the firebox door's clank, a roar of its draught, the heavy lob and its sizzle going. */
		inline constexpr const TCHAR* SlagBombThrow = TEXT("Ember.SlagBomb.Throw");
		/** The bomb bursting (3D, ~1.5 s): a heavy thump, molten slag spraying and sizzling, iron bits clanging down. */
		inline constexpr const TCHAR* SlagBombBurst = TEXT("Ember.SlagBomb.Burst");
		/** The slag pool (3D loop): thick bubbling, popping and hissing as it cools. */
		inline constexpr const TCHAR* SlagBombPool = TEXT("Ember.SlagBomb.Pool");
		/** Slag searing a creature, each burn (3D, short): a hot hiss and a pop. */
		inline constexpr const TCHAR* SlagBombSear = TEXT("Ember.SlagBomb.Sear");
		/** Iron Hide's plates locking (2D, ~1 s): plates slammed shut one after another, bolts ratcheting home, steam hissing out. */
		inline constexpr const TCHAR* IronHideLock = TEXT("Ember.IronHide.Lock");
		/** A blow glancing off the locked plates (2D, short): a heavy plate's dull ring. */
		inline constexpr const TCHAR* IronHideClang = TEXT("Ember.IronHide.Clang");
		/** A biter's teeth on the hot plates (3D, short): a sizzle and a clank. */
		inline constexpr const TCHAR* IronHideBurn = TEXT("Ember.IronHide.Burn");
		/** The plates opening again (2D, ~0.8 s): steam let out in a long sigh, plates sliding back. */
		inline constexpr const TCHAR* IronHideUnlock = TEXT("Ember.IronHide.Unlock");
		/** The rail spike driven (2D, ~1 s): a steam piston's bang, the spike's ring as it leaves, a long hiss after. */
		inline constexpr const TCHAR* RailSpikeFire = TEXT("Ember.RailSpike.Fire");
		/** The spike through a creature (3D, short): a heavy iron punch and a crunch. */
		inline constexpr const TCHAR* RailSpikeHit = TEXT("Ember.RailSpike.Hit");
		/** The spike driven into the world at its end (3D, ~0.8 s): iron biting into rock and ringing. */
		inline constexpr const TCHAR* RailSpikeEmbed = TEXT("Ember.RailSpike.Embed");

		// Pike's: Spyglass, Tripwire, Long Shot (Sight, Craft)
		/** The spyglass drawn out (2D, ~1.2 s): brass tubes sliding out and clicking home, a lens's glint and a sweep of air. */
		inline constexpr const TCHAR* SpyglassStart = TEXT("Ember.Spyglass.Start");
		/** A creature found as the sweep passes it (2D, short, quiet): a small brass tick and a high soft ping. */
		inline constexpr const TCHAR* SpyglassMark = TEXT("Ember.Spyglass.Mark");
		/** The spyglass shut (2D, short): the tubes pushed home. */
		inline constexpr const TCHAR* SpyglassEnd = TEXT("Ember.Spyglass.End");
		/** The wire strung (2D, ~1.1 s): two stakes knocked in, the wire drawn taut and humming. */
		inline constexpr const TCHAR* TripwireSet = TEXT("Ember.Tripwire.Set");
		/** A creature caught by the wire (3D, ~0.6 s): the wire's twang and a body hitting the ground. */
		inline constexpr const TCHAR* TripwireTrip = TEXT("Ember.Tripwire.Trip");
		/** The wire let go (3D, ~0.5 s): a falling twang as it goes slack. */
		inline constexpr const TCHAR* TripwireSlack = TEXT("Ember.Tripwire.Slack");
		/** Long Shot steadied (2D, ~1.3 s): a breath drawn and held, a scope's ring turned in clicks, a far high tone. */
		inline constexpr const TCHAR* LongShotStart = TEXT("Ember.LongShot.Start");
		/** A long shot landing (2D, short, on every hit: quiet): a distant bright tink, pitched up the farther it flew. */
		inline constexpr const TCHAR* LongShotHit = TEXT("Ember.LongShot.Hit");
	}
}
