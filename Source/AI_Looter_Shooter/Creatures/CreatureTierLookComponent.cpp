#include "Creatures/CreatureTierLookComponent.h"
#include "AI_Looter_Shooter.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreaturePackComponent.h"
#include "Creatures/CreatureTierFXSubsystem.h"
#include "Creatures/CreatureTraitComponent.h"
#include "Creatures/UnpaidCreature.h"
#include "UI/World/CreatureHealthBarWidget.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"

const FName UCreatureTierLookComponent::ShellTag(TEXT("TierShell"));

namespace
{
	/** The shell's flare and fade are written again only when they move by this much (each write updates its primitive). */
	constexpr float TierLookWriteStep = 0.01f;

	/** Its notice beat (the half second it holds, facing its prey) lights its glow a little as its body shows it. */
	constexpr float TierNoticeBeatFlare = 0.35f;
}

UCreatureTierLookComponent::UCreatureTierLookComponent()
{
	// Only while there's a shell to drive (UpdateTicking).
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

// ---------------------------------------------------------------------------
// Finding and making it
// ---------------------------------------------------------------------------

UCreatureTierLookComponent* UCreatureTierLookComponent::Find(const AActor* Actor)
{
	return Actor ? Actor->FindComponentByClass<UCreatureTierLookComponent>() : nullptr;
}

bool UCreatureTierLookComponent::IsTierShell(const UPrimitiveComponent* Component)
{
	return Component && Component->ComponentHasTag(ShellTag);
}

void UCreatureTierLookComponent::Refresh(ACreatureBase& Creature)
{
	UCreatureTierLookComponent* Look = Find(&Creature);
	// A Regular creature, a Legendary monster or a boss never gets one; one that had a tier and lost it (a promotion's
	// life over, back as it was placed) keeps its component, which takes its shell off.
	if (!Look && !CreatureTierLook::HasBodyGlow(Creature.GetRank()))
	{
		return;
	}
	if (!Look)
	{
		if (!Creature.GetWorld())
		{
			return;
		}
		Look = NewObject<UCreatureTierLookComponent>(&Creature);
		// Registered as its rank is applied (its own BeginPlay, a promotion): it begins play at once.
		Look->RegisterComponent();
	}
	Look->ApplyTier();
}

void UCreatureTierLookComponent::BeginPlay()
{
	Super::BeginPlay();
	BindHealth();
}

void UCreatureTierLookComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UHealthComponent* Bound = Health.Get())
	{
		Bound->OnDeath.RemoveDynamic(this, &UCreatureTierLookComponent::HandleDeath);
	}
	Health.Reset();
	Super::EndPlay(EndPlayReason);
}

void UCreatureTierLookComponent::BindHealth()
{
	if (Health.IsValid())
	{
		return;
	}
	if (UHealthComponent* Found = GetOwner() ? GetOwner()->FindComponentByClass<UHealthComponent>() : nullptr)
	{
		Found->OnDeath.AddUniqueDynamic(this, &UCreatureTierLookComponent::HandleDeath);
		Health = Found;
	}
}

ACreatureBase* UCreatureTierLookComponent::GetCreature() const
{
	return Cast<ACreatureBase>(GetOwner());
}

// ---------------------------------------------------------------------------
// Its look at its rank
// ---------------------------------------------------------------------------

void UCreatureTierLookComponent::ApplyTier()
{
	ACreatureBase* Creature = GetCreature();
	if (!Creature)
	{
		return;
	}
	BindHealth();
	ShownRank = Creature->GetRank();
	bFollowsPhase = Creature->IsA<AUnpaidCreature>();
	// Its rank applied afresh is a life begun (play, a respawn) or a promotion: alive, its glow lit. An Unpaid's coal has
	// just been written by its rank (AUnpaidCreature::OnRankChanged), at its own strength.
	DeathAge = -1.f;
	bDeathShown = false;
	bDeathShowPending = false;
	ShownCoalScale = 1.f;
	ShellSpec = CreatureTierLook::ShellFor(CreatureTierLook::BodyOf(*Creature), ShownRank);
	if (ShellSpec.bWorn)
	{
		WearShell();
	}
	else
	{
		DropShell();
	}
	WriteLook(HeldFlare, 1.f, true);
	UpdateTicking();
}

