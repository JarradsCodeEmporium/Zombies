//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CommandCommon.h"
#include "CommandFollowerComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ZOMBIES_API UCommandFollowerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCommandFollowerComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	virtual void EndPlay(EEndPlayReason::Type Reason) override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void NotifyCommanderChanged(UCommanderComponent* InCommander);

	UFUNCTION(BlueprintCallable, Category = Command)
	bool IsCommandQueueEnabled() const;

	//Queues a command that will be followed once it reaches the head of the queue
	UFUNCTION(BlueprintCallable, Category = Command)
	void QueueCommand(ACommand* Command, ECommandQueueBehavior Behavior);

	//Unqueues a command, stopping following if it is the current command
	UFUNCTION(BlueprintCallable, Category = Command)
	void UnqueueCommand(ACommand* Command, EEndCommandReason Reason);

	//Clears entire command queue
	UFUNCTION(BlueprintCallable, Category = Command)
	void ClearCommandQueue();

	//Called when following a command to specify that the command could be finished if necessary
	UFUNCTION(BlueprintCallable, Category = Command)
	void SetIsFinishingCommand(ACommand* Command, bool bIsFinishingCommand);

	UFUNCTION(BlueprintCallable, Category = Command)
	FORCEINLINE bool IsFinishingCommand(ACommand* Command) const { return FinishingCommands.Contains(Command); };

	UFUNCTION(BlueprintCallable, Category = Command)
	ACommand* GetHeadOfCommandQueue() const;

	UFUNCTION(BlueprintCallable, Category = Command)
	ECommandFollowState GetCommandFollowState(ACommand* Command) const;

	//The location of the last command in the queue.
	//Commands whose location is not specified by the player will use this as their location.
	UFUNCTION(BlueprintCallable, Category = Command)
	FVector GetLastCommandLocation() const;

	FORCEINLINE auto GetCommander() const { return Commander; }
	FORCEINLINE const auto& GetCommandQueue() const { return CommandQueue; }
	FORCEINLINE const auto GetCurrentCommand() const { return CurrentCommand; }

	//void NotifyCommandQueued(ACommand* Command, ECommandQueueBehavior Behavior);

	void NotifyCommandUnqueued(ACommand* Command, EEndCommandReason Reason);

protected:

	void UpdateCommandQueue();

	void SetCurrentCommand(ACommand* Command);

	UPROPERTY(Transient, BlueprintReadOnly, Category = Command, Meta = (AllowPrivateAccess = "True"))
	UCommanderComponent* Commander;

	UPROPERTY(Transient, BlueprintReadOnly, Category = Command, Meta = (AllowPrivateAccess = "True"))
	TArray<ACommand*> CommandQueue;
	
	//Command that is currently being followed
	UPROPERTY(Transient, BlueprintReadOnly, Category = Command, Meta = (AllowPrivateAccess = "True"))
	ACommand* CurrentCommand;

	//Commands in queue which have had NotifyIsFinishingCommand() called on them
	UPROPERTY(Transient)
	TSet<ACommand*> FinishingCommands;
};
