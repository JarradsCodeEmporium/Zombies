//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PooledActor.h"
#include "GameplayTags.h"
#include "CommandCommon.h"
#include "WorldCursor.h"
#include "Command.generated.h"



//Displays and implements a behaviour the player intends a set of units to follow.
//Essentially this is a smart object for AI to use, that is partially configurable by the player
UCLASS()
class ZOMBIES_API ACommand : public AActor, public IPooledActor
{
	GENERATED_BODY()
	
public:	
	ACommand();

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(EEndPlayReason::Type Reason) override;

public:	

	virtual void Tick(float DeltaTime) override;

	virtual void BeginPlayPooled_Implementation() override;

	virtual void EndPlayPooled_Implementation(EEndPlayReason::Type Reason) override;

	//Called when first initialized
	virtual void NotifyCommanderChanged(UCommanderComponent* InCommander);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Command)
	void InitCommand();

	UFUNCTION(BlueprintNativeEvent, BlueprintPure, BlueprintCallable, Category = Command)
	FBeginCommandParams GetBeginCommandParams();

	//Called by command logic to indicate that relevant followers should now queue this command
	//Retrieves begin command parameters from commander.
	//@return: Whether the command was started. Can fail if current state is not Pending
	UFUNCTION(BlueprintCallable, Category = Command)
	bool BeginCommand();

	UPROPERTY(BlueprintAssignable, Category = Command)
	FCommandGenericDelegate OnBeginCommand;

	//Called by anything to indicate that all followers should stop and this command should end
	//Will always result in the command ending no matter the current state
	UFUNCTION(BlueprintCallable, Category = Command)
	void EndCommand(EEndCommandReason Reason);

	UPROPERTY(BlueprintAssignable, Category = Command)
	FCommandEndedDelegate OnEndCommand;

	void NotifyCommandQueued(UCommandFollowerComponent* Follower);

	void NotifyCommandUnqueued(UCommandFollowerComponent* Follower, EEndCommandReason Reason);

	//The follower has begun to follow this command, implement the behaviour of this command on the follower
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Command)
	void BeginFollowingCommand(UCommandFollowerComponent* Follower, ACommand* PreviousCommand);

	//The follower is following this command and needs an update tick
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Command)
	void TickFollowingCommand(UCommandFollowerComponent* Follower, float DeltaTime);

	//The follower has finished following this command, stop behaviour on the follower
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Command)
	void EndFollowingCommand(UCommandFollowerComponent* Follower, ACommand* NextCommand);

	FORCEINLINE auto GetCommander() const { return Commander; }
	FORCEINLINE auto GetCurrentState() const { return CurrentState; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Command)
	static class AAIController* GetAIController(const UCommandFollowerComponent* Follower);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Command)
	static class UBlackboardComponent* GetBlackboard(const UCommandFollowerComponent* Follower);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Command)
	static class UBehaviorTreeComponent* GetBehaviorTree(const UCommandFollowerComponent* Follower);

	//Get the world cursor component of the commanders owner
	UFUNCTION(BlueprintCallable, Category = Command)
	class UWorldCursorComponent* GetWorldCursorComponent() const;

	UFUNCTION(BlueprintCallable, Category = Command)
	bool GetCursorDirectionAndDistance(const struct FWorldCursorLocation& Cursor, FVector& OutDirection, double& OutDistance) const;

	//Checks if any followers actor also has a selectable component that is selected
	UFUNCTION(BlueprintCallable, Category = Command)
	bool IsAnyFollowerSelected() const;

	UFUNCTION(BlueprintCallable, Category = Command)
	void BeginVisualization();

	UFUNCTION(BlueprintCallable, Category = Command)
	void TickVisualization(float DeltaTime);

	UFUNCTION(BlueprintCallable, Category = Command)
	void EndVisualization();

	UFUNCTION(BlueprintCallable, Category = Command)
	bool ShouldVisualize() const;

	UFUNCTION(BlueprintCallable, Category = Command)
	FORCEINLINE bool IsVisualizing() const { return bIsVisualizing; }

	//Whether to allow this command to be visualized
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Command)
	bool bEnableVisualization = true;

	//Whether to call UpdateVisualization() in actor tick
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Command)
	bool bTickVisualization = true;

	//Only visualize the command when one of its followers is selected
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Command)
	bool bOnlyVisibleWhenSelected = true;

	//When any units are closer than this on XY, the command should completely fade out the relevant visualization elements
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Command)
	double ProximityFadeRadius = 100.f;

	//When any units are closer than this on Z, the command should completely fade out the relevant visualization elements
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Command)
	double ProximityFadeHeight = 100.f;

	//Distance it takes for the fade to fully fade out
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Command)
	double ProximityFadeDistance = 100.f;

	//Gets the alpha value for proximity relevant visualization elements
	UFUNCTION(BlueprintCallable, Category = Command)
	double GetProximityFadeOpacity() const;

protected:

	UPROPERTY(Transient, BlueprintReadOnly, Category = Command, Meta = (AllowPrivateAccess = "True"))
	UCommanderComponent* Commander;

	//Command followers that currently have this command queued
	UPROPERTY(Transient, BlueprintReadOnly, Category = Command, Meta = (AllowPrivateAccess = "True"))
	TSet<UCommandFollowerComponent*> Followers;

	//Current state of this commands lifetime
	UPROPERTY(Transient, BlueprintReadOnly, Category = Command, Meta = (AllowPrivateAccess = "True"))
	ECommandState CurrentState = ECommandState::Pending;

	//Gets the params that the command will use
	//Allows subclasses to easily configure the set of followers that can follow the command
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Command)
	FBeginCommandParams ReceiveBeginCommand();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Command)
	void ReceiveEndCommand();

	//Command actor can end via EndCommand(), EndPlayPooled(), or EndPlay()
	//so this function handles ending via any of these methods.
	void FinalizeEndCommand(EEndCommandReason Reason);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Command)
	void ReceiveBeginVisualization();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Command)
	void ReceiveTickVisualization(float DeltaTime);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Command)
	void ReceiveEndVisualization();

	//Check if we ShouldVisualize() and then begin/end visualization
	void UpdateVisualization();

	//Whether the command is currently visualizing
	bool bIsVisualizing = false;
};


//ACommand for followers that use AAIController to follow commands (i.e. most but not all)
//Includes default behavior for setting behavior tree and blackboard keys
UCLASS()
class ZOMBIES_API AAICommand : public ACommand
{
	GENERATED_BODY()

public:

	virtual void BeginFollowingCommand_Implementation(UCommandFollowerComponent* Follower, ACommand* PreviousCommand) override;

	virtual void EndFollowingCommand_Implementation(UCommandFollowerComponent* Follower, ACommand* NextCommand) override;

	//Always reset the behavior tree between commands.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Command)
	bool bResetBehaviorTree = false;

	//The behavior tree to run while following this command
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Command)
	class UBehaviorTree* BehaviorTree;

	//Blackboard key pointing to this command
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Command)
	FName CommandKey;

};