void UCreatureTierLookComponent::WearShell()
{
	ACreatureBase* Creature = GetCreature();
	USkeletalMeshComponent* Body = Creature ? Creature->GetMesh() : nullptr;
	USkeletalMesh* Model = Body ? Body->GetSkeletalMeshAsset() : nullptr;
	UCreatureTierFXSubsystem* Effects = UCreatureTierFXSubsystem::Get(this);
	UMaterialInstanceDynamic* Look = Effects ? Effects->GetShellLook(ShellSpec, ShownRank) : nullptr;
	if (!Model || !Look)
	{
		// No body to follow (a test level without the model) or no M_CreatureTier yet: the rest of its tier still shows.
		DropShell();
		return;
	}
	if (!Shell)
	{
		Shell = NewObject<USkeletalMeshComponent>(Creature, MakeUniqueObjectName(Creature, USkeletalMeshComponent::StaticClass(), TEXT("TierShell")));
		Shell->ComponentTags.Add(ShellTag);
		// Looks only: no collision (so no physics bodies, no hit zones of its own), no shadow, no decals, no navigation.
		Shell->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Shell->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
		Shell->SetGenerateOverlapEvents(false);
		Shell->SetCanEverAffectNavigation(false);
		Shell->SetCastShadow(false);
		Shell->bReceivesDecals = false;
		// The body's bounds and pose: it costs a draw, nothing more (no animation of its own).
		Shell->bUseBoundsFromLeaderPoseComponent = true;
		// After the body's own translucent parts (the slime's gel), so the core's glow is never dimmed by it.
		Shell->SetTranslucentSortPriority(1);
		Shell->SetupAttachment(Body);
		Shell->SetSkeletalMeshAsset(Model);
		Shell->RegisterComponent();
		Shell->SetLeaderPoseComponent(Body, /*bForceUpdate*/ true);
	}
	else if (Shell->GetSkeletalMeshAsset() != Model)
	{
		// A body built on another model since (a kind that swaps its mesh as play begins).
		Shell->SetSkeletalMeshAsset(Model);
		Shell->SetLeaderPoseComponent(Body, /*bForceUpdate*/ true);
	}
	const int32 Slots = Shell->GetNumMaterials();
	for (int32 Slot = 0; Slot < Slots; ++Slot)
	{
		Shell->SetMaterial(Slot, Look);
	}
	Shell->SetVisibility(Body->IsVisible());
	ShownFlare = -1.f;
	ShownFade = -1.f;
}

void UCreatureTierLookComponent::DropShell()
{
	if (Shell)
	{
		Shell->DestroyComponent();
		Shell = nullptr;
	}
	ShownFlare = -1.f;
	ShownFade = -1.f;
}

void UCreatureTierLookComponent::SetHeldFlare(float Amount)
{
	HeldFlare = FMath::Max(Amount, 0.f);
	UpdateTicking();
}

void UCreatureTierLookComponent::UpdateTicking()
{
	// Alive with a shell it ticks (the notice flare, the phase-step, the body hidden for an entrance); dead, until its glow
	// has gone out. A death show waiting ticks it too, and an Unpaid's coal its traits can brighten while it lives.
	const ACreatureBase* Creature = GetCreature();
	const UCreatureTraitComponent* Traits = Creature ? Creature->GetTraitComponent() : nullptr;
	const bool bShellLive = Shell != nullptr && (DeathAge < 0.f || DeathAge <= CreatureTierLook::DeathFadeSeconds);
	const bool bCoalLive = bFollowsPhase && DeathAge < 0.f && Traits && Traits->GetTraits().Num() > 0;
	const bool bWanted = bShellLive || bDeathShowPending || bCoalLive;
	if (IsComponentTickEnabled() != bWanted)
	{
		SetComponentTickEnabled(bWanted);
	}
}

// ---------------------------------------------------------------------------
// Frame by frame: the flare and the fade
// ---------------------------------------------------------------------------

void UCreatureTierLookComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	Advance(DeltaTime);
}

void UCreatureTierLookComponent::Advance(float DeltaSeconds)
{
	ACreatureBase* Creature = GetCreature();
	if (!Creature)
	{
		UpdateTicking();
		return;
	}
	// Its traits' say over its glow, and a Grave-burst's swell, which holds its death (its soul goes with the blast).
	const UCreatureTraitComponent* Traits = Creature->GetTraitComponent();
	const bool bSwelling = Traits && Traits->IsSwelling();
	const float GlowScale = Traits ? Traits->GetGlowScale() : 1.f;
	if (bDeathShowPending && !bSwelling)
	{
		// A top tier goes out in its colour: the burst, the ring, the pillar, the motes, the loot's pop, both sounds.
		bDeathShowPending = false;
		bDeathShown = true;
		if (UCreatureTierFXSubsystem* Effects = UCreatureTierFXSubsystem::Get(this))
		{
			Effects->PlayDeath(*Creature);
		}
		UE_LOG(LogLooter, Verbose, TEXT("%s (%s) died: the tier's death show."), *Creature->GetName(),
			*UEnum::GetValueAsString(Creature->GetRank()));
	}
	if (bFollowsPhase && DeathAge < 0.f)
	{
		ScaleCoal(GlowScale);
	}
	if (!Shell)
	{
		UpdateTicking();
		return;
	}
	// Far off and out of view its body isn't posed either: nothing to show.
	if (Creature->IsPoseFrozen() && DeathAge < 0.f)
	{
		return;
	}
	float Flare = HeldFlare + CreatureTierLook::TraitFlare(GlowScale);
	float Fade = 1.f;
	if (DeathAge >= 0.f)
	{
		// Held bright while a Grave-burst swells, then flaring out.
		if (!bSwelling)
		{
			DeathAge += FMath::Max(DeltaSeconds, 0.f);
		}
		Flare += CreatureTierLook::DeathFlareAt(DeathAge);
		Fade = CreatureTierLook::DeathFadeAt(DeathAge);
	}
	else
	{
		// It turned on the player (its rank sting, a top tier's the notice sting): the glow flares with the tag.
		if (const UCreaturePackComponent* Pack = Creature->GetPack())
		{
			Flare += CreatureTierLook::NoticeFlareAt(Pack->GetRankStingAge(), CreatureTierLook::IsTopTier(ShownRank));
		}
		Flare += TierNoticeBeatFlare * Creature->GetNoticeStrength();
	}
	if (bFollowsPhase)
	{
		// The Unpaid's phase-step and its dissolve take the shell with the body.
		if (const AUnpaidCreature* Unpaid = Cast<AUnpaidCreature>(Creature))
		{
			Fade *= FMath::Clamp(1.f - Unpaid->GetPhase(), 0.f, 1.f);
		}
	}
	WriteLook(Flare, Fade, false);

	// Shown with the body (hidden for an entrance, a body without its model), and not once its glow has gone.
	const USkeletalMeshComponent* Body = Creature->GetMesh();
	const bool bShown = Fade > 0.01f && Body && Body->IsVisible();
	if (Shell->IsVisible() != bShown)
	{
		Shell->SetVisibility(bShown);
	}
	UpdateTicking();
}

void UCreatureTierLookComponent::WriteLook(float Flare, float Fade, bool bForce)
{
	if (!Shell)
	{
		return;
	}
	if (bForce || ShownFlare < 0.f || FMath::Abs(Flare - ShownFlare) > TierLookWriteStep)
	{
		Shell->SetCustomPrimitiveDataFloat(CreatureTierLook::FlareIndex, Flare);
		ShownFlare = Flare;
	}
	if (bForce || ShownFade < 0.f || FMath::Abs(Fade - ShownFade) > TierLookWriteStep || (Fade <= 0.f && ShownFade > 0.f))
	{
		Shell->SetCustomPrimitiveDataFloat(CreatureTierLook::FadeIndex, Fade);
		ShownFade = Fade;
	}
}

