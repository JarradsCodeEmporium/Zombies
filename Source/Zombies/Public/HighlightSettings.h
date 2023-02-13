//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "HighlightingCommon.h"
#include "HighlightSettings.generated.h"


/**
 * Defines the way selections are made and shown to the player
 */
UCLASS(BlueprintType)
class ZOMBIES_API UHighlightSettings : public UDataAsset
{
	GENERATED_BODY()
public:

	//Enables this highlight setting for display in the post process
	//If there are too many highlight settings assets in the project to display correctly,
	//go through the unused assets and delete them or uncheck this value
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Highlighting)
	bool bEnablePostProcess = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Highlighting)
	FHighlightColor XRay;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Highlighting)
	FHighlightColor Selected;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Highlighting)
	FHighlightColor Highlighted;

	//Gets the highlight color by type
	UFUNCTION(BlueprintCallable, Category = Highlighting)
	FHighlightColor GetHighlightColor(EHighlightType Type) const;

	//Sets the highlight on the component.
	//Internally sets render custom depth and stencil values on the components.
	UFUNCTION(BlueprintCallable, Category = Highlighting)
	void SetHighlighting(class UPrimitiveComponent* Primitive, EHighlightType Type);

};
