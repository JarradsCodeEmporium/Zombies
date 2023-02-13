//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplaySelectable.h"
#include "GameplaySelectableComponent.generated.h"



//USTRUCT()
//struct FSelectorDisplayTypeOverride
//{
//	GENERATED_BODY()
//public:
//
//	TMap<FGameplayTag, EGameplaySelectionDisplayType> Overrides;
//};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ZOMBIES_API UGameplaySelectableComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UGameplaySelectableComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	virtual void EndPlay(EEndPlayReason::Type Reason) override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	//Rule to use for box selections
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Selection)
	EGameplaySelectionBoxRule BoxRule = EGameplaySelectionBoxRule::CenterPoint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Selection)
	FGameplayTag SelectionTag;

	FORCEINLINE const auto& GetSelectors() const { return SelectedBy; }

	UFUNCTION(BlueprintCallable, Category = Selection)
	bool CanBeSelectedBy(UGameplaySelectorComponent* Selector) const;

	//Set the component to be selected by the given selector
	UFUNCTION(BlueprintCallable, Category = Selection)
	void SetIsSelectedBy(UGameplaySelectorComponent* Selector, bool bIsSelected);

	//Set the component to not be selected by anything
	UFUNCTION(BlueprintCallable, Category = Selection)
	void SetDeselectedByAll();

	//Is this selected by a specific component
	UFUNCTION(BlueprintCallable, Category = Selection)
	bool IsSelectedBy(UGameplaySelectorComponent* Selector) const;

	//Is this selected by any components?
	UFUNCTION(BlueprintCallable, Category = Selection)
	FORCEINLINE bool IsSelectedByAny() const { return SelectedBy.Num() > 0; }

	//Called when this component becomes selected/unselected
	UPROPERTY(BlueprintAssignable, Category = Selection)
	FSelectableIsSelectedChangedDelegate OnIsSelectedChanged;

	//Called when this component changes its display type for a selector component
	UPROPERTY(BlueprintAssignable, Category = Selection)
	FSelectableDisplayTypeChangedDelegate OnDisplayTypeChanged;

protected:

	friend UGameplaySelectorComponent;

	//Selector components that are currently selecting this component
	UPROPERTY(BlueprintReadOnly, Category = Selection, Meta = (AllowPrivateAccess = "True"))
	TSet<UGameplaySelectorComponent*> SelectedBy;

	//Selector components that currently have a display type for this component
	UPROPERTY(BlueprintReadOnly, Category = Selection, Meta = (AllowPrivateAccess = "True"))
	TSet<UGameplaySelectorComponent*> DisplayedBy;

	//Called by selector when this component is selected/deselected by it
	void NotifySelectionChanged(UGameplaySelectorComponent* Selector, bool bIsSelected);

	void BroadcastSelectionChanged(UGameplaySelectorComponent* Selector, bool bIsSelected);

	void NotifyDisplayTypeChanged(UGameplaySelectorComponent* Selector, EGameplaySelectionDisplayType DisplayType);

	void BroadcastDisplayTypeChanged(UGameplaySelectorComponent* Selector, EGameplaySelectionDisplayType DisplayType);


};
