//Copyright Jarrad Alexander 2022


#include "AbilityAction.h"
#include "Ability.h"
#include "AbilityComponent.h"

UAbilityAction* UAbilityAction::NewAction(UObject* WorldContextObject, FName ActionName, UClass* Class)
{
	check(Class);

	auto Ability = Cast<AAbility>(WorldContextObject);

	check(Ability);

	auto Action = NewObject<UAbilityAction>(Ability, Class);

	check(Action);

	Action->ActionName = ActionName;

	Ability->RegisterAbilityAction(Action);

	return Action;
}

void UAbilityAction::SetReadyToDestroy()
{
	GetAbility()->UnregisterAbilityAction(this);

	Super::SetReadyToDestroy();
}

AAbility* UAbilityAction::GetAbility() const
{
	return Cast<AAbility>(GetOuter());
}

UAbilityComponent* UAbilityAction::GetAbilityComponent() const
{
	auto Ability = GetAbility();

	if (!Ability)
		return nullptr;

	return Ability->GetAbilityComponent();
}

void UAbilityAction::Cancel()
{
	//UE_LOG(LogTemp, Warning, TEXT("Cancelled %s"), *ActionName.ToString());
	SetReadyToDestroy();
}

bool UAbilityAction::ShouldBroadcastDelegates() const
{
	return IsRegistered();
}

bool UAbilityAction::IsRegistered() const
{
	if (auto Ability = GetAbility())
		if (Ability->GetAbilityActions().Contains(this))
			return true;

	return false;
}

FTimerManager* UAbilityAction::GetTimerManager() const
{
	if (auto Ability = GetAbility())
		return &Ability->GetWorldTimerManager();

	return nullptr;
}