// ---------------------------------------------------------------------------
// Its death
// ---------------------------------------------------------------------------

void UCreatureTierLookComponent::HandleDeath(AController* Killer)
{
	ACreatureBase* Creature = GetCreature();
	if (!Creature || Creature->IsBossCreature())
	{
		return;
	}
	DeathAge = 0.f;
	// A top tier's show plays on its next frame (Advance), once its traits have had their say over the death: a
	// Grave-burst's swell holds it until the blast. Its coal goes back to its own strength as it dissolves.
	if (!bDeathShown && !bDeathShowPending && CreatureTierLook::IsTopTier(Creature->GetRank()))
	{
		bDeathShowPending = true;
	}
	ScaleCoal(1.f);
	UpdateTicking();
}

void UCreatureTierLookComponent::ScaleCoal(float Scale)
{
	AUnpaidCreature* Unpaid = Cast<AUnpaidCreature>(GetOwner());
	if (!Unpaid || FMath::IsNearlyEqual(Scale, ShownCoalScale, 0.01f))
	{
		return;
	}
	ShownCoalScale = Scale;
	// The alpha of its rank colour (UnpaidLook::RankColorIndex's fourth float) is how hard the coal and its ember edge glow.
	const float Glow = CreatureTierLook::CoalGlow(ShownRank, Unpaid->GetRankTraits().CoalGlow) * Scale;
	const int32 GlowIndex = UnpaidLook::RankColorIndex + 3;
	if (USkeletalMeshComponent* Body = Unpaid->GetMesh())
	{
		Body->SetCustomPrimitiveDataFloat(GlowIndex, Glow);
	}
	if (UStaticMeshComponent* Hat = Unpaid->GetHat())
	{
		Hat->SetCustomPrimitiveDataFloat(GlowIndex, Glow);
	}
}

// ---------------------------------------------------------------------------
// Its tag
// ---------------------------------------------------------------------------

FText UCreatureTierLookComponent::TraitLineNow()
{
	const ACreatureBase* Creature = GetCreature();
	const UCreatureTraitComponent* Traits = Creature ? Creature->GetTraitComponent() : nullptr;
	if (!Traits)
	{
		if (!bLineMade || !LineTraits.IsEmpty())
		{
			LineTraits.Reset();
			Line = FText::GetEmpty();
			bLineMade = true;
		}
		return Line;
	}
	const TConstArrayView<ECreatureTrait> Now = Traits->GetTraits();
	bool bSame = bLineMade && Now.Num() == LineTraits.Num();
	for (int32 Index = 0; bSame && Index < Now.Num(); ++Index)
	{
		bSame = Now[Index] == LineTraits[Index];
	}
	if (!bSame)
	{
		LineTraits.Reset();
		LineTraits.Append(Now.GetData(), Now.Num());
		Line = CreatureTierLook::TraitLine(Now);
		bLineMade = true;
	}
	return Line;
}

void UCreatureTierLookComponent::DressTag(const ACreatureBase& Creature, UCreatureHealthBarWidget& Tag)
{
	// Every creature's tag is dressed (a Regular one's plainly): a tag a promoted creature wore keeps nothing of it.
	const ECreatureRank Rank = Creature.GetRank();
	const FCreatureTagTier Look = CreatureTierLook::TagFor(Rank);
	Tag.SetTier(Look.bThickFrame, Look.FrameColor, Look.bTopTierFlash);
	FText Traits = FText::GetEmpty();
	if (CreatureTierLook::ShowsTraitLine(Rank))
	{
		if (UCreatureTierLookComponent* Own = Find(&Creature))
		{
			Traits = Own->TraitLineNow();
		}
	}
	Tag.SetTraitLine(Traits);
	// A Warded one's ward over its health while it stands (any tier: the dev command gives traits to Regulars too).
	const UCreatureTraitComponent* TraitComponent = Creature.GetTraitComponent();
	const bool bWarded = TraitComponent && TraitComponent->HasTrait(ECreatureTrait::Warded);
	Tag.SetWard(bWarded, bWarded ? TraitComponent->GetWardFraction() : 0.f);
}
