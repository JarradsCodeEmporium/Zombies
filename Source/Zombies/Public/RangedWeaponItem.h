//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "WeaponItem.h"
#include "UtilityTask.h"
#include "RangedWeaponItem.generated.h"

/**
 * 
 */
UCLASS()
class ZOMBIES_API ARangedWeaponItem : public AWeaponItem
{
	GENERATED_BODY()
public:


};

UCLASS()
class ZOMBIES_API UUtilityTask_RangedWeapon : public UUtilityTask
{
	GENERATED_BODY()
public:

	virtual void SetPawn(class APawn* InPawn) override;

	virtual void UpdateScore_Implementation() override;

	virtual void BeginTask_Implementation() override;

	virtual void TickTask_Implementation(float DeltaTime) override;

	virtual void EndTask_Implementation() override;

	//The ability that should be used to attack at range
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Utility)
	TSubclassOf<class AShootAbility> ShootAbilityClass;

	//The focus level to use while aiming
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Utility)
	FGameplayTag AimingFocusLevel;

	//The trace channel to use for line of sight checks. 
	//E.G. It may be possible to see an enemy, but not shoot it because there is bullet proof glass in the way.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Utility)
	TEnumAsByte<ECollisionChannel> TraceChannel;

	//Maximum distance to consider using this task
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Utility)
	double MaxDistance = 800.0;

	//Weight of distance to target on final score
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Utility)
	double DistanceWeight = 1.0;

	//Weight of direction to target on final score
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Utility)
	double DirectionWeight = 1.0;

	//Bonus applied to current target to prefer sticking with it and discourage target switching until dead.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Utility)
	double CurrentTargetBonus = 0.2;

protected:

	//The target that is chosen to be attacked using ranged weapon
	UPROPERTY(BlueprintReadWrite, Category = Utility)
	class AActor* ShootTarget;

	//Cached ability component from pawn owner
	UPROPERTY(BlueprintReadWrite, Category = Utility)
	class UAbilityComponent* PawnAbilityComponent;

	//Cached focus component from pawn owner
	UPROPERTY(BlueprintReadWrite, Category = Utility)
	class UFocusComponent* PawnFocusComponent;

	void TryBeginAbility();
};