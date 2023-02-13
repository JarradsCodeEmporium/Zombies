//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "HighlightingCommon.generated.h"

//Controls how objects are highlighted
UENUM(BlueprintType)
enum class EHighlightType : uint8
{
	//Object is not highlighted at all
	None,

	//Object is dimly highlighted when behind another object
	XRay,

	//Object is given a selection highlight
	Selected,

	//Object is given bold highlight when under cursor/inside selection box
	Highlighted,

	//Maximum value of enum, do not use directly.
	Max,
};

//Color parameters for specific conditions
USTRUCT(BlueprintType)
struct ZOMBIES_API FHighlightColor
{
	GENERATED_BODY()
public:

	//Color of this object
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Selection)
	FLinearColor Color;

	//Transient stencil value, only set by game instance on load.
	uint8 StencilValue = 0;

	//Whether to outline this object
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Selection)
	bool bOutline = false;

};