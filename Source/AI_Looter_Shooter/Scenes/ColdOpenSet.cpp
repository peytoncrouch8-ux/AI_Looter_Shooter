#include "Scenes/ColdOpenSet.h"
#include "Components/ArrowComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"

namespace
{
	/** A spot and a facing (degrees) on a deck. */
	FTransform Spot(double X, double Y, double Z, double Yaw)
	{
		return FTransform(FRotator(0.0, Yaw, 0.0), FVector(X, Y, Z));
	}
}

AColdOpenSet::AColdOpenSet()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Static);
	RootComponent = Root;

#if WITH_EDITORONLY_DATA
	Arrow = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
	if (Arrow)
	{
		Arrow->SetupAttachment(Root);
		Arrow->SetRelativeLocation(FVector(0.0, 0.0, 700.0));
		Arrow->ArrowColor = FColor(255, 120, 40);
		Arrow->ArrowSize = 3.f;
		Arrow->bTreatAsASprite = true;
	}
#endif

	// Ransom's Rest (Art/Levels/RansomsRest/layout.json; world cm, X north, Y east): a line in from the west-south-west,
	// the sunset's side, ending 30 m out from the Mooring Notch's mouth and 10 m under the lip, the Point's cliffs ahead.
	// The plains stand about 35 m under the valley, the canyon's floor 70 m; its far wall's top is about 350 m out.
	SkiffCourse = {
		FVector(-21660.0, -52240.0, -1500.0),  // in the evening cloud, over the plains
		FVector(-20106.0, -46444.0, -1700.0),  // low over the far wall's top
		FVector(-17775.0, -37750.0, -3800.0),  // dropping into Gravewind Canyon
		FVector(-14667.0, -26158.0, -4600.0),  // over the river
		FVector(-12853.0, -19396.0, -2400.0),  // climbing toward the notch
		FVector(-11300.0, -13600.0, -1000.0),  // the Point's cliffs above, the notch ahead
	};

	// The gang on the skiff's deck (its frame: +X the bow, the Deck socket at (120, 0, 43)), the Deacon at Ellis's
	// shoulder on the port side, the rest along the rails; two look back at the sunset.
	SkiffCrew = {
		Spot(100.0, -72.0, 43.0, 0.0),
		Spot(240.0, 45.0, 43.0, -10.0),
		Spot(10.0, 66.0, 43.0, 15.0),
		Spot(-90.0, -60.0, 43.0, -5.0),
		Spot(-180.0, 52.0, 43.0, 170.0),
		Spot(-270.0, -46.0, 43.0, 5.0),
		Spot(-350.0, 40.0, 43.0, -160.0),
	};

	// The gang on the lookout's deck (6.5 m up): the Deacon by the front rail, the sunset behind him; Lucky Ned at the
	// stair landing's side, turned toward the bluff path; the rest round the deck, facing in.
	Gang = {
		Spot(330.0, 20.0, 650.0, -158.0),
		Spot(60.0, 200.0, 650.0, 158.0),
		Spot(20.0, -150.0, 650.0, 20.0),
		Spot(230.0, -250.0, 650.0, 70.0),
		Spot(390.0, -200.0, 650.0, 110.0),
		Spot(250.0, -140.0, 650.0, 60.0),
		Spot(60.0, -260.0, 650.0, 40.0),
	};

	// The lookout's Sit socket (Art/Models/Buildings/Lookout.py): the front rail's top, left of middle, facing in.
	SextonSeat = Spot(442.5, 148.0, 755.0, 180.0);

	SkiffMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Game/Art/Vehicles/SM_Skiff_A_Gang.SM_Skiff_A_Gang")));
	FigureMesh = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple")));
	RunAnimation = TSoftObjectPtr<UAnimationAsset>(FSoftObjectPath(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Jog/MF_Unarmed_Jog_Fwd.MF_Unarmed_Jog_Fwd")));
	SextonMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Game/Art/Characters/SM_MisterSexton.SM_MisterSexton")));
	SextonLedgerMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Game/Art/Characters/SM_SextonLedger.SM_SextonLedger")));
	SilhouetteMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Art/Materials/Masters/M_Backdrop.M_Backdrop")));
}

AColdOpenSet* AColdOpenSet::Find(const UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}
	for (TActorIterator<AColdOpenSet> It(World); It; ++It)
	{
		if (!It->IsActorBeingDestroyed())
		{
			return *It;
		}
	}
	return nullptr;
}

FVector AColdOpenSet::ToWorld(const FVector& Local) const
{
	return GetActorTransform().TransformPosition(Local);
}

FTransform AColdOpenSet::ToWorld(const FTransform& Local) const
{
	return Local * GetActorTransform();
}
