//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Target.h"
#include "TargetedAbility.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UTargetedAbility : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class ZOMBIES_API ITargetedAbility
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	//Set the target of this ability, such as the actor to shoot, or the location on the ground to throw.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Target)
	void SetTarget(const FTarget& Target);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Target)
	FTarget GetTarget() const;
};
