// AFarSignalLight: a far Rangers' post answering by signal lamp, keyed in Morse on a backdrop mesa, shown by the story.

#include "World/FarSignalLight.h"
#include "AI_Looter_Shooter.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** The gunfire's additive glow on the engine's plane (R, G, B, strength, shape: 1 a round soft glow). */
	const TCHAR* GlowPath = TEXT("/Game/Weapons/FX/M_FX_Glow.M_FX_Glow");
	const TCHAR* PlanePath = TEXT("/Engine/BasicShapes/Plane.Plane");
	constexpr int32 GlowFloats = 5;
	constexpr int32 StrengthFloat = 3;
	constexpr float RoundShape = 1.f;

	/** The Last Watch: the far post answers in its fifth step (from 0: 4), "Read the answer with Ruth's code book". */
	const FName LastWatch(TEXT("LastWatch"));
	constexpr int32 AnswerStep = 4;

	/** Keyed at 30 frames a second: a unit of 0.22 s is about 6 of them, enough for its soft edges. */
	constexpr float KeyTickSeconds = 1.f / 30.f;

	/** A change of brightness smaller than this isn't worth a render update. */
	constexpr float KeyStep = 0.02f;

	/** The gaps of Morse, in units: between a letter's elements, between letters, between words. */
	constexpr int32 ElementGap = 1;
	constexpr int32 LetterGap = 3;
	constexpr int32 WordGap = 7;

	/** International Morse for a letter or digit (dots and dashes), or null. */
	const TCHAR* MorseFor(TCHAR Character)
	{
		static const TCHAR* const Letters[] = {
			TEXT(".-"), TEXT("-..."), TEXT("-.-."), TEXT("-.."), TEXT("."), TEXT("..-."), TEXT("--."), TEXT("...."), TEXT(".."),
			TEXT(".---"), TEXT("-.-"), TEXT(".-.."), TEXT("--"), TEXT("-."), TEXT("---"), TEXT(".--."), TEXT("--.-"), TEXT(".-."),
			TEXT("..."), TEXT("-"), TEXT("..-"), TEXT("...-"), TEXT(".--"), TEXT("-..-"), TEXT("-.--"), TEXT("--..") };
		static const TCHAR* const Digits[] = {
			TEXT("-----"), TEXT(".----"), TEXT("..---"), TEXT("...--"), TEXT("....-"), TEXT("....."), TEXT("-...."), TEXT("--..."),
			TEXT("---.."), TEXT("----.") };
		const TCHAR Upper = FChar::ToUpper(Character);
		if (Upper >= TEXT('A') && Upper <= TEXT('Z'))
		{
			return Letters[Upper - TEXT('A')];
		}
		if (Upper >= TEXT('0') && Upper <= TEXT('9'))
		{
			return Digits[Upper - TEXT('0')];
		}
		return nullptr;
	}

	void AddUnits(TArray<bool>& Units, bool bOn, int32 Count)
	{
		for (int32 Unit = 0; Unit < Count; ++Unit)
		{
			Units.Add(bOn);
		}
	}

	/** An asset found only once it's in this checkout, so the class works, and its tests run, without it. In a constructor. */
	template <typename T>
	T* FindIfMade(const TCHAR* ObjectPath)
	{
		if (!FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(ObjectPath))))
		{
			return nullptr;
		}
		ConstructorHelpers::FObjectFinder<T> Finder(ObjectPath);
		return Finder.Object;
	}
}

AFarSignalLight::AFarSignalLight()
{
	// Ticks only while shown (SetShown).
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	static UStaticMesh* const Plane = FindIfMade<UStaticMesh>(PlanePath);
	static UMaterialInterface* const Glow = FindIfMade<UMaterialInterface>(GlowPath);
	CardMesh = Plane;
	GlowMaterial = Glow;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	Card = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Card"));
	Card->SetupAttachment(Root);
	Card->SetMobility(EComponentMobility::Movable);
	Card->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Card->SetGenerateOverlapEvents(false);
	Card->SetCanEverAffectNavigation(false);
	Card->SetCastShadow(false);
	Card->bReceivesDecals = false;
	Card->SetNumCustomDataFloats(GlowFloats);
	Card->SetVisibility(false);

	FStoryCondition Answering;
	Answering.DuringMission = LastWatch;
	Answering.FromStep = AnswerStep;
	FStoryCondition After;
	After.AfterMissions = { LastWatch };
	ShownWhen = { Answering, After };
}

void AFarSignalLight::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	BuildCards();
	Card->SetVisibility(bAlwaysShown);
	ApplyKey(bAlwaysShown ? 1.f : 0.f, /*bForce*/ true);
}

void AFarSignalLight::BeginPlay()
{
	Super::BeginPlay();
	Units = KeyUnits(Message);
	BuildCards();
	if (UMissionRunner* Runner = UMissionRunner::Get(this))
	{
		BoundRunner = Runner;
		MissionsChangedHandle = Runner->OnChanged.AddUObject(this, &AFarSignalLight::HandleMissionsChanged);
	}
	// Dark until the story says (the editor's preview may have shown it).
	bShown = true;
	SetShown(false);
	RefreshStory();
}

void AFarSignalLight::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UMissionRunner* Runner = BoundRunner.Get())
	{
		Runner->OnChanged.Remove(MissionsChangedHandle);
	}
	BoundRunner.Reset();
	MissionsChangedHandle.Reset();
	Super::EndPlay(EndPlayReason);
}

