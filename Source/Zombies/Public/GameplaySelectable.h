//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayTags.h"
#include "GameplaySelectable.generated.h"

class UGameplaySelectorComponent;
class UGameplaySelectableComponent;


//Controls how objects are selected using box selection
UENUM(BlueprintType)
enum class EGameplaySelectionBoxRule : uint8
{
	//Select if the selection box the center of the objects bounding box
	CenterPoint,
	//Select if the selection box covers the objects entire bounding box
	FullBounds,
	//Select if the selection box intersects any part of the objects bounding box
	AnyBounds,
};

//Controls how objects should display their selection highlight. Similar to EHighlightType, but specifies highlights in the context of selection.
UENUM(BlueprintType)
enum class EGameplaySelectionDisplayType : uint8
{
	//The object should not display as selected.
	None,
	//The object should display itself as being selected
	Selected,
	//The object should display itself as being highlighted and selected
	Highlighted,
};

//Controls how a new selection modifies the current selection.
UENUM(BlueprintType)
enum class EGameplaySelectionOp : uint8
{
	//The new selection completely replaces the current selection
	Replace,
	//The new selection is added to the current selection
	Add,
	//The new selection removes its objects from the current selection
	Subtract,
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FSelectableDisplayTypeChangedDelegate, UGameplaySelectableComponent*, Selectable, UGameplaySelectorComponent*, Selector, EGameplaySelectionDisplayType, NewDisplayType);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FSelectableIsSelectedChangedDelegate, UGameplaySelectableComponent*, Selectable, UGameplaySelectorComponent*, Selector, bool, bIsSelected);


// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UGameplaySelectable : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface for objects which are selectable by the players selection system
 */
class ZOMBIES_API IGameplaySelectable
{
	GENERATED_BODY()
public:

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Selection)
	UGameplaySelectableComponent* GetSelectableComponent() const;

};
