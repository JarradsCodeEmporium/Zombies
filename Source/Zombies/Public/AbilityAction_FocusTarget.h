//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "AbilityAction.h"
#include "Target.h"
#include "AbilityAction_FocusTarget.generated.h"

/**
 * 
 */
UCLASS()
class ZOMBIES_API UAbilityAction_FocusTarget : public UAbilityAction
{
	GENERATED_BODY()
public:

	//Makes the pawn focus component focus a specific target
	UFUNCTION(BlueprintCallable, Category = Ability, Meta = (BlueprintInternalUseOnly = "True", WorldContext = "WorldContextObject"))
	static UAbilityAction_FocusTarget* FocusTarget(UObject* WorldContextObject, FName ActionName, FTarget Target, FGameplayTag FocusLevelTag, float MinAngleDeg);

	virtual void Activate() override;

	virtual void Cancel() override;


	UPROPERTY(BlueprintReadWrite, Category = Ability)
	FTarget Target;

	UPROPERTY(BlueprintReadWrite, Category = Ability)
	FGameplayTag FocusLevelTag;

	UPROPERTY(BlueprintReadWrite, Category = Ability)
	float MinAngleDeg = 0.5f;

	//The focus component is looking at the target at the given focus level
	UPROPERTY(BlueprintAssignable)
	FGenericAbilityActionDelegate GainedFocus;

	//The focus component is no longer looking at the target
	UPROPERTY(BlueprintAssignable)
	FGenericAbilityActionDelegate LostFocus;

protected:

	//The focus component that is implementing this focus action.
	UPROPERTY(BlueprintReadOnly, Category = Ability)
	class UFocusComponent* FocusComponent;

	void UpdateIsFocusingTarget();

	bool IsFocusingTarget() const;

	void SetIsFocusingTarget(bool bNewIsFocusingTarget);

	bool bIsFocusingTarget = false;
	
	FTimerHandle UpdateTimerHandle;
};
