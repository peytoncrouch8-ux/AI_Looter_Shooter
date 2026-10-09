// AChest's kinds: each one's models, how it opens, what it gives, its words (Art/Models/Loot/Chests.py for the caches and
// the Strongbox; Art/Models/Props/Lootables.py for the coffins, graves, mailboxes and footlockers).

#include "Loot/Chest.h"
#include "Audio/LooterSoundCues.h"

namespace
{
	constexpr EChestKind Kinds[] = { EChestKind::SupplyCrate, EChestKind::Strongbox, EChestKind::Coffin, EChestKind::Grave,
		EChestKind::Mailbox, EChestKind::Footlocker };
}

TConstArrayView<EChestKind> FChestKindInfo::All()
{
	return MakeArrayView(Kinds);
}

FChestKindInfo FChestKindInfo::Get(EChestKind Kind)
{
	FChestKindInfo Info;
	switch (Kind)
	{
	case EChestKind::Strongbox:
		// Chests.py strongbox(): 0.82 x 0.6 m on 4.5 cm feet, 0.445 m to the lid; the hinge 3 cm behind its back.
		Info.BodyPath = TEXT("/Game/Art/Loot/SM_Strongbox.SM_Strongbox");
		Info.LidPath = TEXT("/Game/Art/Loot/SM_Strongbox_Lid.SM_Strongbox_Lid");
		Info.WheelPath = TEXT("/Game/Art/Loot/SM_Strongbox_Wheel.SM_Strongbox_Wheel");
		Info.OpenAngle = 102.f;
		Info.PreMotion = EChestPreMotion::Wheel;
		Info.WheelTurns = 1.5f;
		Info.Guns = 2;
		Info.Luck = 1.f;
		Info.AmmoPickups = 2;
		Info.Hinge = FVector(-33.0, 0.0, 44.5);
		Info.LootPoint = FVector(0.0, 0.0, 13.5);
		Info.WheelHub = FVector(31.2, 0.0, 24.5);
		Info.Height = 44.5f;
		Info.PreCue = LooterSoundCue::StrongboxWheel;
		Info.OpenCue = LooterSoundCue::ChestOpen;
		Info.bOnMap = true;
		break;
	case EChestKind::Coffin:
		// Lootables.py loot_coffin(): Graves.py's toe-pincher (1.94 m along its front, 0.4 m to the lid) with its lid loose
		// on top (SOCKET_Lid at the top's middle). Pried at on its nails, then shoved back off the box to lean on it.
		Info.BodyPath = TEXT("/Game/Art/Props/SM_LootCoffin.SM_LootCoffin");
		Info.LidPath = TEXT("/Game/Art/Props/SM_LootCoffin_Lid.SM_LootCoffin_Lid");
		Info.LidMotion = EChestLidMotion::Slide;
		Info.SlideOffset = FVector(-42.0, 0.0, -8.0);
		Info.SlideTurn = FRotator(28.f, 6.f, 0.f);
		Info.SlideLift = 10.f;
		Info.OpenAngle = 28.f;
		Info.LidSeconds = 0.5f;
		Info.PreMotion = EChestPreMotion::Pry;
		Info.PreSeconds = 0.55f;
		Info.HoldSeconds = 0.7f;
		Info.Guns = 1;
		Info.GunChance = 0.15f;
		Info.Luck = 0.2f;
		Info.AmmoPickups = 1;
		Info.MoteChance = 0.2f;
		Info.Hinge = FVector(0.0, 0.0, 40.0);
		Info.LootPoint = FVector(0.0, 0.0, 14.0);
		Info.Height = 40.f;
		Info.PreCue = LooterSoundCue::Lootables::CoffinPry;
		Info.OpenCue = LooterSoundCue::Lootables::CoffinLidOff;
		break;
	case EChestKind::Grave:
		// Lootables.py loot_grave(): the open grave (spoil heaps either side, the coffin's top in the middle, its pivot the
		// mound's middle, its foot +X, the headboard 1.1 m behind), sunk under the ground until dug; Graves.py's sunken
		// mound over it, sinking away; the spade standing at its foot. Its coffin's lid is shoved onto the right-hand heap.
		Info.BodyPath = TEXT("/Game/Art/Props/SM_LootGrave.SM_LootGrave");
		Info.LidPath = TEXT("/Game/Art/Props/SM_LootGrave_Lid.SM_LootGrave_Lid");
		Info.CoverPath = TEXT("/Game/Art/Props/SM_Grave_MoundSunken.SM_Grave_MoundSunken");
		Info.ShovelPath = TEXT("/Game/Art/Props/SM_LootGrave_Shovel.SM_LootGrave_Shovel");
		Info.LidMotion = EChestLidMotion::Slide;
		Info.SlideOffset = FVector(8.0, 62.0, 14.0);
		Info.SlideTurn = FRotator(0.f, 8.f, 16.f);
		Info.SlideLift = 16.f;
		Info.OpenAngle = 16.f;
		Info.LidSeconds = 0.6f;
		Info.PreMotion = EChestPreMotion::Dig;
		Info.PreSeconds = 1.2f;
		Info.HoldSeconds = 1.f;
		Info.Guns = 1;
		Info.GunChance = 0.12f;
		Info.Luck = 0.2f;
		Info.AmmoPickups = 1;
		Info.MoteChance = 0.3f;
		Info.Hinge = FVector(0.0, 0.0, 12.0);
		Info.LootPoint = FVector(0.0, 0.0, 10.0);
		Info.Height = 25.f;
		Info.InteractPoint = FVector(0.0, 0.0, 30.0);
		Info.BodySink = 45.f;
		Info.CoverSink = 28.f;
		// The spade's blade (its pivot) a hand in the ground at the foot's right corner, its handle leaning out; after,
		// stuck in the left-hand heap.
		Info.ShovelBefore = FTransform(FRotator(-14.f, 30.f, 8.f), FVector(108.0, 46.0, -10.0));
		Info.ShovelAfter = FTransform(FRotator(-10.f, -25.f, -12.f), FVector(18.0, -66.0, 14.0));
		Info.bSolid = false;
		Info.PreCue = LooterSoundCue::Lootables::GraveDig;
		Info.OpenCue = LooterSoundCue::Lootables::CoffinLidOff;
		break;
	case EChestKind::Mailbox:
		// Lootables.py mailbox(): a tin box on a post, its bottom 1.02 m up, its door at the front (+X) hinged along its
		// foot, dropping open forward and down.
		Info.BodyPath = TEXT("/Game/Art/Props/SM_Mailbox.SM_Mailbox");
		Info.LidPath = TEXT("/Game/Art/Props/SM_Mailbox_Door.SM_Mailbox_Door");
		Info.OpenAngle = -100.f;
		Info.LidSeconds = 0.35f;
		Info.Guns = 1;
		Info.GunChance = 0.08f;
		Info.Luck = 0.f;
		Info.AmmoPickups = 1;
		Info.Hinge = FVector(28.0, 0.0, 102.0);
		Info.LootPoint = FVector(8.0, 0.0, 109.0);
		Info.Height = 124.f;
		Info.OpenCue = LooterSoundCue::Lootables::MailboxOpen;
		break;
	case EChestKind::Footlocker:
		// Lootables.py footlocker(): a 0.8 m plank trunk with iron corners and rope handles, 0.4 m to the lid, the hinge on
		// its back top edge.
		Info.BodyPath = TEXT("/Game/Art/Props/SM_Footlocker.SM_Footlocker");
		Info.LidPath = TEXT("/Game/Art/Props/SM_Footlocker_Lid.SM_Footlocker_Lid");
		Info.OpenAngle = 105.f;
		Info.LidSeconds = 0.8f;
		Info.Guns = 1;
		Info.GunChance = 0.35f;
		Info.Luck = 0.25f;
		Info.AmmoPickups = 2;
		Info.Hinge = FVector(-24.0, 0.0, 40.0);
		Info.LootPoint = FVector(0.0, 0.0, 14.0);
		Info.Height = 40.f;
		Info.OpenCue = LooterSoundCue::Lootables::FootlockerOpen;
		break;
	case EChestKind::SupplyCrate:
	default:
		// Chests.py supply_crate(): 1.15 m across its front, 0.52 m deep, 0.40 m to the lid; the hinge just behind its back.
		Info.BodyPath = TEXT("/Game/Art/Loot/SM_SupplyCrate.SM_SupplyCrate");
		Info.LidPath = TEXT("/Game/Art/Loot/SM_SupplyCrate_Lid.SM_SupplyCrate_Lid");
		Info.OpenAngle = 112.f;
		Info.Guns = 1;
		Info.Luck = 0.5f;
		Info.AmmoPickups = 2;
		Info.Hinge = FVector(-28.2, 0.0, 40.0);
		Info.LootPoint = FVector(0.0, 0.0, 24.0);
		Info.Height = 40.f;
		Info.OpenCue = LooterSoundCue::ChestOpen;
		Info.bOnMap = true;
		break;
	}
	return Info;
}

