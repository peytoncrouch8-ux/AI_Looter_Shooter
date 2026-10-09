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
	/** Her venom gurgling up as she rears, and spat. */
	inline constexpr const TCHAR* GravemotherGurgle = TEXT("Boss.Gravemother.Gurgle");
	inline constexpr const TCHAR* GravemotherSpit = TEXT("Boss.Gravemother.Spit");
	/** Her fury's burning crack searing whoever stands on it. */
	inline constexpr const TCHAR* GravemotherCrackBurn = TEXT("Boss.Gravemother.CrackBurn");
	inline constexpr const TCHAR* GravemotherDeath = TEXT("Boss.Gravemother.Death");
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
}
