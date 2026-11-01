// CageBurst: the soul cage opened, and what comes out isn't Abel (Main 6 step 6).

#include "Scenes/CageBurst.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Player/ViewKick.h"
#include "Scenes/SceneSubsystem.h"
#include "Story/VaneBroadcast.h"
#include "World/BargeBoarding.h"
#include "World/BargeFurnace.h"
#include "World/BargeLooks.h"
#include "World/BargeRules.h"
#include "World/LamplighterEffects.h"
#include "World/LamplighterRules.h"
#include "Animation/SkeletalMeshActor.h"
#include "Camera/CameraActor.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInterface.h"

namespace
{
	// When things happen (s).
	constexpr float CrackAt = 0.5f;
	constexpr float BurstAt = 1.1f;
	constexpr float LungeSeconds = 0.5f;
	constexpr float ThrowAt = 1.6f;
	constexpr float ThrowSeconds = 0.55f;
	constexpr float TearAt = 2.2f;
	constexpr float TearSeconds = 1.0f;
	constexpr float StreakSeconds = 1.2f;
	constexpr float VaneAt = 4.0f;
	constexpr float RiseAt = 4.4f;
	constexpr float RiseSeconds = 1.8f;
	constexpr float HandBackAt = 6.5f;
	constexpr float EndAt = 7.f;

	/** Sully is thrown this far back from the cage (cm), never nearer the wall than this. */
	constexpr float ThrowBack = 520.f;
	constexpr float KeepOffWall = 180.f;
	constexpr float EyeHeight = 160.f;
	constexpr float FloorEye = 45.f;
	/** The shape's size (Abel's 1.3), how far short of Sully its lunge stops, and where it streaks to (west, up, far). */
	constexpr float ShapeScale = 1.3f;
	constexpr float LungeShort = 120.f;
	const FVector StreakTo(0.0, -40000.0, 6000.0);
	const TCHAR* ShapePath = TEXT("/Game/Art/Creatures/SK_Abel.SK_Abel");
	const FLinearColor CoalColor(1.f, 0.42f, 0.08f);

	const FName CageLines(TEXT("DA_Lines_Main6Cage"));

	struct FBurstState
	{
		TWeakObjectPtr<USceneSubsystem> Scenes;
		TWeakObjectPtr<ABargeBoarding> Barge;
		TWeakObjectPtr<ABargeFurnace> Furnace;
		TWeakObjectPtr<AActor> Shape;
		FVector Eye = FVector::ZeroVector;
		FVector Cage = FVector::ZeroVector;
		FVector Landed = FVector::ZeroVector;
		FVector LandedFeet = FVector::ZeroVector;
		FVector BackWall = FVector::ZeroVector;
		FVector StreakEnd = FVector::ZeroVector;
		float Clock = 0.f;
		float PuffWait = 0.f;
	};
	using FBurstRef = TSharedRef<FBurstState>;

	bool IsSkipping(const FBurstState& State)
	{
		const USceneSubsystem* Scenes = State.Scenes.Get();
		return !Scenes || Scenes->IsSkipping();
	}

	void SetView(const FBurstState& State, const FVector& From, const FVector& At, float Roll = 0.f)
	{
		const USceneSubsystem* Scenes = State.Scenes.Get();
		if (ACameraActor* Camera = Scenes ? Scenes->GetSceneCamera() : nullptr)
		{
			FRotator Look = (At - From).Rotation();
			Look.Roll = Roll;
			Camera->SetActorLocationAndRotation(From, Look);
		}
	}

	float Ease(float Alpha)
	{
		return FMath::SmoothStep(0.f, 1.f, FMath::Clamp(Alpha, 0.f, 1.f));
	}

