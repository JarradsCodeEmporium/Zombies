//Copyright Jarrad Alexander 2022


#include "GameplaySelectorComponent.h"
#include "GameplaySelectableComponent.h"

// Sets default values for this component's properties
UGameplaySelectorComponent::UGameplaySelectorComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


// Called when the game starts
void UGameplaySelectorComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	TMap<int, int> test;
	test.Add(4, 3);
	test.Add(4, 4);
	check(test[4] == 4);
}

void UGameplaySelectorComponent::EndPlay(EEndPlayReason::Type Reason)
{
	Super::EndPlay(Reason);

	SetAllDeselected();

	ClearAllDisplayTypeOverrides();
}


// Called every frame
void UGameplaySelectorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UGameplaySelectorComponent::SetIsSelected(UGameplaySelectableComponent* Selectable, bool bIsSelected)
{
	if (!IsValid(Selectable))
		return;

	if (IsSelected(Selectable) == bIsSelected)
		return;

	if (bIsSelected)
		SelectedComponents.Add(Selectable);
	else
		SelectedComponents.Remove(Selectable);

	Selectable->NotifySelectionChanged(this, bIsSelected);

	{
		auto Delegate = OnIsSelectedChanged;

		Delegate.Broadcast(Selectable, this, bIsSelected);

	}

	Selectable->BroadcastSelectionChanged(this, bIsSelected);

	UpdateDisplayType(Selectable);
}

bool UGameplaySelectorComponent::IsSelected(UGameplaySelectableComponent* Selectable) const
{
	return IsValid(Selectable) ? SelectedComponents.Contains(Selectable) : false;
}

void UGameplaySelectorComponent::SetAllDeselected()
{
	auto Selectables = SelectedComponents;

	for (auto Selectable : Selectables)
		SetIsSelected(Selectable, false);
}

void UGameplaySelectorComponent::SetDisplayTypeOverride(UGameplaySelectableComponent* Selectable, FGameplayTag Tag, EGameplaySelectionDisplayType DisplayType)
{
	if (!IsValid(Selectable) || !Tag.IsValid())
		return;

	auto& SelectableOverrides = SelectableDisplayTypeOverrides.FindOrAdd(Selectable);

	if (auto CurrentDisplayType = SelectableOverrides.Overrides.Find(Tag); CurrentDisplayType && *CurrentDisplayType == DisplayType)
		//Already set, no work to be done.
		return;
		
	SelectableOverrides.Overrides.Add(Tag, DisplayType);

	UpdateDisplayType(Selectable);
}

void UGameplaySelectorComponent::ClearDisplayTypeOverride(UGameplaySelectableComponent* Selectable, FGameplayTag Tag)
{
	if (!IsValid(Selectable))
		return;

	auto SelectableOverrides = SelectableDisplayTypeOverrides.Find(Selectable);

	if (!SelectableOverrides)
		return;

	if (SelectableOverrides->Overrides.Remove(Tag) <= 0)
		return;

	UpdateDisplayType(Selectable);
}

void UGameplaySelectorComponent::ClearDisplayTypeOverridesForSelectable(UGameplaySelectableComponent* Selectable)
{
	if (!IsValid(Selectable))
		return;

	if (SelectableDisplayTypeOverrides.Remove(Selectable) <= 0)
		return;

	UpdateDisplayType(Selectable);
}

void UGameplaySelectorComponent::ClearDisplayTypeOverridesForTag(FGameplayTag Tag)
{
	TArray<UGameplaySelectableComponent*> RemoveSelectables;
	TArray<UGameplaySelectableComponent*> ChangedSelectables;

	for (auto& [Selectable, Overrides] : SelectableDisplayTypeOverrides)
	{
		if (Overrides.Overrides.Remove(Tag) > 0)
			ChangedSelectables.Add(Selectable);

		if (Overrides.Overrides.Num() == 0)
			RemoveSelectables.Add(Selectable);
	}

	for (auto Selectable : RemoveSelectables)
		SelectableDisplayTypeOverrides.Remove(Selectable);

	for (auto Selectable : ChangedSelectables)
		UpdateDisplayType(Selectable);

}

void UGameplaySelectorComponent::ClearAllDisplayTypeOverrides()
{
	SelectableDisplayTypeOverrides.Empty();

	auto Selectables = SelectableDisplayTypes;

	for (auto [Selectable, DisplayType] : Selectables)
		UpdateDisplayType(Selectable);
}

EGameplaySelectionDisplayType UGameplaySelectorComponent::GetDisplayType(UGameplaySelectableComponent* Selectable) const
{
	if (!IsValid(Selectable))
		return EGameplaySelectionDisplayType::None;

	if (auto DisplayType = SelectableDisplayTypes.Find(Selectable))
		return *DisplayType;

	return EGameplaySelectionDisplayType::None;
}

bool UGameplaySelectorComponent::HandleWorldCursor(const FWorldCursorEventParams& Params)
{
	return false;
}

void UGameplaySelectorComponent::UpdateDisplayType(UGameplaySelectableComponent* Selectable)
{
	if (!IsValid(Selectable))
		return;

	EGameplaySelectionDisplayType NewDisplayType = EGameplaySelectionDisplayType::None;

	if (IsSelected(Selectable))
		//Default to selected
		NewDisplayType = EGameplaySelectionDisplayType::Selected;

	if (auto SelectableOverrides = SelectableDisplayTypeOverrides.Find(Selectable))
		for (auto Priority : DisplayTypePriority)
		{
			if (auto Override = SelectableOverrides->Overrides.Find(Priority))
			{
				//Use this override
				NewDisplayType = *Override;
				break;
			}

		}

	auto CurrentDisplayType = SelectableDisplayTypes.Find(Selectable);

	if (NewDisplayType == EGameplaySelectionDisplayType::None && !CurrentDisplayType)
		return;

	if (CurrentDisplayType && *CurrentDisplayType == NewDisplayType)
		return;

	if (CurrentDisplayType && NewDisplayType == EGameplaySelectionDisplayType::None)
		SelectableDisplayTypes.Remove(Selectable);

	if (!CurrentDisplayType)
		CurrentDisplayType = &SelectableDisplayTypes.Add(Selectable, NewDisplayType);

	*CurrentDisplayType = NewDisplayType;

	Selectable->NotifyDisplayTypeChanged(this, NewDisplayType);

	auto Delegate = OnDisplayTypeChanged;

	Delegate.Broadcast(Selectable, this, NewDisplayType);
	
	Selectable->BroadcastDisplayTypeChanged(this, NewDisplayType);
}

