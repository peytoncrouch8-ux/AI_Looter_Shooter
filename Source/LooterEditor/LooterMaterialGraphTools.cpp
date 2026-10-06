#include "LooterMaterialGraphTools.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpression.h"
#include "Materials/MaterialExpressionComment.h"
#include "UObject/Package.h"

namespace
{
	/** Takes a node out of its material for good: destroyed, or, when it's rooted, moved where nothing reaches it. */
	void Discard(UObject* Node, int32& InOutMovedOut)
	{
		if (Node->IsRooted())
		{
			Node->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional);
			++InOutMovedOut;
		}
		else
		{
			Node->MarkAsGarbage();
		}
	}
}

int32 ULooterMaterialGraphTools::ClearMaterialGraph(UMaterial* Material, int32& OutMovedOut)
{
	OutMovedOut = 0;
	if (!Material)
	{
		return 0;
	}
	Material->Modify();

	// The material's own inputs (base color, emissive, ...) point at nothing once their nodes are gone; the new graph
	// links the ones it uses.
	for (int32 Property = 0; Property < MP_MAX; ++Property)
	{
		if (FExpressionInput* Input = Material->GetExpressionInputForProperty(static_cast<EMaterialProperty>(Property)))
		{
			Input->Expression = nullptr;
		}
	}

	// Copies: removing a node changes the material's lists.
	TArray<UMaterialExpression*> Expressions;
	for (UMaterialExpression* Expression : Material->GetExpressions())
	{
		Expressions.Add(Expression);
	}
	TArray<UMaterialExpressionComment*> Comments;
	for (UMaterialExpressionComment* Comment : Material->GetEditorComments())
	{
		Comments.Add(Comment);
	}
	for (UMaterialExpression* Expression : Expressions)
	{
		if (Expression)
		{
			Material->RemoveExpressionParameter(Expression);
			Material->GetExpressionCollection().RemoveExpression(Expression);
			Discard(Expression, OutMovedOut);
		}
	}
	for (UMaterialExpressionComment* Comment : Comments)
	{
		if (Comment)
		{
			Material->GetExpressionCollection().RemoveComment(Comment);
			Discard(Comment, OutMovedOut);
		}
	}
	Material->MarkPackageDirty();
	return Expressions.Num() + Comments.Num();
}
