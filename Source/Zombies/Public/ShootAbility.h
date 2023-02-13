//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "Ability.h"
#include "TargetedAbility.h"
#include "ShootAbility.generated.h"

/**
 * Base class for shooting abilities.
 */
UCLASS()
class ZOMBIES_API AShootAbility : public AAbility, public ITargetedAbility
{
	GENERATED_BODY()
public:

	//Begin ITargetedAbility

	FORCEINLINE virtual void SetTarget_Implementation(const FTarget& Target) override { ShootTarget = Target; }

	FORCEINLINE virtual FTarget GetTarget_Implementation() const override { return ShootTarget; }

	//End ITargetedAbility

	UPROPERTY(BlueprintReadWrite, Category = Ability)
	FTarget ShootTarget;

protected:


};
