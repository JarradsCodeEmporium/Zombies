//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FocusComponent.h"
#include "PooledActor.h"
#include "AITypes.h"
#include "Navigation/PathFollowingComponent.h"
#include "MoveTarget.generated.h"

//Manages a pawn moving to a target, including facing direction
UCLASS()
class ZOMBIES_API AMoveTarget : public AActor, public IPooledActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AMoveTarget();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void BeginPlayPooled_Implementation() override;

	virtual void EndPlayPooled_Implementation(EEndPlayReason::Type Reason) override;

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly)
	void SetPawn(class APawn* InPawn);

	FORCEINLINE class APawn* GetPawn() const { return Pawn; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MoveTarget)
	TSubclassOf<UNavigationQueryFilter> NavFilter;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MoveTarget)
	bool bUsePathfinding = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MoveTarget)
	bool bAllowPartialPath = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MoveTarget)
	bool bProjectGoalLocation = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MoveTarget)
	bool bReachTestIncludesAgentRadius = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MoveTarget)
	bool bCanStrafe = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MoveTarget)
	float AcceptanceRadius = UPathFollowingComponent::DefaultAcceptanceRadius;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MoveTarget)
	bool bEnableFocusing = true;

	//The tag level to apply the focus at.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MoveTarget)
	FGameplayTag FocusTag;

	//Minimum angle to accept the focus target reached.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MoveTarget)
	double FocusAcceptanceAngleDeg = 5.0;

	//When pawn is within this pathfinding "distance", then we should start focusing. Otherwise let the focus do what it wants
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MoveTarget)
	float FocusAtRemainingPathCost = 500.f;

	//When pawn is within this distance of the move target, then it should start focusing.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MoveTarget)
	double FocusAtMinDistance = 500.0;

	//Whether the pawn is within the distance that it should focus
	UFUNCTION(BlueprintCallable, Category = MoveTarget)
	bool WantsFocusing() const;

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly)
	void SetFocusOverride(const FTarget& InFocusOverride);

	UFUNCTION(BlueprintCallable, Category = MoveTarget)
	bool IsLocationConditionSatisfied() const;
	
	UFUNCTION(BlueprintCallable, Category = MoveTarget)
	bool IsFocusConditionSatisfied() const;

	//The focus target for this move, can be generic direction, or set by specific override
	UFUNCTION(BlueprintCallable, Category = MoveTarget)
	FTarget GetFocusTarget() const;

	FORCEINLINE const FTarget &GetFocusOverride() const { return FocusOverride; }

protected:

	//Override focus for this move target.
	//If not set, just uses this actors rotation as the focus.
	UPROPERTY(BlueprintReadWrite, BlueprintSetter = SetFocusOverride, Category = MoveTarget, Meta = (AllowPrivateAccess = "True"))
	FTarget FocusOverride;

	//The pawn that is following this move target
	UPROPERTY(BlueprintReadWrite, BlueprintSetter = SetPawn, Category = MoveTarget, Meta = (AllowPrivateAccess = "True"))
	class APawn* Pawn;

	EPathFollowingRequestResult::Type StartMove(class AAIController* AIController);

	void AbortMove(class AAIController* AIController);

	UFUNCTION()
	void ReceiveMoveCompleted(FAIRequestID RequestID, EPathFollowingResult::Type Result);

	void UpdatePawnActions();

	void UpdateMove();

	void UpdateFocus();
};