	/** The black shape: Abel's body in soot-black at his size, a coal heart's glow in its chest (a stand-in until Tripo 20). */
	AActor* SpawnShape(UWorld& World, const FVector& At, const FVector& Facing)
	{
		USkeletalMesh* Body = BargeLooks::Load<USkeletalMesh>(ShapePath);
		UMaterialInterface* Soot = BargeLooks::Load<UMaterialInterface>(BargeLooks::IronBlack);
		UMaterialInterface* Glow = BargeLooks::Load<UMaterialInterface>(BargeLooks::Glow);
		UStaticMesh* Sphere = BargeLooks::Load<UStaticMesh>(BargeLooks::Sphere);
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ASkeletalMeshActor* Shape = World.SpawnActor<ASkeletalMeshActor>(At, Facing.Rotation(), Params);
		if (!Shape)
		{
			return nullptr;
		}
		USkeletalMeshComponent* Figure = Shape->GetSkeletalMeshComponent();
		Figure->SetMobility(EComponentMobility::Movable);
		Figure->SetSkeletalMeshAsset(Body);
		Figure->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Figure->SetCastShadow(false);
		Shape->SetActorScale3D(FVector(ShapeScale));
		if (Soot)
		{
			for (int32 Slot = 0; Slot < Figure->GetNumMaterials(); ++Slot)
			{
				Figure->SetMaterial(Slot, Soot);
			}
		}
		// The coal heart behind its ribs, and its glow on the hall round it (shadowless).
		UStaticMeshComponent* Coal = NewObject<UStaticMeshComponent>(Shape);
		Coal->SetupAttachment(Figure);
		Coal->SetStaticMesh(Sphere);
		Coal->SetMaterial(0, Glow);
		Coal->SetRelativeLocation(FVector(10.0, 0.0, 130.0));
		Coal->SetRelativeScale3D(FVector(0.16));
		Coal->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Coal->RegisterComponent();
		UPointLightComponent* Light = NewObject<UPointLightComponent>(Shape);
		Light->SetupAttachment(Figure);
		Light->SetRelativeLocation(FVector(30.0, 0.0, 130.0));
		Light->SetIntensityUnits(ELightUnits::Candelas);
		Light->SetIntensity(40.f);
		Light->SetAttenuationRadius(900.f);
		Light->SetLightColor(CoalColor);
		Light->SetCastShadows(false);
		Light->RegisterComponent();
		return Shape;
	}

	/** Smog pouring off it (code-drawn puffs: no new effects). */
	void Smog(const FBurstState& State, const FVector& At, int32 Puffs)
	{
		const USceneSubsystem* Scenes = State.Scenes.Get();
		for (int32 Puff = 0; Puff < Puffs; ++Puff)
		{
			LamplighterEffects::SmokePuff(Scenes, At + FVector(FMath::FRandRange(-60.f, 60.f), FMath::FRandRange(-60.f, 60.f), FMath::FRandRange(0.f, 180.f)),
				FVector(FMath::FRandRange(-80.f, 80.f), FMath::FRandRange(-80.f, 80.f), 160.0));
		}
	}

	void PlaceShape(const FBurstState& State, const FVector& At, const FVector& Facing)
	{
		if (AActor* Shape = State.Shape.Get())
		{
			// Its feet at At (the shape's pivot is its feet, as SK_Abel is built).
			Shape->SetActorLocationAndRotation(At, FRotator(0.f, static_cast<float>(Facing.Rotation().Yaw), 0.f));
		}
	}
}

FName CageBurst::SceneName()
{
	static const FName Name(TEXT("CageBurst"));
	return Name;
}

float CageBurst::Duration()
{
	return EndAt;
}

