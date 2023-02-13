//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplaySelectable.h"
#include "GameplayTags.h"
#include "WorldCursor.h"
#include "GameplaySelectorComponent.generated.h"


USTRUCT()
struct FSelectableDisplayTypeOverride
{
	GENERATED_BODY()
public:

	TMap<FGameplayTag, EGameplaySelectionDisplayType> Overrides;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ZOMBIES_API UGameplaySelectorComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UGameplaySelectorComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	virtual void EndPlay(EEndPlayReason::Type Reason) override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	FORCEINLINE const auto& GetSelectedComponents() const { return SelectedComponents; }

	//Set the component to be selected/deselected by this
	UFUNCTION(BlueprintCallable, Category = Selection)
	void SetIsSelected(UGameplaySelectableComponent* Selectable, bool bIsSelected);

	//Is a specific component selected by this?
	UFUNCTION(BlueprintCallable, Category = Selection)
	bool IsSelected(UGameplaySelectableComponent* Selectable) const;

	UFUNCTION(BlueprintCallable, Category = Selection)
	void SetAllDeselected();

	//Are there any selected components?
	UFUNCTION(BlueprintCallable, Category = Selection)
	FORCEINLINE bool IsAnySelected() const { return SelectedComponents.Num() > 0; }

	//Called when this component selects/deselects another
	UPROPERTY(BlueprintAssignable, Category = Selection)
	FSelectableIsSelectedChangedDelegate OnIsSelectedChanged;

	//Called when this component changes its display type for a selector component
	UPROPERTY(BlueprintAssignable, Category = Selection)
	FSelectableDisplayTypeChangedDelegate OnDisplayTypeChanged;

	//The given selectable will be displayed with the given type if this override is the highest priority that has been set
	UFUNCTION(BlueprintCallable, Category = Selection)
	void SetDisplayTypeOverride(UGameplaySelectableComponent* Selectable, FGameplayTag Tag, EGameplaySelectionDisplayType DisplayType);

	//Clears override for the given selectable
	UFUNCTION(BlueprintCallable, Category = Selection)
	void ClearDisplayTypeOverride(UGameplaySelectableComponent* Selectable, FGameplayTag Tag);

	//Clears all display overrides for the selectable
	UFUNCTION(BlueprintCallable, Category = Selection)
	void ClearDisplayTypeOverridesForSelectable(UGameplaySelectableComponent* Selectable);

	UFUNCTION(BlueprintCallable, Category = Selection)
	void ClearDisplayTypeOverridesForTag(FGameplayTag Tag);

	//Clears all display type overrides for every component
	UFUNCTION(BlueprintCallable, Category = Selection)
	void ClearAllDisplayTypeOverrides();

	UFUNCTION(BlueprintCallable, Category = Selection)
	EGameplaySelectionDisplayType GetDisplayType(UGameplaySelectableComponent* Selectable) const;

	UFUNCTION(BlueprintCallable, Category = Selection)
	bool HandleWorldCursor(const FWorldCursorEventParams& Params);

protected:

	//Components that are currently selected
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Selection, Meta = (AllowPrivateAccess = "True"))
	TSet<UGameplaySelectableComponent*> SelectedComponents;

	//Currently managed display types for selectables.
	//Contains only selectables which currently have a display type other than None
	UPROPERTY()
	TMap<UGameplaySelectableComponent*, EGameplaySelectionDisplayType> SelectableDisplayTypes;

	//Specific overrides for selectable components
	UPROPERTY()
	TMap<UGameplaySelectableComponent*, FSelectableDisplayTypeOverride> SelectableDisplayTypeOverrides;

	//Selectable objects can define a generalized selection type, such as player unit, enemy unit, item, etc.
	//When a box selection is made, it contains many different types. First index is highest priority.
	//The first match in this list will filter the rest of the selected objects to match the same type.
	//E.G. if the list is PlayerUnit -> EnemyUnit -> Item and the box selection contains 3 enemies and 2 items,
	//only the enemy units will be part of the selection because they match the higher priority entry in the list.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Tactics)
	TArray<FGameplayTag> SelectionPriority;

	//Priority of display type overrides by tag. The first exact tag that has an override will be the display type
	UPROPERTY(EditAnywhere, Category = Selection)
	TArray<FGameplayTag> DisplayTypePriority;

	void UpdateDisplayType(UGameplaySelectableComponent* Selectable);
};
