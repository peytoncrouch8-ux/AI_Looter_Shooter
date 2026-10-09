#include "World/FaunaCloth.h"
#include "Audio/LooterSound.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"

namespace
{
	/** A hung piece's swing as a damped spring: its own rate (radians a second) and damping (a share of critical). */
	constexpr float SwingRate = UE_TWO_PI * 0.8f;
	constexpr float SwingDamping = 0.25f;

	/** The longest step of the springs (s): longer updates are split so they never blow up. */
	constexpr float MaxStep = 1.f / 30.f;

	/** Its flutter's rate (cycles a second, Min to Max). */
	constexpr float FlutterMin = 4.f;
	constexpr float FlutterMax = 7.f;

	/** A gust this strong (a share of the strongest) may snap the washing (s between, Min to Max). */
	constexpr float SnapGust = 0.9f;
	constexpr float SnapSecondsMin = 4.f;
	constexpr float SnapSecondsMax = 9.f;
}

AFaunaCloth::AFaunaCloth()
{
	CullDistance = 4500.f;
}

void AFaunaCloth::BeginPlay()
{
	Super::BeginPlay();
	SetupCloth();
}

void AFaunaCloth::SetupCloth()
{
	if (bReady)
	{
		return;
	}
	bReady = true;
	Random.Initialize(Seed);
	// One component per mesh, its pieces' instances in the order they come.
	Parts.Reset();
	Poses.Reset();
	for (int32 Index = 0; Index < Meshes.Num(); ++Index)
	{
		UStaticMesh* Mesh = Meshes[Index];
		Parts.Add(Mesh ? MakeInstances(FName(*FString::Printf(TEXT("Cloth%d"), Index)), Mesh, /*bCastShadows=*/true) : nullptr);
		Poses.AddDefaulted();
	}
	PieceInstance.Init(INDEX_NONE, Pieces.Num());
	Swings.SetNum(Pieces.Num());
	FBox Box(ForceInit);
	for (int32 Index = 0; Index < Pieces.Num(); ++Index)
	{
		const FFaunaClothPiece& Piece = Pieces[Index];
		Box += Piece.Location;
		Swings[Index].Phase = Random.FRandRange(0.f, 100.f);
		Swings[Index].Flutter = Random.FRandRange(0.f, UE_TWO_PI);
		if (Parts.IsValidIndex(Piece.Mesh) && Parts[Piece.Mesh])
		{
			PieceInstance[Index] = Parts[Piece.Mesh]->AddInstance(FTransform(FRotator(0.f, Piece.Yaw, 0.f), Piece.Location), /*bWorldSpace=*/true);
			Poses[Piece.Mesh].Add(FTransform::Identity);
		}
	}
	Center = Box.IsValid ? Box.GetCenter() : GetActorLocation();
	Reach = Box.IsValid ? static_cast<float>(Box.GetExtent().Size()) + 100.f : 100.f;
	SoundTimer = Random.FRandRange(SnapSecondsMin, SnapSecondsMax);
}

FVector AFaunaCloth::GetFaunaCenter() const
{
	return Center;
}

float AFaunaCloth::GetFaunaRadius() const
{
	return Reach;
}

void AFaunaCloth::UpdateFauna(const FFaunaTick& Tick, const FFaunaContext& Context)
{
	if (!bReady)
	{
		SetupCloth();
	}
	const FVector Wind = FRotator(0.f, WindYaw, 0.f).Vector();
	float StrongestGust = 0.f;
	float Left = Tick.DeltaSeconds;
	while (Left > 0.f)
	{
		const float Step = FMath::Min(Left, MaxStep);
		Left -= Step;
		WindTime += Step;
		for (int32 Index = 0; Index < Pieces.Num(); ++Index)
		{
			FFaunaClothSwing& Swing = Swings[Index];
			const FVector Front = FRotator(0.f, Pieces[Index].Yaw, 0.f).Vector();
			// The wind on its face swings its bottom out the way the wind blows; edge-on, it hardly moves.
			const float Gust = FMath::Clamp(0.5f + FMath::PerlinNoise1D(WindTime / GustSeconds + Swing.Phase), 0.f, 1.2f);
			StrongestGust = FMath::Max(StrongestGust, Gust);
			const float Goal = SwingDegrees * Gust * static_cast<float>(FVector::DotProduct(Wind, Front));
			const float Pull = SwingRate * SwingRate * (Goal - Swing.Angle) - 2.f * SwingDamping * SwingRate * Swing.Speed;
			Swing.Speed += Pull * Step;
			Swing.Angle += Swing.Speed * Step;
			Swing.Flutter = FMath::Fmod(Swing.Flutter + UE_TWO_PI * FMath::Lerp(FlutterMin, FlutterMax, Gust / 1.2f) * Step, UE_TWO_PI);
		}
	}
	// Now and then a strong gust snaps the washing, while someone's near enough to hear it.
	SoundTimer -= Tick.DeltaSeconds;
	if (SoundTimer <= 0.f && StrongestGust > SnapGust && !FlapCue.IsNone() && Pieces.Num() > 0 && IsFaunaShown())
	{
		SoundTimer = Random.FRandRange(SnapSecondsMin, SnapSecondsMax);
		const FVector& Where = Pieces[Random.RandRange(0, Pieces.Num() - 1)].Location;
		for (const FFaunaThreat& Threat : Context.Threats)
		{
			if (Threat.bPlayer && FVector::DistSquared(Threat.Location, Where) < FMath::Square(SoundRange))
			{
				LooterSound::PlayAt(this, FlapCue, Where);
				break;
			}
		}
	}
	if (Tick.bOnScreen && IsFaunaShown())
	{
		PushTransforms();
	}
}

void AFaunaCloth::PushTransforms()
{
	for (int32 Index = 0; Index < Pieces.Num(); ++Index)
	{
		const FFaunaClothPiece& Piece = Pieces[Index];
		if (PieceInstance[Index] == INDEX_NONE)
		{
			continue;
		}
		const FFaunaClothSwing& Swing = Swings[Index];
		// Swung about the line (pitch), fluttering out and twisting a little (roll).
		const float Flap = FlutterDegrees * FMath::Sin(Swing.Flutter);
		const float Twist = FlutterDegrees * 0.6f * FMath::Sin(Swing.Flutter * 1.3f + 1.f);
		Poses[Piece.Mesh][PieceInstance[Index]] = FTransform(FRotator(Swing.Angle + Flap, Piece.Yaw, Twist), Piece.Location);
	}
	for (int32 Mesh = 0; Mesh < Parts.Num(); ++Mesh)
	{
		UInstancedStaticMeshComponent* Part = Parts[Mesh];
		if (Part && Poses[Mesh].Num() > 0 && Part->GetInstanceCount() == Poses[Mesh].Num())
		{
			Part->BatchUpdateInstancesTransforms(0, Poses[Mesh], /*bWorldSpace=*/true, /*bMarkRenderStateDirty=*/false, /*bTeleport=*/true);
		}
	}
}
