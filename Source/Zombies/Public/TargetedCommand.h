//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "Command.h"
#include "TargetedCommand.generated.h"

/**
 * Command for throwing/shooting from/to specific locations, while showing projectile path
 */
UCLASS()
class ZOMBIES_API ATargetedCommand : public AAICommand
{
	GENERATED_BODY()
public:

	virtual void BeginPlayPooled_Implementation() override;

	virtual void EndPlayPooled_Implementation(EEndPlayReason::Type Reason) override;

	virtual void InitCommand_Implementation() override;

	virtual void BeginFollowingCommand_Implementation(UCommandFollowerComponent* Follower, ACommand* PreviousCommand) override;

	virtual void EndFollowingCommand_Implementation(UCommandFollowerComponent* Follower, ACommand* NextCommand) override;


protected:

	virtual FBeginCommandParams ReceiveBeginCommand_Implementation() override;

	UFUNCTION()
	EWorldCursorEventResult CursorHandler(const struct FWorldCursorEventParams& Params);

	//We want to be able to confirm the command itself on spawn so that units can immediately start following and their targeted ability and just hold until we have a target
	//e.g. pull out a grenade and wait for player to click to confirm the target
	bool bTargetConfirmed;


};
