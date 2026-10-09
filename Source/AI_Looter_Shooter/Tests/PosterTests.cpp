#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Interaction/InteractionComponent.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Missions/MissionLastingInteractObjective.h"
#include "Missions/MissionRunner.h"
#include "Missions/MissionSubsystem.h"
#include "Session/CampaignRecord.h"
#include "Session/SessionSave.h"
#include "Session/SessionSubsystem.h"
#include "Story/CaptionSubsystem.h"
#include "Tests/InteractionTestWorld.h"
#include "Tests/MissionTestWorld.h"
#include "World/WantedPoster.h"
#include "Components/BoxComponent.h"
#include "Components/DecalComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/PackageName.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

namespace
{
	/**
	 * A paper on a wall at Where, facing Yaw (180: back toward a stand-in at the origin who looks along +X), begun as play
	 * begins it (it joins the level's interactables). A name, as a level's placed posters have one, when given.
	 */
	AWantedPoster* SpawnPoster(UWorld* World, const FVector& Where, EWantedPosterVariant Variant = EWantedPosterVariant::Wanted,
		FName Name = NAME_None, double Yaw = 180.0)
	{
		FActorSpawnParameters Params;
		Params.Name = Name;
		AWantedPoster* Poster = World->SpawnActor<AWantedPoster>(Where, FRotator(0.0, Yaw, 0.0), Params);
		if (!Poster)
		{
			return nullptr;
		}
		if (Variant != EWantedPosterVariant::Wanted)
		{
			Poster->SetVariant(Variant);
		}
		Poster->DispatchBeginPlay();
		return Poster;
	}

	/** The first objective's progress of a running mission ("2 / 6"), or "not running". */
	FString ProgressOf(const UMissionRunner& Runner, FName MissionId)
	{
		const TArray<FMissionObjectiveView> Views = Runner.GetObjectiveViews(MissionId);
		return Views.IsEmpty() ? FString(TEXT("not running")) : Views[0].Progress;
	}

	/** The cell its decal's material shows, when the material is made (build_decal_materials.py); unset before. */
	TOptional<FLinearColor> MaterialCell(const AWantedPoster& Poster)
	{
		const UMaterialInstanceDynamic* Material = Cast<UMaterialInstanceDynamic>(Poster.Decal->GetDecalMaterial());
		FLinearColor Cell;
		if (Material && Material->GetVectorParameterValue(FHashedMaterialParameterInfo(TEXT("Cell")), Cell))
		{
			return Cell;
		}
		return TOptional<FLinearColor>();
	}

	bool SameCell(const FLinearColor& Shown, const FVector4& Cell)
	{
		return FMath::IsNearlyEqual(Shown.R, Cell.X, 1e-4) && FMath::IsNearlyEqual(Shown.G, Cell.Y, 1e-4)
			&& FMath::IsNearlyEqual(Shown.B, Cell.Z, 1e-4) && FMath::IsNearlyEqual(Shown.A, Cell.W, 1e-4);
	}