bool CageBurst::Play(ABargeBoarding& Barge, ABargeFurnace& Furnace)
{
	USceneSubsystem* Scenes = USceneSubsystem::Get(&Barge);
	UWorld* World = Barge.GetWorld();
	const auto PassOver = [Scenes]()
	{
		if (Scenes && !Scenes->HasPlayed(SceneName()))
		{
			Scenes->MarkPlayed(SceneName());
		}
	};
	if (!Scenes || !World || !USceneSubsystem::AreScenesOn(World) || Scenes->IsPlaying())
	{
		PassOver();
		return false;
	}
	const FBurstRef State = MakeShared<FBurstState>();
	State->Scenes = Scenes;
	State->Barge = &Barge;
	State->Furnace = &Furnace;
	State->Cage = Furnace.GetCageLocation();
	State->BackWall = Furnace.GetBackWall();
	State->StreakEnd = State->BackWall + StreakTo;

	FScenePlay Scene;
	Scene.Name = SceneName();
	Scene.bLookOnly = false;
	Scene.OnStart = [State]()
	{
		USceneSubsystem* Played = State->Scenes.Get();
		ABargeFurnace* Cage = State->Furnace.Get();
		if (!Played || !Cage)
		{
			return;
		}
		// Sully's eyes where they stand at the cage; where they'll land, thrown back away from it inside the hall.
		const APawn* Player = Played->GetHeldPawn();
		const FVector Facing = Cage->GetCageFacing();
		State->Eye = Player ? Player->GetActorLocation() + FVector(0.0, 0.0, Player->BaseEyeHeight) : State->Cage + Facing * 250.0;
		const FVector Middle = Cage->GetActorLocation();
		FVector Out = State->Eye - Middle;
		Out.Z = 0.0;
		const double Reach = FMath::Min(Out.Size() + ThrowBack, static_cast<double>(BargeRules::HallRadius - KeepOffWall));
		const FVector Flat = Out.GetSafeNormal(UE_SMALL_NUMBER, Facing);
		State->LandedFeet = FVector(Middle.X, Middle.Y, Middle.Z) + Flat * Reach;
		State->Landed = State->LandedFeet + FVector(0.0, 0.0, FloorEye);
		Played->ViewFromSceneCamera(0.15f);
		SetView(*State, State->Eye, State->Cage);
		LooterSound::PlayAt(Played, LooterSoundCue::GrappleStrain, State->Cage, 0.9f, 0.6f);
	};
	Scene.OnTick = [State](float DeltaSeconds) { State->Clock += DeltaSeconds; };
	FSceneTimeline& Timeline = Scene.Timeline;

	// At the cage: the glass cracks, the light spikes.
	Timeline.AddMove(0.f, ThrowAt, [State](float) { SetView(*State, State->Eye, State->Cage); });
	Timeline.AddMoment(TEXT("Crack"), CrackAt, [State]()
	{
		if (IsSkipping(*State))
		{
			return;
		}
		LooterSound::PlayAt(State->Scenes.Get(), LooterSoundCue::ImpactMetal, State->Cage, 1.2f, 1.3f);
		LamplighterEffects::Sparks(State->Scenes.Get(), State->Cage, (State->Eye - State->Cage).GetSafeNormal(), 10, 0.6f);
		LamplighterEffects::Shake(State->Scenes.Get(), 0.25f, 0.4f);
		if (ABargeFurnace* Cage = State->Furnace.Get())
		{
			Cage->CageLight->SetIntensity(Cage->LitCandelas * 3.f);
		}
	});
	// The burst: the cage gone, and the shape in the smog where Abel's light was.
	Timeline.AddMoment(TEXT("Burst"), BurstAt, [State]()
	{
		ABargeFurnace* Cage = State->Furnace.Get();
		if (Cage)
		{
			Cage->SetCageBurst(true);
		}
		if (IsSkipping(*State) || !Cage)
		{
			return;
		}
		USceneSubsystem* Played = State->Scenes.Get();
		LooterSound::PlayAt(Played, LooterSoundCue::Launcher::Explosion, State->Cage, 0.8f, 0.7f);
		LooterSound::PlayAt(Played, LooterSoundCue::AbelWail, State->Cage, 1.3f, 0.7f);
		LamplighterEffects::Fireball(Played, State->Cage, (State->Eye - State->Cage).GetSafeNormal() * 300.0);
		LamplighterEffects::Shake(Played, 1.f, 0.9f);
		if (UWorld* World = Cage->GetWorld())
		{
			AActor* Shape = SpawnShape(*World, State->Cage - FVector(0.0, 0.0, BargeRules::CageUp), State->Eye - State->Cage);
			State->Shape = Shape;
			if (Shape)
			{
				// Gone with the scene, played or skipped.
				Played->AddSceneActor(Shape);
			}
		}
		Smog(*State, State->Cage, 8);
	});
	// It comes at Sully.
	Timeline.AddMove(BurstAt, LungeSeconds, [State](float Alpha)
	{
		const FVector Toward = (State->Eye - State->Cage).GetSafeNormal2D();
		const FVector From = State->Cage - FVector(0.0, 0.0, BargeRules::CageUp);
		const FVector To = FVector(State->Eye.X, State->Eye.Y, From.Z) - Toward * LungeShort;
		PlaceShape(*State, FMath::Lerp(From, To, static_cast<double>(Ease(Alpha))), Toward);
		if (!IsSkipping(*State) && FMath::FRand() < 0.4f)
		{
			Smog(*State, FMath::Lerp(From, To, static_cast<double>(Alpha)), 1);
		}
	});
	// Thrown across the hall: the view knocked back and rolling onto the floor.
	Timeline.AddMoment(TEXT("Throw"), ThrowAt, [State]()
	{
		if (IsSkipping(*State))
		{
			return;
		}
		USceneSubsystem* Played = State->Scenes.Get();
		LooterSound::PlayAt(Played, LooterSoundCue::Gunfeel::ImpactHeavy, State->Eye, 1.3f, 0.8f);
		FViewKick Blow;
		Blow.Pitch = 9.f;
		Blow.Roll = 14.f;
		Blow.FieldOfView = 0.05f;
		Blow.Frequency = 6.f;
		Blow.Damping = 0.4f;
		LamplighterEffects::Kick(Played, Blow);
		LamplighterEffects::Shake(Played, 0.9f, 0.7f);
	});
	Timeline.AddMove(ThrowAt, ThrowSeconds, [State](float Alpha)
	{
		const FVector Arc = FMath::Lerp(State->Eye, State->Landed, static_cast<double>(Ease(Alpha))) + FVector(0.0, 0.0, 70.0 * FMath::Sin(UE_PI * Alpha));
		SetView(*State, Arc, State->Cage, 25.f * FMath::Sin(UE_PI * Alpha));
	});
	Timeline.AddMoment(TEXT("Landed"), ThrowAt + ThrowSeconds, [State]()
	{
		if (!IsSkipping(*State))
		{
			LooterSound::PlayAt(State->Scenes.Get(), LooterSoundCue::Footfall::LandHard, State->LandedFeet, 1.2f, 0.8f);
			LamplighterEffects::Shake(State->Scenes.Get(), 0.5f, 0.4f);
		}
	});
	// From the floor: it turns from Sully, over the furnace, and tears out through the hall's back wall.
	Timeline.AddMove(ThrowAt + ThrowSeconds, RiseAt - ThrowAt - ThrowSeconds, [State](float)
	{
		const AActor* Shape = State->Shape.Get();
		SetView(*State, State->Landed, Shape ? Shape->GetActorLocation() + FVector(0.0, 0.0, 180.0) : State->BackWall, 6.f);
	});
	Timeline.AddMove(TearAt, TearSeconds, [State](float Alpha)
	{
		const FVector Toward = (State->Eye - State->Cage).GetSafeNormal2D();
		const FVector From = FVector(State->Eye.X, State->Eye.Y, State->Cage.Z - BargeRules::CageUp) - Toward * LungeShort;
		const FVector Over = State->Cage + FVector(0.0, 0.0, 500.0);
		const FVector To = State->BackWall;
		const FVector At = Alpha < 0.5f ? FMath::Lerp(From, Over, static_cast<double>(Ease(Alpha * 2.f)))
			: FMath::Lerp(Over, To, static_cast<double>(Ease(Alpha * 2.f - 1.f)));
		PlaceShape(*State, At, (To - From).GetSafeNormal2D());
		if (!IsSkipping(*State) && FMath::FRand() < 0.5f)
		{
			Smog(*State, At, 1);
		}
	});
	Timeline.AddMoment(TEXT("Tear"), TearAt + TearSeconds, [State]()
	{
		if (IsSkipping(*State))
		{
			return;
		}
		USceneSubsystem* Played = State->Scenes.Get();
		LooterSound::PlayAt(Played, LooterSoundCue::Foreman::RoofStrike, State->BackWall, 1.5f, 0.7f);
		LamplighterEffects::Fireball(Played, State->BackWall, FVector(0.0, 0.0, 200.0));
		LamplighterEffects::Sparks(Played, State->BackWall, FVector::UpVector, 30, 1.5f);
		LamplighterEffects::Shake(Played, 0.8f, 1.f);
	});
	// West into the dusk, a black streak.
	Timeline.AddMove(TearAt + TearSeconds, StreakSeconds, [State](float Alpha)
	{
		const FVector At = FMath::Lerp(State->BackWall, State->StreakEnd, static_cast<double>(Alpha * Alpha));
		PlaceShape(*State, At, State->StreakEnd - State->BackWall);
		if (!IsSkipping(*State))
		{
			Smog(*State, At, 1);
		}
	});
	Timeline.AddMoment(TEXT("Gone"), TearAt + TearSeconds + StreakSeconds, [State]()
	{
		if (AActor* Shape = State->Shape.Get())
		{
			Shape->SetActorHiddenInGame(true);
		}
	});
	// Vane, on the horns.
	Timeline.AddMoment(TEXT("Vane"), VaneAt, [State]()
	{
		if (!IsSkipping(*State))
		{
			UVaneBroadcast::Say(State->Scenes.Get(), LandfallRules::LoadLines(CageLines));
		}
	});
	// Up off the floor, looking at the breach.
	Timeline.AddMove(RiseAt, RiseSeconds, [State](float Alpha)
	{
		const FVector Up = State->LandedFeet + FVector(0.0, 0.0, EyeHeight);
		SetView(*State, FMath::Lerp(State->Landed, Up, static_cast<double>(Ease(Alpha))), State->BackWall, 6.f * (1.f - Alpha));
	});
	Timeline.AddMoment(TEXT("HandBack"), HandBackAt, [State]()
	{
		USceneSubsystem* Played = State->Scenes.Get();
		if (!Played)
		{
			return;
		}
		if (!State->LandedFeet.IsNearlyZero())
		{
			Played->PlaceHeldPlayer(State->LandedFeet, static_cast<float>((State->BackWall - State->LandedFeet).Rotation().Yaw));
		}
		Played->ViewFromPlayer(Played->IsSkipping() ? 0.f : 0.5f);
	});
	Timeline.AddMoment(TEXT("End"), EndAt);
	// Played or skipped: the cage burst, the barge's pilot light gone, and she begins to die.
	Scene.AfterEnd = [State]()
	{
		if (ABargeFurnace* Cage = State->Furnace.Get())
		{
			Cage->SetCageBurst(true);
		}
		if (ABargeBoarding* Barge = State->Barge.Get())
		{
			Barge->NoteCageBurst();
			Barge->BeginDying();
		}
	};
	const bool bPlays = Scenes->Play(MoveTemp(Scene));
	if (!bPlays)
	{
		PassOver();
	}
	else
	{
		UE_LOG(LogLooter, Log, TEXT("CageBurst: the scene plays (%.0f s)."), EndAt);
	}
	return bPlays;
}
