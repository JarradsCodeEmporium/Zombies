//Copyright Jarrad Alexander 2022


#include "GameplaySelectableComponent.h"
#include "GameplaySelectorComponent.h"

// Sets default values for this component's properties
UGameplaySelectableComponent::UGameplaySelectableComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


// Called when the game starts
void UGameplaySelectableComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}

void UGameplaySelectableComponent::EndPlay(EEndPlayReason::Type Reason)
{
	Super::EndPlay(Reason);

	//Ensure all our selectors are not selecting or displaying us.

	SetDeselectedByAll();

	auto Selectors = DisplayedBy;

	for (auto Selector : Selectors)
		Selector->ClearDisplayTypeOverridesForSelectable(this);
}


// Called every frame
void UGameplaySelectableComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

bool UGameplaySelectableComponent::CanBeSelectedBy(UGameplaySelectorComponent* Selector) const
{
	//@todo: probably have a bool or delegate
	return true;
}

void UGameplaySelectableComponent::SetIsSelectedBy(UGameplaySelectorComponent* Selector, bool bIsSelected)
{
	if (!IsValid(Selector))
		return;

	Selector->SetIsSelected(this, bIsSelected);
}

void UGameplaySelectableComponent::SetDeselectedByAll()
{
	auto Selectors = SelectedBy;

	for (auto Selector : Selectors)
		Selector->SetIsSelected(this, false);
	
}

bool UGameplaySelectableComponent::IsSelectedBy(UGameplaySelectorComponent* Selector) const
{
	return IsValid(Selector) ? SelectedBy.Contains(Selector) : false;
}

void UGameplaySelectableComponent::NotifySelectionChanged(UGameplaySelectorComponent* Selector, bool bIsSelected)
{
	if (bIsSelected)
		SelectedBy.Add(Selector);
	else
		SelectedBy.Remove(Selector);
}

void UGameplaySelectableComponent::BroadcastSelectionChanged(UGameplaySelectorComponent* Selector, bool bIsSelected)
{
	auto Delegate = OnIsSelectedChanged;

	Delegate.Broadcast(this, Selector, bIsSelected);
}

void UGameplaySelectableComponent::NotifyDisplayTypeChanged(UGameplaySelectorComponent* Selector, EGameplaySelectionDisplayType DisplayType)
{
	if (DisplayType == EGameplaySelectionDisplayType::None)
		DisplayedBy.Remove(Selector);
	else
		DisplayedBy.Add(Selector);
}

void UGameplaySelectableComponent::BroadcastDisplayTypeChanged(UGameplaySelectorComponent* Selector, EGameplaySelectionDisplayType DisplayType)
{
	auto Delegate = OnDisplayTypeChanged;

	Delegate.Broadcast(this, Selector, DisplayType);
}