	/** Through the save format and back, read as the game reads a session; null when it failed. */
	ULooterSessionSave* WriteAndRead(ULooterSessionSave* Save)
	{
		TArray<uint8> Bytes;
		return UGameplayStatics::SaveGameToMemory(Save, Bytes) ? USessionSubsystem::ReadSave(Bytes) : nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPosterTearTest, "Looter.Poster.Tear",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPosterTearTest::RunTest(const FString& Parameters)
{
	// A wanted poster: the atlas's poster upright on its wall, its box for the Interact key's line only; torn down, the
	// remnant in its place, once only, and a scrap that tumbles down, fades and is gone after its 1.5 s.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UInteractionComponent* Interaction = nullptr;
	APawn* Player = InteractionTestWorld::SpawnPlayer(World, Interaction);
	AWantedPoster* Poster = SpawnPoster(World, InteractionTestWorld::Ahead(150.0));
	if (!TestTrue(TEXT("Stand-in and poster placed"), Player && Interaction && Poster))
	{
		return false;
	}
	const FWantedPosterAtlas& Atlas = Poster->Atlas;
	TestTrue(TEXT("Tagged as a wanted poster for missions"), Poster->ActorHasTag(AWantedPoster::WantedTag)
		&& !Poster->ActorHasTag(AWantedPoster::NoteTag));
	TestTrue(TEXT("It shows the whole poster"), Poster->GetShownCell() == Atlas.Poster);
	if (const TOptional<FLinearColor> Shown = MaterialCell(*Poster))
	{
		TestTrue(TEXT("Its material shows the poster's cell"), SameCell(*Shown, Atlas.Poster));
	}

	// At the art's size, upright and unmirrored for whoever faces it: the decal projects into the wall, the picture's U
	// (the decal's Z) runs to the reader's right and its V (the decal's Y) runs down.
	const UDecalComponent* Decal = Poster->Decal;
	TestTrue(TEXT("0.5 x 0.703 m"), FMath::IsNearlyEqual(Decal->DecalSize.Z, 25.0, 0.01) && FMath::IsNearlyEqual(Decal->DecalSize.Y, 35.15625, 0.01));
	TestTrue(TEXT("It projects into the wall"), Decal->GetForwardVector().Equals(-Poster->GetActorForwardVector(), 1e-3));
	TestTrue(TEXT("U runs to the reader's right"), Decal->GetUpVector().Equals(-Poster->GetActorRightVector(), 1e-3));
	TestTrue(TEXT("V runs down"), Decal->GetRightVector().Equals(-FVector::UpVector, 1e-3));
	const FVector DecalSpot = Decal->GetRelativeLocation();
	const FVector DecalSize = Decal->DecalSize;

	// The box: found by the Interact key's line (Visibility) and nothing else.
	const UBoxComponent* Box = Poster->Hitbox;
	TestTrue(TEXT("The Interact key's line finds its box"), Box->GetCollisionEnabled() == ECollisionEnabled::QueryOnly
		&& Box->GetCollisionResponseToChannel(ECC_Visibility) == ECR_Block);
	TestTrue(TEXT("No player, bullet or camera meets it"), Box->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Ignore
		&& Box->GetCollisionResponseToChannel(ECC_GameTraceChannel2) == ECR_Ignore && Box->GetCollisionResponseToChannel(ECC_Camera) == ECR_Ignore);
	TestTrue(TEXT("World dynamic: the minimap's bake never sees it"), Box->GetCollisionObjectType() == ECC_WorldDynamic);
	TestFalse(TEXT("Up, it's not used up"), Poster->IsUsedUp());

	// Torn down: the remnant in the same size and spot, once only, its box gone.
	UCaptionSubsystem* Captions = UCaptionSubsystem::Get(World);
	TestTrue(TEXT("Torn down"), Poster->Tear(Player));
	TestTrue(TEXT("Down, and used up for the missions"), Poster->IsTorn() && Poster->IsUsedUp());
	TestTrue(TEXT("It shows the remnant"), Poster->GetShownCell() == Atlas.Remnant);
	if (const TOptional<FLinearColor> Shown = MaterialCell(*Poster))
	{
		TestTrue(TEXT("Its material shows the remnant's cell"), SameCell(*Shown, Atlas.Remnant));
	}
	TestTrue(TEXT("In the poster's place"), Decal->GetRelativeLocation().Equals(DecalSpot, 0.01) && Decal->DecalSize.Equals(DecalSize, 0.01));
	TestFalse(TEXT("It can't be torn twice"), Poster->Tear(Player));
	TestFalse(TEXT("Nothing left to use"), Poster->GetInteractionOptions(*Interaction).bUsable);
	TestTrue(TEXT("Its box is gone"), Box->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
	TestEqual(TEXT("One of the level's posters is down"), AWantedPoster::CountTorn(World), 1);
	const FCaptionEntry* Said = Captions ? Captions->GetCurrent() : nullptr;
	TestTrue(TEXT("Hob remarks on the first one down"), Said && Said->Line.Speaker.ToString() == TEXT("Hob"));

	// The scrap: a card with no collision, shadow or decal on it, tumbling down and fading, gone after ScrapSeconds.
	UStaticMeshComponent* Scrap = Poster->GetScrap();
	if (!TestNotNull(TEXT("A scrap tears off"), Scrap))
	{
		return false;
	}
	TestTrue(TEXT("Nothing meets it, it casts no shadow and takes no decal"), Scrap->GetCollisionEnabled() == ECollisionEnabled::NoCollision
		&& !Scrap->CastShadow && !Scrap->bReceivesDecals);
	TestTrue(TEXT("Solid as it comes off"), FMath::IsNearlyEqual(Poster->GetScrapOpacity(), 1.f));
	const FVector Start = Scrap->GetRelativeLocation();
	Poster->Advance(0.75f);
	TestTrue(TEXT("Halfway: well down and out from the wall"), Scrap->GetRelativeLocation().Z < Start.Z - 40.0
		&& Scrap->GetRelativeLocation().X > Start.X);
	const float Halfway = Poster->GetScrapOpacity();
	TestTrue(TEXT("Halfway: fading"), Halfway > 0.05f && Halfway < 0.95f);
	const TArray<float>& Data = Scrap->GetCustomPrimitiveData().Data;
	TestTrue(TEXT("Its material hears the fade"), Data.Num() > 8 && FMath::IsNearlyEqual(Data[8], Halfway, 1e-4f));
	Poster->Advance(0.5f);
	TestNotNull(TEXT("Still falling at 1.25 s"), Poster->GetScrap());
	Poster->Advance(0.3f);
	TestNull(TEXT("Gone after 1.5 s"), Poster->GetScrap());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPosterHoldTest, "Looter.Poster.Hold",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPosterHoldTest::RunTest(const FString& Parameters)
{
	// Through the Interact key: a poster takes only a hold of 0.6 s; a tap or letting go early leaves it up.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UInteractionComponent* Interaction = nullptr;
	APawn* Player = InteractionTestWorld::SpawnPlayer(World, Interaction);
	AWantedPoster* Poster = SpawnPoster(World, InteractionTestWorld::Ahead(150.0));
	if (!TestTrue(TEXT("Stand-in and poster placed"), Player && Interaction && Poster))
	{
		return false;
	}
	Interaction->UpdateInteraction(0.f);
	TestTrue(TEXT("The poster in front is focused"), Interaction->GetFocusedActor() == Poster);
	const FInteractionOptions Options = Interaction->GetFocusedOptions();
	TestTrue(TEXT("Only a hold"), Options.bHold && !Options.bTap);
	TestTrue(TEXT("A short hold: 0.6 s"), FMath::IsNearlyEqual(Options.HoldSeconds, 0.6f) && FMath::IsNearlyEqual(Poster->TearHoldSeconds, 0.6f));
	TestEqual(TEXT("Its words"), Options.HoldPrompt.ToString(), FString(TEXT("Tear down the poster")));

	Interaction->PressInteract();
	Interaction->ReleaseInteract();
	TestFalse(TEXT("A tap leaves it up"), Poster->IsTorn());

	Interaction->PressInteract();
	Interaction->UpdateInteraction(0.3f);
	TestFalse(TEXT("Half its time: still up"), Poster->IsTorn());
	TestTrue(TEXT("The bar half full"), FMath::IsNearlyEqual(Interaction->GetHoldProgress(), 0.5f, 0.01f));
	Interaction->ReleaseInteract();
	TestFalse(TEXT("Let go early: still up"), Poster->IsTorn());

	Interaction->PressInteract();
	Interaction->UpdateInteraction(0.35f);
	TestFalse(TEXT("Not before its time"), Poster->IsTorn());
	Interaction->UpdateInteraction(0.3f);
	TestTrue(TEXT("Held its 0.6 s: torn down"), Poster->IsTorn());
	Interaction->ReleaseInteract();
	Interaction->UpdateInteraction(0.f);
	TestNull(TEXT("Down, it's no longer offered"), Interaction->GetFocusedActor());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPosterCalderNoteTest, "Looter.Poster.CalderNote",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPosterCalderNoteTest::RunTest(const FString& Parameters)
{
	// Calder's note: the note's cell at its size, never torn (by a hold, a script or a save), read by a tap as captions
	// (her words, then Hob's), and not again while they play.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UInteractionComponent* Interaction = nullptr;
	APawn* Player = InteractionTestWorld::SpawnPlayer(World, Interaction);
	AWantedPoster* Note = SpawnPoster(World, InteractionTestWorld::Ahead(150.0), EWantedPosterVariant::CalderNote);
	UCaptionSubsystem* Captions = UCaptionSubsystem::Get(World);
	if (!TestTrue(TEXT("Stand-in, note and captions"), Player && Interaction && Note && Captions))
	{
		return false;
	}
	TestTrue(TEXT("Tagged as Calder's note"), Note->ActorHasTag(AWantedPoster::NoteTag) && !Note->ActorHasTag(AWantedPoster::WantedTag));
	TestTrue(TEXT("It shows the note"), Note->GetShownCell() == Note->Atlas.Note);
	TestTrue(TEXT("0.25 x 0.297 m"), FMath::IsNearlyEqual(Note->Decal->DecalSize.Z, 12.5, 0.01)
		&& FMath::IsNearlyEqual(Note->Decal->DecalSize.Y, 14.84375, 0.01));

	Interaction->UpdateInteraction(0.f);
	TestTrue(TEXT("Focused"), Interaction->GetFocusedActor() == Note);
	const FInteractionOptions Options = Interaction->GetFocusedOptions();
	TestTrue(TEXT("A tap reads it"), Options.bTap && !Options.bHold);
	TestEqual(TEXT("Its words"), Options.TapPrompt.ToString(), FString(TEXT("Read the note")));

	TestFalse(TEXT("Never torn"), Note->Tear(Player) || Note->IsTorn() || Note->IsUsedUp());
	Note->RestoreTorn();
	TestFalse(TEXT("Not even by a save"), Note->IsTorn());

	// Read by the key: the lines play, hers first, Hob's last.
	Interaction->PressInteract();
	const FCaptionEntry* Said = Captions->GetCurrent();
	TestTrue(TEXT("Read: her first line on screen"), Said && !Note->NoteLines.IsEmpty() && Said->Line.Text.EqualTo(Note->NoteLines[0].Text));
	TestTrue(TEXT("Hob has the last word"), !Note->NoteLines.IsEmpty() && Note->NoteLines.Last().Speaker.ToString() == TEXT("Hob"));
	Interaction->ReleaseInteract();
	TestTrue(TEXT("Being read"), Note->IsBeingRead());
	TestFalse(TEXT("Not again while its lines play"), Note->CanRead() || Note->Read(Player));

	// Its lines over, it can be read again.
	Captions->Update(120.f);
	Captions->Update(120.f);
	TestTrue(TEXT("Read again once they're over"), Note->CanRead() && Note->Read(Player));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPosterMissionTest, "Looter.Poster.Mission",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPosterMissionTest::RunTest(const FString& Parameters)
{
	// Side 1 as create_side_mission_assets.py makes it: after Main 3, tear down 6 wanted posters (counted from the world:
	// torn ones before it opened, held uses heard at once, ones torn by the console seen on the next look, and none lost
	// when it starts over after a reload), then read Calder's note on the board.
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	UMissionSubsystem* Display = World->GetSubsystem<UMissionSubsystem>();
	if (!TestTrue(TEXT("The level has a mission runner and its display"), Runner && Display))
	{
		return false;
	}
	const FName Rest(TEXT("TestRest"));
	const FName MainId(TEXT("TestMain3"));
	const FName SideId(TEXT("TestPosters"));
	UPackage* Scratch = CreatePackage(nullptr);
	UMissionDefinition* Main = MissionTestWorld::NewMission(Scratch, TEXT("TestMain3"), EMissionKind::Main, EMissionStart::Manual, Rest);
	MissionTestWorld::AddObjective<UMissionEventObjective>(Main, 0)->Event = TEXT("Test.Never");
	UMissionDefinition* Side = MissionTestWorld::NewMission(Scratch, TEXT("TestPosters"), EMissionKind::Side, EMissionStart::Automatic, Rest);
	Side->Prerequisites = { MainId };
	UMissionLastingInteractObjective* TearDown = MissionTestWorld::AddObjective<UMissionLastingInteractObjective>(Side, 0);
	TearDown->Target.ActorClass = AWantedPoster::StaticClass();
	TearDown->Target.ActorTag = AWantedPoster::WantedTag;
	TearDown->Count = 6;
	TearDown->bHold = true;
	UMissionInteractObjective* ReadNote = MissionTestWorld::AddObjective<UMissionInteractObjective>(Side, 1);
	ReadNote->Target.ActorClass = AWantedPoster::StaticClass();
	ReadNote->Target.ActorTag = AWantedPoster::NoteTag;
	// Turned in to Tilly at her window (create_side_mission_assets.py), for 35 experience.
	const FName TillyTag(TEXT("Speaker_Tilly"));
	Side->TurnIn.SpeakerTag = TillyTag;
	Side->TurnIn.GiverName = FText::FromString(TEXT("Tilly"));
	Side->Rewards.Experience = 35;

	// Seven posters in a row ahead of the player, so one can be missed; Calder's note off to the side.
	AActor* Player = MissionTestWorld::SpawnMarker(World, FVector::ZeroVector);
	TArray<AWantedPoster*> Posters;
	for (int32 Index = 0; Index < 7; ++Index)
	{
		Posters.Add(SpawnPoster(World, FVector(1000.0 + 300.0 * Index, 0.0, 150.0)));
	}
	AWantedPoster* Note = SpawnPoster(World, FVector(0.0, 1000.0, 150.0), EWantedPosterVariant::CalderNote);
	AActor* Tilly = MissionTestWorld::SpawnMarker(World, FVector(-1500.0, 0.0, 150.0), TillyTag);
	if (!TestTrue(TEXT("Player, posters, note and Tilly placed"), Player && Note && Tilly && !Posters.Contains(nullptr)))
	{
		return false;
	}

	Runner->BeginForTesting({ Main, Side }, Campaign, Player, Rest);
	Runner->Update(0.f);
	TestFalse(TEXT("Shut until Main 3 is finished"), Runner->IsRunning(SideId));
	Posters[0]->Tear(nullptr);
	Posters[1]->Tear(nullptr);
	TestTrue(TEXT("Main 3 finished"), Runner->CompleteMission(MainId));
	TestTrue(TEXT("It opens after Main 3"), Runner->IsRunning(SideId) && Runner->GetStep(SideId) == 0);
	TestEqual(TEXT("The two torn down before count"), ProgressOf(*Runner, SideId), FString(TEXT("2 / 6")));

	// Held by the player: the interaction component's event counts at once, and the same poster counts once.
	Posters[2]->Tear(nullptr);
	Runner->NotifyEvent(FMissionEvent::Interaction(Posters[2], /*bHeld*/ true));
	TestEqual(TEXT("A held tear counts at once"), ProgressOf(*Runner, SideId), FString(TEXT("3 / 6")));
	Runner->NotifyEvent(FMissionEvent::Interaction(Posters[2], /*bHeld*/ true));
	Runner->Update(UMissionRunner::UpdateInterval);
	TestEqual(TEXT("The same poster counts once"), ProgressOf(*Runner, SideId), FString(TEXT("3 / 6")));
	Runner->NotifyEvent(FMissionEvent::Interaction(Posters[3], /*bHeld*/ false));
	TestEqual(TEXT("A tap on one still up doesn't count"), ProgressOf(*Runner, SideId), FString(TEXT("3 / 6")));

	// Torn with no event (the console): the next look counts it.
	Posters[3]->Tear(nullptr);
	Runner->Update(UMissionRunner::UpdateInterval);
	TestEqual(TEXT("Torn by the console: counted on the next look"), ProgressOf(*Runner, SideId), FString(TEXT("4 / 6")));

	// A reload starts a side mission over from its first step: the world kept the posters down, so nothing is lost.
	TestTrue(TEXT("Started over"), Runner->StartMission(SideId, 0, /*bForce*/ true));
	TestEqual(TEXT("Still 4 of 6 after starting over"), ProgressOf(*Runner, SideId), FString(TEXT("4 / 6")));
	const FMission* Shown = Display->GetTracked();
	TestTrue(TEXT("The arrow points to the nearest poster still up"), Shown && Shown->Waypoint.IsSet()
		&& Shown->Waypoint->Equals(Posters[4]->GetActorLocation(), 1.0));

	Posters[4]->Tear(nullptr);
	Posters[5]->Tear(nullptr);
	Runner->Update(UMissionRunner::UpdateInterval);
	TestTrue(TEXT("Six down: on to the notice board"), Runner->GetStep(SideId) == 1);
	Shown = Display->GetTracked();
	TestTrue(TEXT("The arrow points to Calder's note"), Shown && Shown->Waypoint.IsSet() && Shown->Waypoint->Equals(Note->GetActorLocation(), 1.0));
	Posters[6]->Tear(nullptr);
	Runner->NotifyEvent(FMissionEvent::Interaction(Posters[6], /*bHeld*/ true));
	TestTrue(TEXT("The seventh poster isn't the note"), Runner->IsRunning(SideId) && Runner->GetStep(SideId) == 1);
	Runner->NotifyEvent(FMissionEvent::Interaction(Note, /*bHeld*/ false));
	TestTrue(TEXT("The note read: ready to turn in to Tilly, not finished"), Runner->IsReadyToTurnIn(SideId) && !Campaign.HasCompleted(SideId));
	Shown = Display->GetTracked();
	TestTrue(TEXT("...the arrow on Tilly's window"), Shown && Shown->Tracker.bTurnIn && Shown->Waypoint.IsSet()
		&& Shown->Waypoint->Equals(Tilly->GetActorLocation(), 1.0));
	Runner->NotifyEvent(FMissionEvent::Talked(Tilly));
	TestFalse(TEXT("Turned in to her: finished"), Runner->IsRunning(SideId));
	TestTrue(TEXT("Recorded in the campaign"), Campaign.HasCompleted(SideId));

	// The mission asset, once create_side_mission_assets.py has made it, asks for the same.
	const TCHAR* AssetPath = TEXT("/Game/Data/Missions/DA_Mission_Side1.DA_Mission_Side1");
	const UMissionDefinition* Asset = FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(AssetPath)))
		? LoadObject<UMissionDefinition>(nullptr, AssetPath) : nullptr;
	if (!Asset)
	{
		AddInfo(TEXT("DA_Mission_Side1 isn't made yet (Tools/Unreal/create_side_mission_assets.py): only the rules were checked."));
		return true;
	}
	const UMissionLastingInteractObjective* AssetTear = Cast<UMissionLastingInteractObjective>(Asset->GetObjective(0, 0));
	const UMissionInteractObjective* AssetRead = Cast<UMissionInteractObjective>(Asset->GetObjective(1, 0));
	TestTrue(TEXT("Side 1 is a side mission after Main 3 on Ransom's Rest"), Asset->Kind == EMissionKind::Side
		&& Asset->Prerequisites.Contains(FName(TEXT("Main3"))) && Asset->Area == FName(TEXT("RansomsRest")) && Asset->Steps.Num() == 2);
	TestTrue(TEXT("Its first step: hold to tear down 6 wanted posters, counted from the world"), AssetTear && AssetTear->Count == 6
		&& AssetTear->bHold && AssetTear->Target.ActorTag == AWantedPoster::WantedTag);
	TestTrue(TEXT("Its second: read Calder's note"), AssetRead && !AssetRead->IsA<UMissionLastingInteractObjective>()
		&& AssetRead->Target.ActorTag == AWantedPoster::NoteTag);
	TestTrue(TEXT("Turned in to Tilly, with words of her own for it"), Asset->NeedsTurnIn() && Asset->TurnIn.SpeakerTag == TillyTag
		&& !Asset->TurnIn.Lines.IsEmpty());
	TestTrue(TEXT("35 experience and a Rare gun"), Asset->Rewards.Experience == 35 && Asset->Rewards.bGun
		&& Asset->Rewards.GunRarityFloor == EWeaponRarity::Rare);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPosterSaveTest, "Looter.Poster.Save",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPosterSaveTest::RunTest(const FString& Parameters)
{
	// Torn posters stay down: the session keeps them by name with the map's world, through the save file, and puts them
	// back down (quietly) when the level is played again; the others stay up.
	FTestWorldWrapper PlayedLevel;
	FTestWorldWrapper AgainLevel;
	if (!TestTrue(TEXT("Test levels made"), PlayedLevel.CreateTestWorld(EWorldType::EditorPreview) && AgainLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* Played = PlayedLevel.GetTestWorld();
	UWorld* Again = AgainLevel.GetTestWorld();
	const FName SheriffName(TEXT("Poster_Sheriff"));
	const FName SaloonName(TEXT("Poster_Saloon"));
	const FName NoteName(TEXT("Poster_CalderNote"));
	AWantedPoster* Sheriff = SpawnPoster(Played, FVector(500.0, 0.0, 150.0), EWantedPosterVariant::Wanted, SheriffName);
	AWantedPoster* Saloon = SpawnPoster(Played, FVector(500.0, 300.0, 150.0), EWantedPosterVariant::Wanted, SaloonName);
	AWantedPoster* Note = SpawnPoster(Played, FVector(500.0, 600.0, 150.0), EWantedPosterVariant::CalderNote, NoteName);
	if (!TestTrue(TEXT("The level's posters"), Sheriff && Saloon && Note))
	{
		return false;
	}
	Sheriff->Tear(nullptr);
	Note->Read(nullptr);

	ULooterSessionSave* Save = NewObject<ULooterSessionSave>();
	Save->Version = ULooterSessionSave::CurrentVersion;
	USessionSubsystem::CaptureWorld(Played, *Save);
	const FString PlayedMap = USessionSubsystem::MapOf(Played);
	const FSavedMapWorld* Kept = Save->FindWorld(PlayedMap);
	TestTrue(TEXT("The torn one is kept by name, and only it"), Kept && Kept->TornPosters.Num() == 1 && Kept->TornPosters[0] == SheriffName);
	ULooterSessionSave* Back = WriteAndRead(Save);
	const FSavedMapWorld* Read = Back ? Back->FindWorld(PlayedMap) : nullptr;
	if (!TestTrue(TEXT("Through the save file"), Read && Read->TornPosters.Contains(SheriffName)))
	{
		return false;
	}

	// The level played again: its posters as built, all up; the session puts back what was torn. (The second test level
	// stands in for the same map, so its world is filed under that map's name.)
	AWantedPoster* SheriffAgain = SpawnPoster(Again, FVector(500.0, 0.0, 150.0), EWantedPosterVariant::Wanted, SheriffName);
	AWantedPoster* SaloonAgain = SpawnPoster(Again, FVector(500.0, 300.0, 150.0), EWantedPosterVariant::Wanted, SaloonName);
	AWantedPoster* NoteAgain = SpawnPoster(Again, FVector(500.0, 600.0, 150.0), EWantedPosterVariant::CalderNote, NoteName);
	if (!TestTrue(TEXT("The level's posters again"), SheriffAgain && SaloonAgain && NoteAgain && !SheriffAgain->IsTorn()))
	{
		return false;
	}
	// Copied first: adding the second map's entry may move the first's.
	const FSavedMapWorld PlayedWorld = *Read;
	Back->FindOrAddWorld(USessionSubsystem::MapOf(Again)) = PlayedWorld;
	USessionSubsystem::RestoreWorld(Again, *Back);
	TestTrue(TEXT("Torn again"), SheriffAgain->IsTorn() && SheriffAgain->GetShownCell() == SheriffAgain->Atlas.Remnant);
	TestNull(TEXT("Quietly: no scrap falls"), SheriffAgain->GetScrap());
	TestFalse(TEXT("The other still up"), SaloonAgain->IsTorn());
	TestFalse(TEXT("Calder's note is never down"), NoteAgain->IsTorn());
	TestEqual(TEXT("One down in the level"), AWantedPoster::CountTorn(Again), 1);
	return true;
}

#endif
