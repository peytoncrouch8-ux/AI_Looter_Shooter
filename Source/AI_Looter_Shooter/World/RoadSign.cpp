#include "World/RoadSign.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"

namespace
{
	/** One of the Signpost model's arrow boards, in the actor's frame (cm, degrees). */
	struct FSignBoard
	{
		/** The way its arrow points. */
		float Yaw;
		/** Its middle's height over the post's foot. */
		float Height;
		/** How far it droops toward its tip. */
		float Tilt;
		/** How far along it (from the post's middle) the name's middle goes. */
		float Along;
		/** The room along it for a name: its square part, clear of the post on the back and of the arrow's point. */
		float Room;
	};

	// Art/Models/Props/VillageProps.py signpost('Signpost', 50): the upper (teal) board 0.78 m long at 1.98 m, turned
	// -12 degrees in Blender, the lower (red) 0.70 m at 1.72 m turned 158; each starts 2 cm behind the post's middle and
	// ends in a point 12.8 cm long; Random(50) gave their droops (2.13 and 0.82 degrees). Blender's -Y is the actor's
	// +X and its -X the actor's +Y (Art/README.md), so a board turned psi in Blender points (-sin psi, -cos psi) here:
	// -78 and 112 degrees. Names sit on the square part, 9 cm from the post's middle (8 cm clear of it on the back) to a
	// few cm short of the point.
	constexpr FSignBoard Boards[2] = {
		{ -78.f, 198.f, 2.134f, 34.f, 50.f },
		{ 112.f, 172.f, 0.824f, 30.5f, 43.f },
	};
	/** A board's front face (cm out from the post's middle along its normal) and its back face. */
	constexpr float FrontFace = 9.f;
	constexpr float BackFace = 6.f;
	/** The paint stands this far off the face, so it never flickers into the wood. */
	constexpr float PaintLift = 0.3f;
}

ARoadSign::ARoadSign()
{
	PrimaryActorTick.bCanEverTick = false;
	Post = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Post"));
	RootComponent = Post;
	Post->SetMobility(EComponentMobility::Static);

	static const TCHAR* const FaceNames[4] = { TEXT("UpperFront"), TEXT("UpperBack"), TEXT("LowerFront"), TEXT("LowerBack") };
	for (const TCHAR* FaceName : FaceNames)
	{
		UTextRenderComponent* Text = CreateDefaultSubobject<UTextRenderComponent>(FaceName);
		Text->SetupAttachment(Post);
		Text->SetMobility(EComponentMobility::Static);
		Text->SetHorizontalAlignment(EHTA_Center);
		Text->SetVerticalAlignment(EVRTA_TextCenter);
		Text->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Text->SetCastShadow(false);
		Names.Add(Text);
	}
}

float ARoadSign::BoardYaw(int32 Board)
{
	return Boards[FMath::Clamp(Board, 0, 1)].Yaw;
}

FRoadSignFace ARoadSign::Face(int32 Board, bool bFront)
{
	const FSignBoard& B = Boards[FMath::Clamp(Board, 0, 1)];
	const float Yaw = FMath::DegreesToRadians(B.Yaw);
	const FVector Along(FMath::Cos(Yaw), FMath::Sin(Yaw), 0.f);
	// The front face looks out 90 degrees left of the arrow (VillageProps' -Y face, turned with the board).
	const FVector Out(-FMath::Sin(Yaw), FMath::Cos(Yaw), 0.f);
	const float Offset = bFront ? FrontFace + PaintLift : BackFace - PaintLift;
	FRoadSignFace Result;
	Result.Middle = Along * B.Along + Out * Offset
		+ FVector(0.f, 0.f, B.Height - B.Along * FMath::Sin(FMath::DegreesToRadians(B.Tilt)));
	// A text render faces its +X and reads toward its -Y, which is then the board's tip seen from the front and its tail
	// seen from the back: either way it reads left to right.
	Result.FacingYaw = bFront ? B.Yaw + 90.f : B.Yaw - 90.f;
	Result.Room = B.Room;
	return Result;
}

void ARoadSign::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	for (int32 Index = 0; Index < Names.Num(); ++Index)
	{
		const int32 Board = Index / 2;
		Lay(Names[Index], Board, Index % 2 == 0, Board == 0 ? UpperName : LowerName);
	}
}

void ARoadSign::Lay(UTextRenderComponent* Text, int32 Board, bool bFront, const FText& Name) const
{
	if (!Text)
	{
		return;
	}
	const FRoadSignFace Frame = Face(Board, bFront);
	Text->SetRelativeLocationAndRotation(Frame.Middle, FRotator(0.f, Frame.FacingYaw, 0.f));
	Text->SetText(Name);
	Text->SetTextRenderColor(PaintColor);
	Text->SetWorldSize(LetterHeight);
	Text->SetCullDistance(TextDrawDistance);
	// A long name is squeezed along its board rather than run past the point (or shrunk shorter than its neighbours).
	Text->SetXScale(1.f);
	const float Width = Text->GetTextLocalSize().Y;
	Text->SetXScale(Width > Frame.Room && Width > 0.f ? Frame.Room / Width : 1.f);
	Text->SetVisibility(!Name.IsEmpty());
}