//
//EGameplaySelectionDisplayType UGameplaySelectableComponent::GetDisplayType(UGameplaySelectorComponent* Selector) const
//{
//	if (!IsValid(Selector))
//		return EGameplaySelectionDisplayType::None;
//
//	if (auto DisplayType = SelectorDisplayTypes.Find(Selector))
//		return *DisplayType;
//
//	return EGameplaySelectionDisplayType::None;
//}
//
//void UGameplaySelectableComponent::SetDisplayTypeOverrideForSelector(UGameplaySelectorComponent* Selector, FGameplayTag Tag, EGameplaySelectionDisplayType DisplayType)
//{
//	if (!IsValid(Selector))
//		return;
//
//	SelectorDisplayTypeOverrides.FindOrAdd(Selector).Overrides.FindOrAdd(Tag) = DisplayType;
//
//	UpdateDisplayTypes();
//}
//
//void UGameplaySelectableComponent::ClearDisplayTypeOverrideForSelector(UGameplaySelectorComponent* Selector, FGameplayTag Tag)
//{
//	if (!IsValid(Selector))
//		return;
//
//	auto SelectorOverrides = SelectorDisplayTypeOverrides.Find(Selector);
//
//	if (!SelectorOverrides)
//		return;
//
//	if (SelectorOverrides->Overrides.Remove(Tag) <= 0)
//		return;
//
//	if (SelectorOverrides->Overrides.Num() == 0)
//	{
//		//Selector has no override tags left, so remove it from the override map
//		SelectorOverrides = nullptr;
//
//		SelectorDisplayTypeOverrides.Remove(Selector);
//	}
//
//	//Update to broadcast events before potentially removing the selector
//	UpdateDisplayTypes();
//
//	TrimSelector(Selector);
//}
//
//void UGameplaySelectableComponent::SetDisplayTypeOverride(FGameplayTag Tag, EGameplaySelectionDisplayType DisplayType)
//{
//	auto& CurrentDisplayType = TagDisplayTypeOverrides.FindOrAdd(Tag);
//	
//	if (CurrentDisplayType == DisplayType)
//		return;
//
//	CurrentDisplayType = DisplayType;
//
//	UpdateDisplayTypes();
//}
//
//void UGameplaySelectableComponent::ClearDisplayTypeOverride(FGameplayTag Tag)
//{
//	if (TagDisplayTypeOverrides.Remove(Tag) <= 0)
//		return;
//
//	UpdateDisplayTypes();
//}
//
//void UGameplaySelectableComponent::UpdateDisplayTypes()
//{
//	TArray<TTuple<UGameplaySelectorComponent*, EGameplaySelectionDisplayType>> Changes;
//
//	for (auto& SelectorDisplayType : SelectorDisplayTypes)
//	{
//		auto Selector = SelectorDisplayType.Key;
//
//		//Need to break this out manually because structured bindings can't be captured in lambdas for some reason
//		auto& DisplayType = SelectorDisplayType.Value;
//
//		auto SelectorOverrides = SelectorDisplayTypeOverrides.Find(Selector);
//
//		bool bFoundOverride = false;
//
//		for (auto Priority : DisplayTypePriority)
//		{
//			auto FindOverride = [&](const TMap<FGameplayTag, EGameplaySelectionDisplayType>& Overrides) -> bool
//			{
//				auto DisplayTypeOverride = Overrides.Find(Priority);
//
//				if (!DisplayTypeOverride)
//					//No override for this specific priority tag
//					return false;
//
//				if (DisplayType != *DisplayTypeOverride)
//				{
//					//Found an override that is different to the current state
//					Changes.Add({ Selector, *DisplayTypeOverride });
//					DisplayType = *DisplayTypeOverride;
//				}
//
//				return true;
//			};
//
//			if (SelectorOverrides && FindOverride(SelectorOverrides->Overrides))
//			{
//				//Found a selector specific override
//				bFoundOverride = true;
//				break;
//			}
//
//			if (FindOverride(TagDisplayTypeOverrides))
//			{
//				//Found a global tag override
//				bFoundOverride = true;
//				break;
//			}
//		}
//
//		if (bFoundOverride)
//			//There was an override for this selector, so don't set it to the defaults
//			continue;
//
//		auto DefaultDisplayType = Selectors.Contains(Selector) ? EGameplaySelectionDisplayType::Selected : EGameplaySelectionDisplayType::None;
//
//		if (DefaultDisplayType != DisplayType)
//		{
//			//Default display type is different to its current display type
//			Changes.Add({ Selector, DefaultDisplayType });
//			DisplayType = DefaultDisplayType;
//		}
//
//	}
//
//	auto Delegate = OnDisplayTypeChanged;
//
//	for (auto& Change : Changes)
//	{
//		Delegate.Broadcast(this, Change.Get<0>(), Change.Get<1>());
//	}
//
//}
//
//void UGameplaySelectableComponent::TrimSelector(UGameplaySelectorComponent* Selector)
//{
//	if (!IsValid(Selector))
//		return;
//
//	if (Selectors.Contains(Selector) || SelectorDisplayTypeOverrides.Contains(Selector))
//		return;
//
//	//Selector has no overrides and is not selected so it has to be implicitly None with its events already broadcast,
//	//so we can forget about it entirely
//	SelectorDisplayTypes.Remove(Selector);
//}
//
