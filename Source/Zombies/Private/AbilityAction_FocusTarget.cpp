//Copyright Jarrad Alexander 2022


#include "AbilityAction_FocusTarget.h"
#include "FocusComponent.h"
#include "AbilityComponent.h"

UAbilityAction_FocusTarget* UAbilityAction_FocusTarget::FocusTarget(UObject* WorldContextObject, FName ActionName, FTarget Target, FGameplayTag FocusLevelTag, float MinAngleDeg)
{
	auto Action = NewAction<UAbilityAction_FocusTarget>(WorldContextObject, ActionName);

	if (!Action)
		return nullptr;

	Action->Target = Target;
	Action->FocusLevelTag = FocusLevelTag;
	Action->MinAngleDeg = MinAngleDeg;
	
	return Action;
}

void UAbilityAction_FocusTarget::Activate()
{
	Super::Activate();

	auto AbilityComponent = GetAbilityComponent();

	if (!AbilityComponent)
		return;

	FocusComponent = AbilityComponent->GetOwner()->FindComponentByClass<UFocusComponent>();

	if (!FocusComponent)
		return;

	FocusComponent->SetFocus(FocusLevelTag, Target);

	UpdateIsFocusingTarget();

}

void UAbilityAction_FocusTarget::Cancel()
{
	Super::Cancel();

	if (FocusComponent)
		FocusComponent->ClearFocus(FocusLevelTag);

	if (UpdateTimerHandle.IsValid())
		if (auto TimerManager = GetTimerManager())
			TimerManager->ClearTimer(UpdateTimerHandle);
}

void UAbilityAction_FocusTarget::UpdateIsFocusingTarget()
{
	SetIsFocusingTarget(IsFocusingTarget());

	auto TimerManager = GetTimerManager();

	if (!TimerManager)
		return;

	UpdateTimerHandle = TimerManager->SetTimerForNextTick(this, &UAbilityAction_FocusTarget::UpdateIsFocusingTarget);

}

bool UAbilityAction_FocusTarget::IsFocusingTarget() const
{
	if (!FocusComponent)
		return false;

	if (!FocusComponent->IsFocusLevelActive(FocusLevelTag))
		return false;

	if (!FocusComponent->IsFacingFocus(MinAngleDeg))
		return false;

	return true;
}

void UAbilityAction_FocusTarget::SetIsFocusingTarget(bool bNewIsFocusingTarget)
{
	if (bNewIsFocusingTarget == bIsFocusingTarget)
		return;

	bIsFocusingTarget = bNewIsFocusingTarget;

	if (!ShouldBroadcastDelegates())
		return;

	if (bIsFocusingTarget)
		GainedFocus.Broadcast();
	else
		LostFocus.Broadcast();
}