void AFarSignalLight::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Advance(DeltaSeconds);
}

// ---------------------------------------------------------------------------
// The signal
// ---------------------------------------------------------------------------

TArray<bool> AFarSignalLight::KeyUnits(const FString& Text)
{
	TArray<bool> Keyed;
	bool bAny = false;
	bool bNewWord = false;
	for (const TCHAR Character : Text)
	{
		const TCHAR* Code = MorseFor(Character);
		if (!Code)
		{
			bNewWord = bAny;
			continue;
		}
		if (bAny)
		{
			AddUnits(Keyed, false, bNewWord ? WordGap : LetterGap);
		}
		bNewWord = false;
		for (int32 Element = 0; Code[Element] != TEXT('\0'); ++Element)
		{
			if (Element > 0)
			{
				AddUnits(Keyed, false, ElementGap);
			}
			AddUnits(Keyed, true, Code[Element] == TEXT('.') ? 1 : 3);
		}
		bAny = true;
	}
	if (bAny)
	{
		AddUnits(Keyed, false, RepeatGapUnits);
	}
	return Keyed;
}

float AFarSignalLight::KeyAt(const TArray<bool>& Keyed, float Seconds, float Unit, float EdgeShare)
{
	const int32 Count = Keyed.Num();
	if (Count == 0 || Unit <= 0.f)
	{
		return 0.f;
	}
	const float Position = FMath::Fmod(FMath::Max(Seconds, 0.f) / Unit, static_cast<float>(Count));
	const int32 Index = FMath::Clamp(FMath::FloorToInt(Position), 0, Count - 1);
	if (!Keyed[Index])
	{
		return 0.f;
	}
	// A wink eases on as it starts and off as it ends; within a dash it holds.
	const float Within = Position - static_cast<float>(Index);
	const float Edge = FMath::Clamp(EdgeShare, 0.01f, 0.5f);
	float Key = 1.f;
	if (!Keyed[(Index + Count - 1) % Count])
	{
		Key *= FMath::SmoothStep(0.f, Edge, Within);
	}
	if (!Keyed[(Index + 1) % Count])
	{
		Key *= 1.f - FMath::SmoothStep(1.f - Edge, 1.f, Within);
	}
	return Key;
}

// ---------------------------------------------------------------------------
// The story and the look
// ---------------------------------------------------------------------------

bool AFarSignalLight::ShouldShow(const FCampaignRecord& Campaign, const UMissionRunner* Runner) const
{
	return Campaign.HasFlag(ShownFlag)
		|| ShownWhen.ContainsByPredicate([&Campaign, Runner](const FStoryCondition& Each) { return Each.IsMet(Campaign, Runner); });
}

void AFarSignalLight::RefreshStory()
{
	if (bAlwaysShown)
	{
		SetShown(true);
		return;
	}
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	if (Runner && Runner->IsActive())
	{
		SetShown(ShouldShow(Runner->GetCampaign(), Runner));
	}
}

void AFarSignalLight::HandleMissionsChanged()
{
	RefreshStory();
}

void AFarSignalLight::SetShown(bool bInShown)
{
	if (bShown == bInShown)
	{
		return;
	}
	bShown = bInShown;
	if (Units.IsEmpty())
	{
		Units = KeyUnits(Message);
	}
	Clock = 0.f;
	Card->SetVisibility(bShown && CardMesh != nullptr);
	ApplyKey(bShown ? KeyAt(Units, Clock, UnitSeconds) : 0.f, /*bForce*/ true);
	SetActorTickInterval(KeyTickSeconds);
	SetActorTickEnabled(bShown);
	if (bShown)
	{
		UE_LOG(LogLooter, Log, TEXT("%s: a far light keys \"%s\"."), *GetActorNameOrLabel(), *Message);
	}
}

void AFarSignalLight::Advance(float DeltaSeconds)
{
	if (!bShown)
	{
		return;
	}
	Clock += DeltaSeconds;
	ApplyKey(KeyAt(Units, Clock, UnitSeconds), /*bForce*/ false);
}

void AFarSignalLight::BuildCards()
{
	Card->SetStaticMesh(CardMesh);
	Card->SetMaterial(0, GlowMaterial);
	Card->SetNumCustomDataFloats(GlowFloats);
	Card->ClearInstances();
	// The engine's plane lies flat in XY: one card flat, one standing along X, one along Y, so it reads from anywhere.
	const float Data[] = { Color.R, Color.G, Color.B, 0.f, RoundShape };
	const FRotator Cards[] = { FRotator(0.0, 0.0, 0.0), FRotator(0.0, 0.0, 90.0), FRotator(90.0, 0.0, 0.0) };
	for (const FRotator& Each : Cards)
	{
		Card->SetCustomData(Card->AddInstance(FTransform(Each, FVector::ZeroVector, FVector(Size / 100.f))), Data);
	}
	LastKey = -1.f;
}

void AFarSignalLight::ApplyKey(float Key, bool bForce)
{
	if (!bForce && FMath::Abs(Key - LastKey) < KeyStep && !(Key == 0.f && LastKey != 0.f))
	{
		return;
	}
	for (int32 Instance = 0; Instance < Card->GetInstanceCount(); ++Instance)
	{
		Card->SetCustomDataValue(Instance, StrengthFloat, Strength * Key, /*bMarkRenderStateDirty*/ false);
	}
	Card->MarkRenderStateDirty();
	LastKey = Key;
}
