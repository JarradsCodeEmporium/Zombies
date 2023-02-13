//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CommandCommon.h"
#include "CommanderComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ZOMBIES_API UCommanderComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCommanderComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	virtual void EndPlay(EEndPlayReason::Type Reason) override;


public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = Command)
	void AddFollower(UCommandFollowerComponent* Follower);

	UFUNCTION(BlueprintCallable, Category = Command)
	void RemoveFollower(UCommandFollowerComponent* Follower);

	UFUNCTION(BlueprintCallable, Category = Command)
	void RemoveAllFollowers();

	UPROPERTY(BlueprintAssignable, Category = Command)
	FCommandFollowerChangedDelegate OnFollowerAdded;

	UPROPERTY(BlueprintAssignable, Category = Command)
	FCommandFollowerChangedDelegate OnFollowerRemoved;

	UFUNCTION(BlueprintCallable, Category = Command)
	ACommand* SpawnCommand(const FTransform& Transform, TSubclassOf<ACommand> Class);

	template <typename T>
	T* SpawnCommand(const FTransform& Transform, TSubclassOf<ACommand> Class = nullptr)
	{
		return Cast<ACommand>(SpawnCommand(Class.Get() ? Class : T::StaticClass(), Transform));
	}

	//When a new command is beginning, specify the parameters it should use
	UFUNCTION(BlueprintCallable, Category = Command)
	FBeginCommandParams GetBeginCommandParams(ACommand* Command) const;

	//Provide override behavior for beginning commands
	UFUNCTION(BlueprintCallable, Category = Command)
	void SetBeginCommandParamsDelegate(FGetBeginCommandParamsDelegate Delegate);

	//Reset the override behavior for beginning commands
	UFUNCTION(BlueprintCallable, Category = Command)
	void ClearBeginCommandParamsDelegate();

	UFUNCTION(BlueprintCallable, Category = Command)
	void EndAllCommands();

	void NotifyFollowerAdded(UCommandFollowerComponent* Follower);

	void NotifyFollowerRemoved(UCommandFollowerComponent* Follower);

	void NotifyCommandAdded(ACommand* Command);

	void NotifyCommandRemoved(ACommand* Command);

	////Command followers that will recieve new commands (generally this will just be the selection)
	//UPROPERTY(Transient, BlueprintReadWrite, Category = Command)
	//TSet<UCommandFollowerComponent*> BeginCommandFollowers;

	////Specify queue behavior that new commands will use
	//UPROPERTY(Transient, BlueprintReadWrite, Category = Command)
	//EBeginCommandQueueBehavior BeginCommandQueueBehavior = EBeginCommandQueueBehavior::Replace;

	////Event allowing NewCommandFollowers to be updated before being passed to the new command
	////E.G. to filter/add/remove followers depending on type
	//UPROPERTY(BlueprintAssignable, Category = Command)
	//FCommandGenericDelegate OnBeginCommand;

	FORCEINLINE const auto& GetFollowers() const { return Followers; }
	FORCEINLINE const auto& GetCommands() const { return Commands; }

	//UFUNCTION(BlueprintCallable, Category = Command)
	//bool HandleWorldCursor(const FWorldCursorEventParams& Params);

protected:

	//Followers owned by this command component
	UPROPERTY(Transient, BlueprintReadOnly, Category = Command, Meta = (AllowPrivateAccess = "True"))
	TSet<UCommandFollowerComponent*> Followers;

	UPROPERTY(Transient, BlueprintReadOnly, Category = Command, Meta = (AllowPrivateAccess = "True"))
	TSet<ACommand*> Commands;

	FGetBeginCommandParamsDelegate OnGetBeginCommandParams;
};
