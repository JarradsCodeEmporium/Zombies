//Copyright Jarrad Alexander 2022


#include "AbilityComponent.h"
#include "Ability.h"
#include "UnitStatsComponent.h"

// Sets default values for this component's properties
UAbilityComponent::UAbilityComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UAbilityComponent::BeginPlay()
{
	Super::BeginPlay();

	UnitStatsComponent = GetOwner()->FindComponentByClass<UUnitStatsComponent>();
	
	SkeletalMeshComponent = GetOwner()->FindComponentByClass<USkeletalMeshComponent>();
}

void UAbilityComponent::EndPlay(EEndPlayReason::Type Reason)
{
	TGuardValue<bool> Guard(bIsEndingPlay, true);

	Super::EndPlay(Reason);

	EndAllAbilities(EEndAbilityReason::Cancelled);
}


// Called every frame
void UAbilityComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

}

void UAbilityComponent::GetOwnedGameplayTags(FGameplayTagContainer& Container) const
{
	Container.AppendTags(CustomOwnedTags);

	for (auto Ability : ActiveAbilities)
		Container.AppendTags(Ability->GrantedTags);
}

bool UAbilityComponent::CanBeginAbilityByClass(TSubclassOf<class AAbility> AbilityClass) const
{
	if (!AbilityClass.GetDefaultObject())
		return false;

	return AbilityClass.GetDefaultObject()->CanBeginAbility(this);
}

bool UAbilityComponent::CanBeginAbility(AAbility* Ability) const
{
	if (!Ability)
		return false;

	if (bIsEndingPlay)
		//Don't let new abilities activate in response to abilities being interrupted by end play.
		return false;

	return Ability->CanBeginAbility(this);
}

bool UAbilityComponent::BeginAbilityByClass(TSubclassOf<class AAbility> AbilityClass)
{
	if (!CanBeginAbilityByClass(AbilityClass))
		return false;

	auto Ability = AAbility::CreateAbility(this, AbilityClass);

	if (!Ability)
		return false;

	if (!BeginAbility(Ability))
	{
		Ability->DestroyAbility();
		return false;
	}

	return true;
}

bool UAbilityComponent::BeginAbility(AAbility* Ability)
{
	if (!Ability)
		return false;

	if (IsAbilityActive(Ability))
		return false;

	Ability->SetAbilityComponent(this);

	if (!Ability->CanBeginAbility(this))
		return false;

	//Need to do this before activating the ability, otherwise it could instantly interrupt itself
	EndAbilitiesWithTags(Ability->InterruptAbilityTags, EEndAbilityReason::Interrupted);

	//Technically this ability might not be able to activate depending on what the end ability delegates do, but we always force it here.

	ActiveAbilities.Add(Ability);

	Ability->OnBeginAbility();

	auto Delegate = Ability->OnBeginAbilityEvent;

	Delegate.Broadcast(Ability);

	return true;
}

bool UAbilityComponent::EndAbility(AAbility* Ability, EEndAbilityReason Reason)
{
	if (!Ability)
		return false;

	if (!IsAbilityActive(Ability))
		return false;

	ActiveAbilities.RemoveSingle(Ability);

	Ability->OnEndAbility(Reason);

	auto Delegate = Ability->OnEndAbilityEvent;

	Delegate.Broadcast(Ability, Reason);

	Ability->DestroyAbility();

	return true;
}

int32 UAbilityComponent::EndAbilitiesWithTags(const FGameplayTagContainer& EndTags, EEndAbilityReason Reason)
{
	int32 Result = 0;

	auto CurrentActiveAbilities = ActiveAbilities;

	for (auto Ability : CurrentActiveAbilities)
	{
		if (!IsValid(Ability))
			continue;

		if (!Ability->AbilityTags.HasAny(EndTags))
			continue;

		Result += (int32)EndAbility(Ability, Reason);
	}

	return Result;
}

int32 UAbilityComponent::EndAllAbilities(EEndAbilityReason Reason)
{
	auto EndAbilities = ActiveAbilities;
	
	for (auto Ability : EndAbilities)
		EndAbility(Ability, Reason);

	return EndAbilities.Num();
}

bool UAbilityComponent::IsAbilityActive(AAbility* Ability) const
{
	if (!Ability)
		return false;

	return ActiveAbilities.Contains(Ability);
}

AActor* UAbilityComponent::GetAvatarActor() const
{
	return GetOwner();
}

AController* UAbilityComponent::GetController() const
{
	if (auto Pawn = GetAvatarActor<APawn>())
		return Pawn->GetController();

	if (auto Controller = GetAvatarActor<AController>())
		return Controller;

	return nullptr;
}