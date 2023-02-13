//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "Command.h"
#include "WorldCursor.h"
#include "MoveCommand.generated.h"

UENUM(BlueprintType)
enum class EMoveCommandType : uint8
{
	//Simple location move with no facing direction
	Location,
	//Simple location move with a facing direction
	Facing,
};

/**
 * 
 */
UCLASS()
class ZOMBIES_API AMoveCommand : public ACommand
{
	GENERATED_BODY()
public:

	AMoveCommand();

	virtual void BeginPlayPooled_Implementation() override;

	virtual void InitCommand_Implementation() override;

	virtual void BeginFollowingCommand_Implementation(UCommandFollowerComponent* Follower, ACommand* PreviousCommand) override;

	virtual void TickFollowingCommand_Implementation(UCommandFollowerComponent* Follower, float DeltaTime) override;

	virtual void EndFollowingCommand_Implementation(UCommandFollowerComponent* Follower, ACommand* NextCommand) override;

	//Set command type. This is generally done automatically by cursor handler.
	UFUNCTION(BlueprintCallable, Category = Command)
	void SetMoveCommandType(EMoveCommandType NewType);

	//Type of move that this command represents
	FORCEINLINE EMoveCommandType GetMoveCommandType() const { return Type; }

	//If the cursor is dragged far enough away from the starting point,
	//then this move specifies a facing direction for the unit
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Command)
	float FacingMoveDistance = 50.f;

	//Minimum cursor screen distance to move to enable facing moves.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Command)
	float FacingMoveScreenDistance = 10.f;

	////Blackboard key for setting the movement location target vector
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Command)
	//FName LocationTargetKey;

	////Blackboard key for whether this move has a focusing location
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Command)
	//FName HasFocusTargetKey;

	////Blackboard key for the location to focus
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Command)
	//FName FocusTargetKey;

	////The focus level to use for facing moves
	////Note this should be higher than the automatic path following focus, otherwise it will always be overridden.
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Command)
	//FGameplayTag FacingMoveFocusLevel;

	//The class to use for move targets that implement moves for the command followers
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Command)
	TSubclassOf<class AMoveTarget> MoveTargetClass;

protected:

	void ReceiveEndCommand_Implementation() override;

	UPROPERTY(BlueprintReadOnly, Category = Command, Meta = (AllowPrivateAccess = "True"))
	EMoveCommandType Type;

	void UpdateFromCursorLocation(const struct FWorldCursorLocation& Cursor);

	UFUNCTION()
	EWorldCursorEventResult CursorHandler(const struct FWorldCursorEventParams& Params);

	UPROPERTY()
	FWorldCursorDelegateHandle CursorHandle;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Command)
	void OnMoveCommandTypeChanged(EMoveCommandType OldType);

	//Requires cursor to move on screen as well as world distance in order to make this a facing move
	//This helps when camera is moving and player is clicking without moving mouse
	FVector2D InitialCursorScreenLocation;

	//Move targets for each follower that has begun this move command
	//Is placed here because it's possible in the future we will want to coordinate the move targets for each follower in a move.
	UPROPERTY(BlueprintReadOnly, Category = Command, Meta = (AllowPrivateAccess = "True"))
	TMap<UCommandFollowerComponent*, class AMoveTarget*> MoveTargets;

	//Ensures a move target exists and is moving the follower
	void AssignMoveTarget(UCommandFollowerComponent* Follower);

	//Ensures the move target is destroyed and nothing is assign for the follower
	void ClearMoveTarget(UCommandFollowerComponent* Follower);
};