FText AChest::DefaultPrompt(EChestKind ForKind)
{
	switch (ForKind)
	{
	case EChestKind::Strongbox: return NSLOCTEXT("LooterChest", "OpenStrongbox", "Open the strongbox");
	case EChestKind::Coffin: return NSLOCTEXT("LooterChest", "PryCoffin", "Pry the coffin open");
	case EChestKind::Grave: return NSLOCTEXT("LooterChest", "DigGrave", "Dig up the grave");
	case EChestKind::Mailbox: return NSLOCTEXT("LooterChest", "CheckMailbox", "Check the mailbox");
	case EChestKind::Footlocker: return NSLOCTEXT("LooterChest", "OpenFootlocker", "Open the footlocker");
	case EChestKind::SupplyCrate:
	default: return NSLOCTEXT("LooterChest", "OpenCrate", "Open the crate");
	}
}

FText AChest::KindName(EChestKind ForKind)
{
	switch (ForKind)
	{
	case EChestKind::Strongbox: return NSLOCTEXT("LooterChest", "StrongboxName", "Strongbox");
	case EChestKind::Coffin: return NSLOCTEXT("LooterChest", "CoffinName", "Coffin");
	case EChestKind::Grave: return NSLOCTEXT("LooterChest", "GraveName", "Grave");
	case EChestKind::Mailbox: return NSLOCTEXT("LooterChest", "MailboxName", "Mailbox");
	case EChestKind::Footlocker: return NSLOCTEXT("LooterChest", "FootlockerName", "Footlocker");
	case EChestKind::SupplyCrate:
	default: return NSLOCTEXT("LooterChest", "SupplyCrateName", "Supply crate");
	}
}
